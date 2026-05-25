#include "push/WebRtcQosPushRuntime.h"

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/ClientIds.h"
#include "common/RuntimeLogHelpers.h"
#include "push/PushSdkTransportThread.h"
#include "push/PushTrackSourceWorker.h"

namespace webrtc_qos_plain {
namespace {

PushTrackSourceMode SourceModeFromOptions(bool realtimeMode, bool mp4DecodeMode, bool v4l2Mode)
{
	if (realtimeMode) return PushTrackSourceMode::kSynthetic;
	if (mp4DecodeMode) return PushTrackSourceMode::kMp4DecodeLoop;
	if (v4l2Mode) return PushTrackSourceMode::kV4L2;
	return PushTrackSourceMode::kCopy;
}

const PushTrackOptions* FindTrackOptionsBySsrc(
	const std::unordered_map<uint32_t, const PushTrackOptions*>& tracksBySsrc,
	uint32_t ssrc)
{
	const auto it = tracksBySsrc.find(ssrc);
	return it == tracksBySsrc.end() ? nullptr : it->second;
}

} // namespace

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
	std::string error;
	const bool realtimeMode = options_.inputSynthetic && options_.encoder == "x264";
	const bool mp4DecodeMode = options_.inputDecodeLoop && options_.encoder == "x264";
	bool trackV4L2Mode = false;
	std::unordered_map<uint32_t, const PushTrackOptions*> tracksBySsrc;
	for (const auto& track : options_.tracks) {
		tracksBySsrc[track.videoSsrc] = &track;
		trackV4L2Mode = trackV4L2Mode || track.source == "v4l2" || !track.v4l2Device.empty();
	}
	const bool v4l2Mode = (!options_.inputV4L2.empty() || trackV4L2Mode) && options_.encoder == "x264";
	const std::string sourceMode =
		realtimeMode ? "synthetic" : (mp4DecodeMode ? "mp4_decode_loop" : (v4l2Mode ? "v4l2" : "copy"));

	VideoSessionParams sessionParams;
	sessionParams.roomId = options_.room;
	sessionParams.transportId = publishInfo_.transportId;
	sessionParams.sourceId = options_.peer;
	sessionParams.receiverId = "";
	sessionParams.startBitrateBps = options_.startBitrateBps;
	sessionParams.minBitrateBps = options_.minBitrateBps;
	sessionParams.maxBitrateBps = options_.maxBitrateBps;
	sessionParams.debugName = "mediasoup_plain_push:" + options_.room + ":" + options_.peer;
	for (size_t index = 0; index < publishInfo_.videoTracks.size(); ++index) {
		const auto& publishedTrack = publishInfo_.videoTracks[index];
		VideoTrackSessionParams track;
		track.trackIdString = publishedTrack.trackId;
		track.trackId = static_cast<uint32_t>(index + 1);
		track.senderSsrc = publishedTrack.ssrc;
		track.payloadType = publishedTrack.payloadType;
		track.transportCcExtId = publishedTrack.transportCcExtId;
		track.weight = publishedTrack.weight;
		track.baseTrack = index == 0;
		sessionParams.tracks.push_back(track);
	}
	if (sessionParams.tracks.empty()) {
		VideoTrackSessionParams track;
		track.trackIdString = "track0";
		track.trackId = 1;
		track.senderSsrc = publishInfo_.ssrc;
		track.payloadType = publishInfo_.payloadType;
		track.transportCcExtId = publishInfo_.transportCcExtId;
		track.weight = 100;
		track.baseTrack = true;
		sessionParams.tracks.push_back(track);
	}
	auto session = MakeVideoSessionConfig(sessionParams);

	PushSdkTransportThreadConfig sdkThreadConfig;
	sdkThreadConfig.session = session;
	sdkThreadConfig.mediaRemoteIp = options_.mediaRemoteIp;
	sdkThreadConfig.mediaRemotePort = publishInfo_.port;
	sdkThreadConfig.logDir = options_.logDir;
	sdkThreadConfig.processTickMs = options_.processTickMs;
	PushSdkTransportThread sdkThread(std::move(sdkThreadConfig), logger_);
	const int sdkStartRc = sdkThread.Start(&error);
	if (sdkStartRc != 0) {
		logger_->error("push_sdk_transport_thread_start_failed error={}", error);
		return sdkStartRc;
	}
	const auto startMetrics = sdkThread.metrics();

