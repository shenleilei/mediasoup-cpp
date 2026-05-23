#include "push/WebRtcQosPushRuntime.h"

#include <chrono>
#include <optional>
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

	std::optional<H264AnnexBSource> copySource;
	std::optional<RealtimeH264Source> realtimeSource;
	const bool realtimeMode = options_.inputSynthetic && options_.encoder == "x264";
	if (realtimeMode) {
		RealtimeH264SourceConfig sourceConfig;
		sourceConfig.width = options_.syntheticWidth;
		sourceConfig.height = options_.syntheticHeight;
		sourceConfig.fps = options_.syntheticFps;
		sourceConfig.bitrateBps = options_.startBitrateBps;
		sourceConfig.minBitrateBps = options_.minBitrateBps;
		sourceConfig.maxBitrateBps = options_.maxBitrateBps;
		sourceConfig.pattern = options_.syntheticPattern;
		realtimeSource.emplace(sourceConfig);
		if (!realtimeSource->Open(&error)) {
			logger_->error("realtime_source_open_failed encoder={} pattern={} error={}",
				options_.encoder, options_.syntheticPattern, error);
			return 2;
		}
	} else {
		copySource.emplace(options_.input, options_.loopInput);
		if (!copySource->Open(&error)) {
			logger_->error("h264_source_open_failed input={} error={}", options_.input, error);
			return 2;
		}
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
		"push_runtime_started roomId={} peerId={} producerId={} transportId={} videoSsrc={} videoPt={} transportCcExtId={} sourceMode={} encoder={} udpLocalIp={} udpLocalPort={} udpRemoteIp={} udpRemotePort={}",
		options_.room,
		options_.peer,
		publishInfo_.producerId,
		publishInfo_.transportId,
		publishInfo_.ssrc,
		publishInfo_.payloadType,
		publishInfo_.transportCcExtId,
		realtimeMode ? "synthetic" : "copy",
		options_.encoder,
		udp.localEndpoint().ip,
		udp.localEndpoint().port,
		udp.remoteEndpoint().ip,
		udp.remoteEndpoint().port);

	AnnexBAccessUnit nextAu;
	bool haveAu = false;
	if (!realtimeMode) {
		haveAu = copySource->NextAccessUnit(&nextAu, &error);
	}
	if (!realtimeMode && !haveAu && !error.empty()) {
		logger_->error("h264_source_read_failed error={}", error);
		push->Stop();
		return 4;
	}

	bool firstAu = true;
	int64_t startWallUs = 0;
	int64_t firstMediaUs = 0;
	uint64_t pushedAu = 0;
	int64_t lastSnapshotUs = 0;
	FeedbackCounters feedbackCounters;

	while (running.load()) {
		const int64_t nowUs = MonotonicNowUs();
		if (signaling) signaling->DispatchNotifications();
		DrainUdpFeedback(udp, *push, nowUs, feedbackCounters);
		const auto adaptation = push->GetEncoderAdaptation(nowUs);
		if (realtimeSource) {
			std::string adaptationError;
			if (!realtimeSource->ApplyEncoderAdaptation(adaptation, &adaptationError)) {
				logger_->error("encoder_adaptation_failed status={}", adaptationError);
				push->Stop();
				return 4;
			}
		}

		if (realtimeSource) {
			bool produced = false;
			while (realtimeSource->NextAccessUnit(nowUs, &nextAu, &error)) {
				produced = true;
				webrtc_qos::AnnexBAccessUnitView view;
				view.bytes = nextAu.bytes.data();
				view.size = nextAu.bytes.size();
				view.capture_time_us = startWallUs == 0 ? nowUs : startWallUs + nextAu.mediaTimeUs;
				view.keyframe = nextAu.keyframe;
				view.ids = session.video_tracks.front().ids;
				status = push->PushAnnexBAccessUnit(view);
				if (!status) {
					logger_->error("push_au_failed status={}", StatusToString(status));
					push->Stop();
					return 4;
				}
				++pushedAu;
			}
			if (!error.empty()) {
				logger_->error("realtime_source_read_failed error={}", error);
				push->Stop();
				return 4;
			}
			if (produced && startWallUs == 0) startWallUs = nowUs;
		} else if (haveAu) {
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
				haveAu = copySource->NextAccessUnit(&nextAu, &error);
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
				DrainUdpFeedback(udp, *push, drainNowUs, feedbackCounters);
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
			logger_->info(
				"push_metrics pushedAu={} targetBps={} pacingBps={} finalTargetBps={} rttMs={} loss={} rtcpFeedbackPacketsIn={} rtcpFeedbackBytesIn={} rtcpFeedbackFailures={} maxFps={} requestKeyframe={}",
				pushedAu,
				snapshot.sender_rates.googcc_target_bps,
				snapshot.sender_rates.pacing_bps,
				snapshot.sender_rates.final_target_bps,
				snapshot.sender_rates.rtt_ms,
				snapshot.sender_rates.loss_fraction,
				feedbackCounters.rtcpPacketsIn,
				feedbackCounters.rtcpBytesIn,
				feedbackCounters.rtcpFailures,
				adaptation.max_fps,
				adaptation.request_keyframe);
			if (realtimeSource) {
				const auto& sourceMetrics = realtimeSource->metrics();
				logger_->info(
					"encoder_metrics mode=synthetic encoder={} currentBitrateBps={} currentFps={} width={} height={} framesGenerated={} framesEncoded={} accessUnits={} keyframes={} encoderRecreates={} bitrateChanges={} fpsChanges={} forcedKeyframeRequests={} lastKeyframe={}",
					options_.encoder,
					sourceMetrics.currentBitrateBps,
					sourceMetrics.currentFps,
					sourceMetrics.width,
					sourceMetrics.height,
					sourceMetrics.framesGenerated,
					sourceMetrics.framesEncoded,
					sourceMetrics.accessUnits,
					sourceMetrics.keyframes,
					sourceMetrics.encoderRecreates,
					sourceMetrics.bitrateChanges,
					sourceMetrics.fpsChanges,
					sourceMetrics.forcedKeyframeRequests,
					sourceMetrics.lastAccessUnitKeyframe);
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(options_.processTickMs));
	}

	push->Stop();
	logger_->info(
		"push_runtime_stopped pushedAu={} rtcpFeedbackPacketsIn={} rtcpFeedbackBytesIn={} rtcpFeedbackFailures={}",
		pushedAu,
		feedbackCounters.rtcpPacketsIn,
		feedbackCounters.rtcpBytesIn,
		feedbackCounters.rtcpFailures);
	return 0;
}

bool WebRtcQosPushRuntime::DrainUdpFeedback(
	PlainUdpTransport& udp,
	webrtc_qos::VideoPushClient& push,
	int64_t nowUs,
	FeedbackCounters& counters)
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
			++counters.rtcpPacketsIn;
			counters.rtcpBytesIn += static_cast<uint64_t>(received);
			auto status = push.OnTransportFeedback(buffer, static_cast<size_t>(received), nowUs);
			if (!status) {
				++counters.rtcpFailures;
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
