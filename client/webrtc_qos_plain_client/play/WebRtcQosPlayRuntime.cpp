#include "play/WebRtcQosPlayRuntime.h"

#include <chrono>
#include <thread>
#include <utility>

#include "common/ClientIds.h"
#include "common/RtpRtcpClassifier.h"
#include "common/RuntimeLogHelpers.h"
#include "common/SdkRuntimeConfig.h"
#include "webrtc_qos/video_play_client.h"

namespace webrtc_qos_plain {

WebRtcQosPlayRuntime::WebRtcQosPlayRuntime(
	PlayOptions options,
	ConsumerInfo consumerInfo,
	std::shared_ptr<spdlog::logger> logger,
	PlainUdpTransport udp)
	: options_(std::move(options)),
	  consumerInfo_(std::move(consumerInfo)),
	  logger_(std::move(logger)),
	  udp_(std::move(udp))
{
}

int WebRtcQosPlayRuntime::Run(std::atomic<bool>& running, PlaySignalingSession* signaling)
{
	AnnexBSink sink;
	std::string error;
	if (options_.outputNull) {
		sink.EnableNull();
	} else if (!sink.OpenFile(options_.outputAu, &error)) {
		logger_->error("annexb_sink_open_failed output={} error={}", options_.outputAu, error);
		return 2;
	}

	SingleVideoSessionParams sessionParams;
	sessionParams.roomId = options_.room;
	sessionParams.transportId = consumerInfo_.transportId;
	sessionParams.sourceId = consumerInfo_.peerId.empty() ? consumerInfo_.producerId : consumerInfo_.peerId;
	sessionParams.receiverId = options_.peer;
	sessionParams.receiverIdOverride = options_.receiverId;
	sessionParams.senderSsrc = consumerInfo_.ssrc;
	sessionParams.payloadType = consumerInfo_.payloadType;
	sessionParams.transportCcExtId = consumerInfo_.transportCcExtId;
	sessionParams.startBitrateBps = options_.startBitrateBps;
	sessionParams.minBitrateBps = options_.minBitrateBps;
	sessionParams.maxBitrateBps = options_.maxBitrateBps;
	sessionParams.debugName = "mediasoup_plain_play:" + options_.room + ":" + options_.peer;
	auto session = MakeSingleVideoSessionConfig(sessionParams);

	webrtc_qos::VideoPlayClientConfig config;
	config.session = session;
	const bool sdkRuntimeFilesEnabled = ConfigureSdkRuntimeFiles(config, "play", options_.logDir);
	logger_->info("sdk_runtime_files role=play enabled={}", sdkRuntimeFilesEnabled);
	config.transport_output = [&](const webrtc_qos::TransportPacketView& packet) {
		std::string sendError;
		const bool ok = udp_.SendTo(
			options_.mediaRemoteIp,
			consumerInfo_.plainTransportPort,
			packet.bytes,
			packet.size,
			&sendError);
		if (!ok) {
			logger_->error("sdk_transport_output_failed kind=rtcp bytes={} remoteIp={} remotePort={} error={}",
				packet.size,
				options_.mediaRemoteIp,
				consumerInfo_.plainTransportPort,
				sendError);
			return webrtc_qos::Status::Error(
				webrtc_qos::StatusCode::kInternalError,
				sendError);
		}
		return webrtc_qos::Status::Ok();
	};
	config.decoded_access_unit_output = [&](const webrtc_qos::AnnexBAccessUnitView& accessUnit) {
		auto status = sink.Write(accessUnit);
		if (!status) logger_->warn("annexb_sink_write_failed status={}", StatusToString(status));
		return status;
	};

	auto play = webrtc_qos::CreateVideoPlayClient(config);
	if (!play) {
		logger_->error("create_video_play_client_failed");
		return 3;
	}
	auto status = play->Start();
	if (!status) {
		logger_->error("play_start_failed status={}", StatusToString(status));
		return 3;
	}

	logger_->info(
		"play_runtime_started roomId={} peerId={} producerPeerId={} producerId={} consumerId={} transportId={} videoSsrc={} videoPt={} transportCcExtId={} udpLocalIp={} udpLocalPort={} udpRemoteIp={} udpRemotePort={}",
		options_.room,
		options_.peer,
		consumerInfo_.peerId,
		consumerInfo_.producerId,
		consumerInfo_.consumerId,
		consumerInfo_.transportId,
		consumerInfo_.ssrc,
		consumerInfo_.payloadType,
		consumerInfo_.transportCcExtId,
		udp_.localEndpoint().ip,
		udp_.localEndpoint().port,
		options_.mediaRemoteIp,
		consumerInfo_.plainTransportPort);

	uint8_t buffer[2048];
	uint64_t rtpPackets = 0;
	uint64_t rtcpPackets = 0;
	int64_t lastSnapshotUs = 0;

	while (running.load()) {
		const int64_t nowUs = MonotonicNowUs();
		if (signaling) signaling->DispatchNotifications();
		while (true) {
			UdpEndpoint from;
			std::string recvError;
			const ssize_t received = udp_.Recv(buffer, sizeof(buffer), &from, &recvError);
			if (received == 0) break;
			if (received < 0) {
				logger_->warn("udp_recv_failed error={}", recvError);
				break;
			}
			const auto kind = ClassifyRtpOrRtcp(buffer, static_cast<size_t>(received));
			if (kind == PacketKind::Rtp) {
				status = play->OnRtpPacket(buffer, static_cast<size_t>(received), nowUs);
				++rtpPackets;
			} else if (kind == PacketKind::Rtcp) {
				status = play->OnRtcpPacket(buffer, static_cast<size_t>(received), nowUs);
				++rtcpPackets;
			} else {
				logger_->warn("malformed_inbound_packet bytes={} from={}:{}", received, from.ip, from.port);
				continue;
			}
			if (!status) {
				logger_->warn("play_packet_input_failed kind={} bytes={} status={}",
					PacketKindName(kind),
					received,
					StatusToString(status));
			}
		}

		status = play->Process(nowUs);
		if (!status) {
			logger_->error("play_process_failed status={}", StatusToString(status));
			play->Stop();
			return 4;
		}

		if (nowUs - lastSnapshotUs >= 1000000) {
			lastSnapshotUs = nowUs;
			auto snapshot = play->GetQosSnapshot(nowUs);
			logger_->info(
				"play_metrics rtpPackets={} rtcpPackets={} outputAu={} nack={} pli={} retransmission={} droppedRetransmission={} rttMs={} lossQ8={}",
				rtpPackets,
				rtcpPackets,
				sink.writtenAccessUnits(),
				snapshot.nack_count,
				snapshot.pli_count,
				snapshot.retransmission_count,
				snapshot.dropped_retransmission_packets,
				snapshot.downlink_quality.rtt_ms,
				snapshot.downlink_quality.fraction_lost_q8);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(options_.processTickMs));
	}

	play->Stop();
	logger_->info("play_runtime_stopped rtpPackets={} rtcpPackets={} outputAu={}",
		rtpPackets,
		rtcpPackets,
		sink.writtenAccessUnits());
	return 0;
}

} // namespace webrtc_qos_plain