	logger_->info(
		"push_runtime_started roomId={} peerId={} producerId={} transportId={} trackCount={} videoSsrc={} videoPt={} transportCcExtId={} sourceMode={} encoder={} udpLocalIp={} udpLocalPort={} udpRemoteIp={} udpRemotePort={}",
		options_.room,
		options_.peer,
		publishInfo_.producerId,
		publishInfo_.transportId,
		session.video_tracks.size(),
		publishInfo_.ssrc,
		publishInfo_.payloadType,
		publishInfo_.transportCcExtId,
		sourceMode,
		options_.encoder,
		startMetrics.localEndpoint.ip,
		startMetrics.localEndpoint.port,
		startMetrics.remoteEndpoint.ip,
		startMetrics.remoteEndpoint.port);

	std::vector<std::unique_ptr<PushTrackSourceWorker>> sourceWorkers;
	sourceWorkers.reserve(session.video_tracks.size());
	const auto sourceWorkerMode = SourceModeFromOptions(realtimeMode, mp4DecodeMode, v4l2Mode);
	for (size_t index = 0; index < session.video_tracks.size(); ++index) {
		const auto& track = session.video_tracks[index];
		PushTrackSourceWorkerConfig sourceConfig;
		sourceConfig.ids = track.ids;
		sourceConfig.trackName = index < sessionParams.tracks.size()
			? sessionParams.tracks[index].trackIdString
			: ("track" + std::to_string(index));
		sourceConfig.mode = sourceWorkerMode;
		sourceConfig.inputPath = options_.input;
		sourceConfig.loopInput = options_.loopInput;
		sourceConfig.encoder = options_.encoder;
		sourceConfig.processTickMs = options_.processTickMs;
		sourceConfig.syntheticWidth = options_.syntheticWidth;
		sourceConfig.syntheticHeight = options_.syntheticHeight;
		sourceConfig.syntheticFps = options_.syntheticFps;
		sourceConfig.syntheticPattern = options_.syntheticPattern;
		sourceConfig.v4l2Device = options_.inputV4L2;
		sourceConfig.v4l2Width = options_.v4l2Width;
		sourceConfig.v4l2Height = options_.v4l2Height;
		sourceConfig.v4l2Fps = options_.v4l2Fps;
		sourceConfig.v4l2InputFormat = options_.v4l2InputFormat;
		if (const auto* trackOptions = FindTrackOptionsBySsrc(tracksBySsrc, track.ids.sender_ssrc)) {
			if (!trackOptions->v4l2Device.empty()) sourceConfig.v4l2Device = trackOptions->v4l2Device;
			if (trackOptions->v4l2Width > 0) sourceConfig.v4l2Width = trackOptions->v4l2Width;
			if (trackOptions->v4l2Height > 0) sourceConfig.v4l2Height = trackOptions->v4l2Height;
			if (trackOptions->v4l2Fps > 0) sourceConfig.v4l2Fps = trackOptions->v4l2Fps;
			if (!trackOptions->v4l2InputFormat.empty()) sourceConfig.v4l2InputFormat = trackOptions->v4l2InputFormat;
		}
		if (sourceWorkerMode == PushTrackSourceMode::kV4L2 && sourceConfig.v4l2Device.empty()) {
			logger_->error(
				"push_track_source_worker_start_failed trackId={} senderSsrc={} mode=v4l2 error=missing_v4l2_device",
				track.ids.track_id,
				track.ids.sender_ssrc);
			for (auto& startedWorker : sourceWorkers) startedWorker->Stop();
			sdkThread.Stop();
			return 2;
		}
		sourceConfig.injectEncoderDelayMs = options_.injectEncoderDelayMs;
		sourceConfig.startBitrateBps = options_.startBitrateBps;
		sourceConfig.minBitrateBps = options_.minBitrateBps;
		sourceConfig.maxBitrateBps = options_.maxBitrateBps;
		auto worker = std::make_unique<PushTrackSourceWorker>(sourceConfig, &sdkThread, logger_);
		const int workerStartRc = worker->Start(&error);
		if (workerStartRc != 0) {
			logger_->error(
				"push_track_source_worker_start_failed trackId={} senderSsrc={} mode={} error={}",
				track.ids.track_id,
				track.ids.sender_ssrc,
				ToString(sourceWorkerMode),
				error);
			for (auto& startedWorker : sourceWorkers) startedWorker->Stop();
			sdkThread.Stop();
			return workerStartRc;
		}
		sourceWorkers.push_back(std::move(worker));
	}

