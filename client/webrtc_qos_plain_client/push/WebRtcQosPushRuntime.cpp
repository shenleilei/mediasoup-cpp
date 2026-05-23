#include "push/WebRtcQosPushRuntime.h"

#include <chrono>
#include <thread>

#include "common/ClientIds.h"
#include "common/RtpRtcpClassifier.h"
#include "common/RuntimeLogHelpers.h"
#include "common/SdkRuntimeConfig.h"

namespace webrtc_qos_plain {

WebRtcQosPushRuntime::WebRtcQosPushRuntime(
	PushOptions options,
	PublishInfo publishInfo,
	std::shared_ptr<spdlog::logger> logger)
	: options_(std::move(options)),
	  publishInfo_(std::move(publishInfo)),
	  logger_(std::move(logger))
{
}

int WebRtcQosPushRuntime::Run(std::atomic<bool>& running, PushSignalingSession* signaling)
{
	PlainUdpTransport udp;
	std::string error;
	if (!udp.Connect(options_.mediaRemoteIp, publishInfo_.port, &error)) {
		logger_->error("udp_connect_failed remoteIp={} remotePort={} error={}",
			options_.mediaRemoteIp, publishInfo_.port, error);
		return 2;
	}

	H264AnnexBSource source(options_.input, options_.loopInput);
	if (!source.Open(&error)) {
		logger_->error("h264_source_open_failed input={} error={}", options_.input, error);
		return 2;
	}

	SingleVideoSessionParams sessionParams;
	sessionParams.roomId = options_.room;
	sessionParams.transportId = publishInfo_.transportId;
	sessionParams.sourceId = options_.peer;
	sessionParams.receiverId = "";
	sessionParams.senderSsrc = publishInfo_.ssrc;
	sessionParams.payloadType = publishInfo_.payloadType;
	sessionParams.transportCcExtId = publishInfo_.transportCcExtId;
	sessionParams.startBitrateBps = options_.startBitrateBps;
	sessionParams.minBitrateBps = options_.minBitrateBps;
	sessionParams.maxBitrateBps = options_.maxBitrateBps;
	sessionParams.debugName = "mediasoup_plain_push:" + options_.room + ":" + options_.peer;
	auto session = MakeSingleVideoSessionConfig(sessionParams);

	webrtc_qos::VideoPushClientConfig config;
	config.session = session;
	const bool sdkRuntimeFilesEnabled = ConfigureSdkRuntimeFiles(config, "push", options_.logDir);
	logger_->info("sdk_runtime_files role=push enabled={}", sdkRuntimeFilesEnabled);
	config.transport_output = [&](const webrtc_qos::TransportPacketView& packet) {
		std::string sendError;
		const bool ok = udp.Send(packet.bytes, packet.size, &sendError);
		if (!ok) {
			logger_->error("sdk_transport_output_failed kind={} bytes={} error={}",
				packet.metadata.kind == webrtc_qos::TransportPacketKind::kRtcp ? "rtcp" : "rtp",
				packet.size,
				sendError);
			return webrtc_qos::Status::Error(
				webrtc_qos::StatusCode::kInternalError,
				sendError);
		}
		return webrtc_qos::Status::Ok();
	};

	auto push = webrtc_qos::CreateVideoPushClient(config);
	if (!push) {
		logger_->error("create_video_push_client_failed");
		return 3;
	}
	auto status = push->Start();
	if (!status) {
		logger_->error("push_start_failed status={}", StatusToString(status));
		return 3;
	}

	logger_->info(
		"push_runtime_started roomId={} peerId={} producerId={} transportId={} videoSsrc={} videoPt={} transportCcExtId={} udpLocalIp={} udpLocalPort={} udpRemoteIp={} udpRemotePort={}",
		options_.room,
		options_.peer,
		publishInfo_.producerId,
		publishInfo_.transportId,
		publishInfo_.ssrc,
		publishInfo_.payloadType,
		publishInfo_.transportCcExtId,
		udp.localEndpoint().ip,
		udp.localEndpoint().port,
		udp.remoteEndpoint().ip,
		udp.remoteEndpoint().port);

	AnnexBAccessUnit nextAu;
	bool haveAu = source.NextAccessUnit(&nextAu, &error);
	if (!haveAu && !error.empty()) {
		logger_->error("h264_source_read_failed error={}", error);
		push->Stop();
		return 4;
	}

	bool firstAu = true;
	int64_t startWallUs = 0;
	int64_t firstMediaUs = 0;
	uint64_t pushedAu = 0;
	int64_t lastSnapshotUs = 0;

	while (running.load()) {
		const int64_t nowUs = MonotonicNowUs();
		if (signaling) signaling->DispatchNotifications();
		DrainUdpFeedback(udp, *push, nowUs);

		if (haveAu) {
			if (firstAu) {
				firstAu = false;
				startWallUs = nowUs;
				firstMediaUs = nextAu.mediaTimeUs;
			}
			const int64_t scheduledUs = startWallUs + (nextAu.mediaTimeUs - firstMediaUs);
			if (nowUs >= scheduledUs) {
				webrtc_qos::AnnexBAccessUnitView view;
				view.bytes = nextAu.bytes.data();
				view.size = nextAu.bytes.size();
				view.capture_time_us = scheduledUs;
				view.keyframe = nextAu.keyframe;
				view.ids = session.video_tracks.front().ids;
				status = push->PushAnnexBAccessUnit(view);
				if (!status) {
					logger_->error("push_au_failed status={}", StatusToString(status));
					push->Stop();
					return 4;
				}
				++pushedAu;
				haveAu = source.NextAccessUnit(&nextAu, &error);
				if (!haveAu && !error.empty()) {
					logger_->error("h264_source_read_failed error={}", error);
					push->Stop();
					return 4;
				}
			}
		} else if (!options_.loopInput) {
			logger_->info("push_input_eof pushedAu={} drainingMs=1000", pushedAu);
			const int64_t drainUntilUs = nowUs + 1000000;
			while (running.load() && MonotonicNowUs() < drainUntilUs) {
				const int64_t drainNowUs = MonotonicNowUs();
				if (signaling) signaling->DispatchNotifications();
				DrainUdpFeedback(udp, *push, drainNowUs);
				status = push->Process(drainNowUs);
				if (!status) {
					logger_->error("push_process_failed status={}", StatusToString(status));
					push->Stop();
					return 4;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(options_.processTickMs));
			}
			break;
		}

		status = push->Process(nowUs);
		if (!status) {
			logger_->error("push_process_failed status={}", StatusToString(status));
			push->Stop();
			return 4;
		}

		if (nowUs - lastSnapshotUs >= 1000000) {
			lastSnapshotUs = nowUs;
			auto snapshot = push->GetQosSnapshot(nowUs);
			auto adaptation = push->GetEncoderAdaptation(nowUs);
			logger_->info(
				"push_metrics pushedAu={} targetBps={} pacingBps={} finalTargetBps={} rttMs={} loss={} maxFps={} requestKeyframe={}",
				pushedAu,
				snapshot.sender_rates.googcc_target_bps,
				snapshot.sender_rates.pacing_bps,
				snapshot.sender_rates.final_target_bps,
				snapshot.sender_rates.rtt_ms,
				snapshot.sender_rates.loss_fraction,
				adaptation.max_fps,
				adaptation.request_keyframe);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(options_.processTickMs));
	}

	push->Stop();
	logger_->info("push_runtime_stopped pushedAu={}", pushedAu);
	return 0;
}

bool WebRtcQosPushRuntime::DrainUdpFeedback(
	PlainUdpTransport& udp,
	webrtc_qos::VideoPushClient& push,
	int64_t nowUs)
{
	bool progressed = false;
	uint8_t buffer[2048];
	while (true) {
		UdpEndpoint from;
		std::string error;
		const ssize_t received = udp.Recv(buffer, sizeof(buffer), &from, &error);
		if (received == 0) return progressed;
		if (received < 0) {
			logger_->warn("udp_recv_failed error={}", error);
			return progressed;
		}
		const auto kind = ClassifyRtpOrRtcp(buffer, static_cast<size_t>(received));
		if (kind == PacketKind::Rtcp) {
			auto status = push.OnTransportFeedback(buffer, static_cast<size_t>(received), nowUs);
			if (!status) {
				logger_->warn("push_feedback_failed bytes={} from={}:{} status={}",
					received,
					from.ip,
					from.port,
					StatusToString(status));
			}
			progressed = true;
		} else if (kind == PacketKind::Rtp) {
			logger_->warn("unexpected_inbound_rtp bytes={} from={}:{}", received, from.ip, from.port);
		} else {
			logger_->warn("malformed_inbound_packet bytes={} from={}:{}", received, from.ip, from.port);
		}
	}
}

} // namespace webrtc_qos_plain