	int64_t lastSnapshotUs = 0;

	while (running.load()) {
		const int64_t nowUs = MonotonicNowUs();
		if (signaling) signaling->DispatchNotifications();
		if (sdkThread.hasFatalError()) {
			const auto metrics = sdkThread.metrics();
			logger_->error("push_sdk_transport_thread_failed error={}", metrics.fatalError);
			sdkThread.Stop();
			return 4;
		}
		const auto sdkMetricsForControl = sdkThread.metrics();
		bool allSourceWorkersEof = !sourceWorkers.empty() && sourceWorkerMode == PushTrackSourceMode::kCopy;
		for (auto& worker : sourceWorkers) {
			auto workerMetrics = worker->metrics();
			auto adaptation = sdkMetricsForControl.adaptation;
			for (const auto& trackMetrics : sdkMetricsForControl.tracks) {
				if (trackMetrics.trackId == workerMetrics.trackId && trackMetrics.adaptationAvailable) {
					adaptation = trackMetrics.adaptation;
					break;
				}
			}
			worker->StoreEncoderAdaptation(adaptation);
			if (worker->hasFatalError()) {
				workerMetrics = worker->metrics();
				logger_->error(
					"push_track_source_worker_failed trackId={} senderSsrc={} error={}",
					workerMetrics.trackId,
					workerMetrics.senderSsrc,
					workerMetrics.fatalError);
				for (auto& sourceWorker : sourceWorkers) sourceWorker->Stop();
				sdkThread.Stop();
				return 4;
			}
			if (sourceWorkerMode == PushTrackSourceMode::kCopy) {
				allSourceWorkersEof = allSourceWorkersEof && workerMetrics.eof;
			}
		}

		if (allSourceWorkersEof && !options_.loopInput) {
			const auto sdkMetrics = sdkThread.metrics();
			logger_->info("push_input_eof queuedAu={} drainingMs=1000", sdkMetrics.enqueuedAccessUnits);
			const int64_t drainUntilUs = nowUs + 1000000;
			while (running.load() && MonotonicNowUs() < drainUntilUs) {
				if (signaling) signaling->DispatchNotifications();
				if (sdkThread.hasFatalError()) {
					const auto metrics = sdkThread.metrics();
					logger_->error("push_sdk_transport_thread_failed error={}", metrics.fatalError);
					sdkThread.Stop();
					return 4;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(options_.processTickMs));
			}
			break;
		}

		if (nowUs - lastSnapshotUs >= 1000000) {
			lastSnapshotUs = nowUs;
			const auto sdkMetrics = sdkThread.metrics();
			const auto& snapshot = sdkMetrics.snapshot;
			const int64_t sdkHeartbeatAgeMs =
				sdkMetrics.lastHeartbeatUs > 0 ? (nowUs - sdkMetrics.lastHeartbeatUs) / 1000 : -1;
			logger_->info(
				"push_metrics pushedAu={} targetBps={} pacingBps={} finalTargetBps={} rttMs={} loss={} rtcpFeedbackPacketsIn={} rtcpFeedbackBytesIn={} rtcpFeedbackFailures={} maxFps={} requestKeyframe={} droppedFrames={} queuedAu={} sdkQueueDepth={} sdkQueueMaxDepth={} sdkQueueDroppedAu={} sdkStarted={} sdkStopped={} sdkHeartbeatAgeMs={} sdkLoopGapMaxUs={}",
				sdkMetrics.pushedAccessUnits,
				snapshot.sender_rates.googcc_target_bps,
				snapshot.sender_rates.pacing_bps,
				snapshot.sender_rates.final_target_bps,
				snapshot.sender_rates.rtt_ms,
				snapshot.sender_rates.loss_fraction,
				sdkMetrics.rtcpPacketsIn,
				sdkMetrics.rtcpBytesIn,
				sdkMetrics.rtcpFailures,
				sdkMetrics.adaptation.max_fps,
				sdkMetrics.adaptation.request_keyframe,
				snapshot.dropped_frames,
				sdkMetrics.enqueuedAccessUnits,
				sdkMetrics.queueDepth,
				sdkMetrics.queueMaxDepth,
				sdkMetrics.droppedAccessUnits,
				sdkMetrics.started,
				sdkMetrics.stopped,
				sdkHeartbeatAgeMs,
				sdkMetrics.loopGapMaxUs);
			if (sourceWorkerMode != PushTrackSourceMode::kCopy) {
				RealtimeH264SourceMetrics aggregateSourceMetrics;
				for (const auto& worker : sourceWorkers) {
					const auto workerMetrics = worker->metrics();
					const auto& metrics = workerMetrics.sourceMetrics;
					aggregateSourceMetrics.framesGenerated += metrics.framesGenerated;
					aggregateSourceMetrics.framesEncoded += metrics.framesEncoded;
					aggregateSourceMetrics.accessUnits += metrics.accessUnits;
					aggregateSourceMetrics.keyframes += metrics.keyframes;
					aggregateSourceMetrics.encoderRecreates += metrics.encoderRecreates;
					aggregateSourceMetrics.bitrateChanges += metrics.bitrateChanges;
					aggregateSourceMetrics.fpsChanges += metrics.fpsChanges;
					aggregateSourceMetrics.forcedKeyframeRequests += metrics.forcedKeyframeRequests;
					aggregateSourceMetrics.forcedKeyframes += metrics.forcedKeyframes;
					aggregateSourceMetrics.maxForcedKeyframeDelayUs = std::max(
						aggregateSourceMetrics.maxForcedKeyframeDelayUs,
						metrics.maxForcedKeyframeDelayUs);
					aggregateSourceMetrics.currentBitrateBps += metrics.currentBitrateBps;
					aggregateSourceMetrics.currentFps = std::max(aggregateSourceMetrics.currentFps, metrics.currentFps);
					aggregateSourceMetrics.width = std::max(aggregateSourceMetrics.width, metrics.width);
					aggregateSourceMetrics.height = std::max(aggregateSourceMetrics.height, metrics.height);
					aggregateSourceMetrics.lastAccessUnitKeyframe =
						aggregateSourceMetrics.lastAccessUnitKeyframe || metrics.lastAccessUnitKeyframe;
				}
				logger_->info(
					"encoder_metrics mode={} encoder={} currentBitrateBps={} currentFps={} width={} height={} framesGenerated={} framesEncoded={} accessUnits={} keyframes={} encoderRecreates={} bitrateChanges={} fpsChanges={} forcedKeyframeRequests={} forcedKeyframes={} maxForcedKeyframeDelayUs={} lastKeyframe={}",
					sourceMode,
					options_.encoder,
					aggregateSourceMetrics.currentBitrateBps,
					aggregateSourceMetrics.currentFps,
					aggregateSourceMetrics.width,
					aggregateSourceMetrics.height,
					aggregateSourceMetrics.framesGenerated,
					aggregateSourceMetrics.framesEncoded,
					aggregateSourceMetrics.accessUnits,
					aggregateSourceMetrics.keyframes,
					aggregateSourceMetrics.encoderRecreates,
					aggregateSourceMetrics.bitrateChanges,
					aggregateSourceMetrics.fpsChanges,
					aggregateSourceMetrics.forcedKeyframeRequests,
					aggregateSourceMetrics.forcedKeyframes,
					aggregateSourceMetrics.maxForcedKeyframeDelayUs,
					aggregateSourceMetrics.lastAccessUnitKeyframe);
				for (const auto& worker : sourceWorkers) {
					const auto workerMetrics = worker->metrics();
					const auto& metrics = workerMetrics.sourceMetrics;
					logger_->info(
						"encoder_track_metrics mode={} encoder={} trackId={} senderSsrc={} queuedAu={} currentBitrateBps={} currentFps={} width={} height={} framesGenerated={} framesEncoded={} accessUnits={} keyframes={} encoderRecreates={} bitrateChanges={} fpsChanges={} forcedKeyframeRequests={} forcedKeyframes={} maxForcedKeyframeDelayUs={} lastKeyframe={} injectedEncoderDelayCount={} injectedEncoderDelayTotalMs={} workerLoopGapMaxUs={}",
						sourceMode,
						options_.encoder,
						workerMetrics.trackId,
						workerMetrics.senderSsrc,
						workerMetrics.queuedAu,
						metrics.currentBitrateBps,
						metrics.currentFps,
						metrics.width,
						metrics.height,
						metrics.framesGenerated,
						metrics.framesEncoded,
						metrics.accessUnits,
						metrics.keyframes,
						metrics.encoderRecreates,
						metrics.bitrateChanges,
						metrics.fpsChanges,
						metrics.forcedKeyframeRequests,
						metrics.forcedKeyframes,
						metrics.maxForcedKeyframeDelayUs,
						metrics.lastAccessUnitKeyframe,
						workerMetrics.injectedEncoderDelayCount,
						workerMetrics.injectedEncoderDelayTotalMs,
						workerMetrics.loopGapMaxUs);
				}
			}
			for (const auto& trackMetrics : sdkMetrics.tracks) {
				logger_->info(
					"push_track_metrics trackId={} senderSsrc={} queuedAu={} pushedAu={} droppedAu={} queueDepth={} queueMaxDepth={} pushFailures={} adaptationAvailable={} targetBps={} maxFps={} requestKeyframe={} snapshotAvailable={} finalTargetBps={} droppedFrames={}",
					trackMetrics.trackId,
					trackMetrics.senderSsrc,
					trackMetrics.enqueuedAccessUnits,
					trackMetrics.pushedAccessUnits,
					trackMetrics.droppedAccessUnits,
					trackMetrics.queueDepth,
					trackMetrics.queueMaxDepth,
					trackMetrics.pushFailures,
					trackMetrics.adaptationAvailable,
					trackMetrics.adaptation.target_bitrate_bps,
					trackMetrics.adaptation.max_fps,
					trackMetrics.adaptation.request_keyframe,
					trackMetrics.snapshotAvailable,
					trackMetrics.snapshot.sender_rates.final_target_bps,
					trackMetrics.snapshot.dropped_frames);
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(options_.processTickMs));
	}

	for (auto& sourceWorker : sourceWorkers) sourceWorker->Stop();
	sdkThread.Stop();
	const auto finalSdkMetrics = sdkThread.metrics();
	logger_->info(
		"push_runtime_stopped pushedAu={} rtcpFeedbackPacketsIn={} rtcpFeedbackBytesIn={} rtcpFeedbackFailures={} trackCount={} queuedAu={} sdkQueueDroppedAu={} sdkQueueMaxDepth={} sdkStarted={} sdkStopped={} sdkStopReason={} sdkFatalError={}",
		finalSdkMetrics.pushedAccessUnits,
		finalSdkMetrics.rtcpPacketsIn,
		finalSdkMetrics.rtcpBytesIn,
		finalSdkMetrics.rtcpFailures,
		finalSdkMetrics.tracks.size(),
		finalSdkMetrics.enqueuedAccessUnits,
		finalSdkMetrics.droppedAccessUnits,
		finalSdkMetrics.queueMaxDepth,
		finalSdkMetrics.started,
		finalSdkMetrics.stopped,
		finalSdkMetrics.stopReason,
		finalSdkMetrics.fatalError);
	for (const auto& trackMetrics : finalSdkMetrics.tracks) {
		logger_->info(
			"push_track_final trackId={} senderSsrc={} queuedAu={} pushedAu={} droppedAu={} queueMaxDepth={} pushFailures={} adaptationAvailable={} snapshotAvailable={}",
			trackMetrics.trackId,
			trackMetrics.senderSsrc,
			trackMetrics.enqueuedAccessUnits,
			trackMetrics.pushedAccessUnits,
			trackMetrics.droppedAccessUnits,
			trackMetrics.queueMaxDepth,
			trackMetrics.pushFailures,
			trackMetrics.adaptationAvailable,
			trackMetrics.snapshotAvailable);
	}
	return 0;
}

} // namespace webrtc_qos_plain
