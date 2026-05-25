#include "play/WebRtcQosPlayRuntime.h"

#include <chrono>
#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <thread>
#include <utility>

#include "common/ClientIds.h"
#include "common/RuntimeLogHelpers.h"
#include "play/DecodedAuSinkWorker.h"
#include "play/PlaySdkTransportThread.h"

namespace webrtc_qos_plain {
namespace {

struct SinkWorkerEntry {
	uint32_t trackId{0};
	uint32_t senderSsrc{0};
	std::string name;
	std::unique_ptr<DecodedAuSinkWorker> worker;
};

DecodedAuSinkWorkerMetrics AggregateSinkMetrics(const std::vector<SinkWorkerEntry>& entries)
{
	DecodedAuSinkWorkerMetrics out;
	out.started = !entries.empty();
	out.stopped = !entries.empty();
	out.stopReason = entries.empty() ? "no_workers" : "queue_closed";
	for (const auto& entry : entries) {
		if (!entry.worker) continue;
		const auto metrics = entry.worker->metrics();
		out.started = out.started && metrics.started;
		out.stopped = out.stopped && metrics.stopped;
		if (metrics.lastHeartbeatUs > out.lastHeartbeatUs) out.lastHeartbeatUs = metrics.lastHeartbeatUs;
		if (metrics.loopGapMaxUs > out.loopGapMaxUs) out.loopGapMaxUs = metrics.loopGapMaxUs;
		out.loopIterations += metrics.loopIterations;
		out.enqueuedAccessUnits += metrics.enqueuedAccessUnits;
		out.droppedAccessUnits += metrics.droppedAccessUnits;
		out.writtenAccessUnits += metrics.writtenAccessUnits;
		out.sinkWriteFailures += metrics.sinkWriteFailures;
		out.injectedSinkDelayCount += metrics.injectedSinkDelayCount;
		out.injectedSinkDelayTotalMs += metrics.injectedSinkDelayTotalMs;
		out.queueDepth += metrics.queueDepth;
		if (metrics.queueMaxDepth > out.queueMaxDepth) out.queueMaxDepth = metrics.queueMaxDepth;
		if (!metrics.stopReason.empty() && metrics.stopReason != "queue_closed") out.stopReason = metrics.stopReason;
		for (const auto& item : metrics.writtenAccessUnitsByTrack) {
			out.writtenAccessUnitsByTrack[item.first] += item.second;
		}
		for (const auto& item : metrics.enqueuedAccessUnitsByTrack) {
			out.enqueuedAccessUnitsByTrack[item.first] += item.second;
		}
		out.qoe.enabled = out.qoe.enabled || metrics.qoe.enabled;
		out.qoe.accessUnitsIn += metrics.qoe.accessUnitsIn;
		out.qoe.keyframesIn += metrics.qoe.keyframesIn;
		out.qoe.decodedFrames += metrics.qoe.decodedFrames;
		out.qoe.decodeErrors += metrics.qoe.decodeErrors;
		out.qoe.freezeCount += metrics.qoe.freezeCount;
		if (out.qoe.firstFrameDelayUs < 0 ||
			(metrics.qoe.firstFrameDelayUs >= 0 && metrics.qoe.firstFrameDelayUs < out.qoe.firstFrameDelayUs)) {
			out.qoe.firstFrameDelayUs = metrics.qoe.firstFrameDelayUs;
		}
		if (metrics.qoe.maxFrameGapUs > out.qoe.maxFrameGapUs) out.qoe.maxFrameGapUs = metrics.qoe.maxFrameGapUs;
		out.qoe.outputFps += metrics.qoe.outputFps;
		if (out.qoe.width == 0 && metrics.qoe.width > 0) out.qoe.width = metrics.qoe.width;
		if (out.qoe.height == 0 && metrics.qoe.height > 0) out.qoe.height = metrics.qoe.height;
	}
	return out;
}

void StopSinkWorkers(std::vector<SinkWorkerEntry>& entries)
{
	for (auto& entry : entries) {
		if (entry.worker) entry.worker->Stop();
	}
}

} // namespace

WebRtcQosPlayRuntime::WebRtcQosPlayRuntime(
	PlayOptions options,
	ConsumerInfo consumerInfo,
	std::shared_ptr<spdlog::logger> logger,
	PlainUdpTransport udp)
	: options_(std::move(options)),
	  consumerInfos_({std::move(consumerInfo)}),
	  logger_(std::move(logger)),
	  udp_(std::move(udp))
{
}

WebRtcQosPlayRuntime::WebRtcQosPlayRuntime(
	PlayOptions options,
	std::vector<ConsumerInfo> consumerInfos,
	std::shared_ptr<spdlog::logger> logger,
	PlainUdpTransport udp)
	: options_(std::move(options)),
	  consumerInfos_(std::move(consumerInfos)),
	  logger_(std::move(logger)),
	  udp_(std::move(udp))
{
}

int WebRtcQosPlayRuntime::Run(std::atomic<bool>& running, PlaySignalingSession* signaling)
{
	std::string error;
	if (consumerInfos_.empty()) {
		logger_->error("play_runtime_start_failed reason=no_consumers");
		return 3;
	}
	const auto& firstConsumer = consumerInfos_.front();
	VideoSessionParams sessionParams;
	sessionParams.roomId = options_.room;
	sessionParams.transportId = firstConsumer.transportId;
	sessionParams.sourceId = firstConsumer.peerId.empty() ? firstConsumer.producerId : firstConsumer.peerId;
	sessionParams.receiverId = options_.peer;
	sessionParams.receiverIdOverride = options_.receiverId;
	sessionParams.startBitrateBps = options_.startBitrateBps;
	sessionParams.minBitrateBps = options_.minBitrateBps;
	sessionParams.maxBitrateBps = options_.maxBitrateBps;
	sessionParams.debugName = "mediasoup_plain_play:" + options_.room + ":" + options_.peer;
	for (size_t index = 0; index < consumerInfos_.size(); ++index) {
		const auto& consumer = consumerInfos_[index];
		VideoTrackSessionParams track;
		track.trackIdString = consumer.consumerId.empty()
			? ("consumer" + std::to_string(index))
			: consumer.consumerId;
		track.trackId = static_cast<uint32_t>(index + 1);
		track.senderSsrc = consumer.ssrc;
		track.payloadType = consumer.payloadType;
		track.transportCcExtId = consumer.transportCcExtId;
		track.weight = 100;
		track.baseTrack = index == 0;
		sessionParams.tracks.push_back(track);
	}
	auto session = MakeVideoSessionConfig(sessionParams);

	std::vector<SinkWorkerEntry> sinkWorkers;
	std::unordered_map<uint32_t, DecodedAuSinkWorker*> sinkWorkersByTrack;
	for (const auto& track : session.video_tracks) {
		if (!track.enabled) continue;
		SinkWorkerEntry entry;
		entry.trackId = track.ids.track_id;
		entry.senderSsrc = track.ids.sender_ssrc;
		entry.name = "track" + std::to_string(track.ids.track_id);
		entry.worker = std::make_unique<DecodedAuSinkWorker>(
			options_.outputNull,
			options_.outputAu,
			options_.decodeQoe,
			logger_,
			64,
			options_.injectSinkDelayMs,
			entry.trackId,
			entry.senderSsrc,
			entry.name);
		if (!entry.worker->Start(&error)) {
			logger_->error("decoded_sink_worker_start_failed trackId={} senderSsrc={} outputNull={} output={} decodeQoe={} error={}",
				entry.trackId, entry.senderSsrc, options_.outputNull, options_.outputAu, options_.decodeQoe, error);
			StopSinkWorkers(sinkWorkers);
			return 2;
		}
		sinkWorkersByTrack[entry.trackId] = entry.worker.get();
		sinkWorkers.push_back(std::move(entry));
	}
	if (sinkWorkers.empty()) {
		logger_->error("play_runtime_start_failed reason=no_sink_workers");
		return 3;
	}

	PlaySdkTransportThreadConfig sdkThreadConfig;
	sdkThreadConfig.session = session;
	sdkThreadConfig.udp = std::move(udp_);
	sdkThreadConfig.mediaRemoteIp = options_.mediaRemoteIp;
	sdkThreadConfig.mediaRemotePort = firstConsumer.plainTransportPort;
	sdkThreadConfig.logDir = options_.logDir;
	sdkThreadConfig.processTickMs = options_.processTickMs;
	sdkThreadConfig.decodedAccessUnitOutput = [&](const webrtc_qos::AnnexBAccessUnitView& accessUnit) {
		auto it = sinkWorkersByTrack.find(accessUnit.ids.track_id);
		if (it == sinkWorkersByTrack.end() || !it->second) {
			auto status = webrtc_qos::Status::Error(
				webrtc_qos::StatusCode::kInvalidArgument,
				"decoded access unit has unknown track id");
			logger_->warn("decoded_au_enqueue_failed trackId={} status={}",
				accessUnit.ids.track_id, StatusToString(status));
			return status;
		}
		auto status = it->second->Enqueue(accessUnit);
		if (!status) logger_->warn("decoded_au_enqueue_failed status={}", StatusToString(status));
		return status;
	};
	PlaySdkTransportThread sdkThread(std::move(sdkThreadConfig), logger_);
	const int sdkStartRc = sdkThread.Start(&error);
	if (sdkStartRc != 0) {
		logger_->error("play_sdk_transport_thread_start_failed error={}", error);
		StopSinkWorkers(sinkWorkers);
		return sdkStartRc;
	}
	const auto startMetrics = sdkThread.metrics();

	std::ostringstream consumerList;
	for (size_t index = 0; index < consumerInfos_.size(); ++index) {
		if (index != 0) consumerList << ",";
		consumerList << consumerInfos_[index].consumerId << ":" << consumerInfos_[index].ssrc;
	}

	logger_->info(
		"play_runtime_started roomId={} peerId={} producerPeerId={} producerId={} consumerId={} transportId={} videoSsrc={} videoPt={} transportCcExtId={} udpLocalIp={} udpLocalPort={} udpRemoteIp={} udpRemotePort={} trackCount={} consumers={}",
		options_.room,
		options_.peer,
		firstConsumer.peerId,
		firstConsumer.producerId,
		firstConsumer.consumerId,
		firstConsumer.transportId,
		firstConsumer.ssrc,
		firstConsumer.payloadType,
		firstConsumer.transportCcExtId,
		startMetrics.localEndpoint.ip,
		startMetrics.localEndpoint.port,
		startMetrics.remoteEndpoint.ip,
		startMetrics.remoteEndpoint.port,
		consumerInfos_.size(),
		consumerList.str());

	int64_t lastSnapshotUs = 0;

	while (running.load()) {
		const int64_t nowUs = MonotonicNowUs();
		if (signaling) signaling->DispatchNotifications();
		if (sdkThread.hasFatalError()) {
			const auto metrics = sdkThread.metrics();
			logger_->error("play_sdk_transport_thread_failed error={}", metrics.fatalError);
			sdkThread.Stop();
			StopSinkWorkers(sinkWorkers);
			return 4;
		}

		if (nowUs - lastSnapshotUs >= 1000000) {
			lastSnapshotUs = nowUs;
			const auto sdkMetrics = sdkThread.metrics();
			const auto& snapshot = sdkMetrics.snapshot;
			const auto sinkMetrics = AggregateSinkMetrics(sinkWorkers);
			const int64_t sdkHeartbeatAgeMs =
				sdkMetrics.lastHeartbeatUs > 0 ? (nowUs - sdkMetrics.lastHeartbeatUs) / 1000 : -1;
			const int64_t sinkHeartbeatAgeMs =
				sinkMetrics.lastHeartbeatUs > 0 ? (nowUs - sinkMetrics.lastHeartbeatUs) / 1000 : -1;
			logger_->info(
				"play_metrics rtpPackets={} rtcpPackets={} rtcpPacketsOut={} rtcpBytesOut={} rtcpSendFailures={} outputAu={} nack={} pli={} retransmission={} droppedRetransmission={} rttMs={} lossQ8={} sdkStarted={} sdkStopped={} sdkHeartbeatAgeMs={} sdkLoopGapMaxUs={} packetInputFailures={} sinkQueueDepth={} sinkQueueMaxDepth={} sinkQueueDroppedAu={} sinkStarted={} sinkStopped={} sinkHeartbeatAgeMs={} sinkLoopGapMaxUs={} sinkInjectedDelayCount={} sinkInjectedDelayTotalMs={}",
				sdkMetrics.rtpPacketsIn,
				sdkMetrics.rtcpPacketsIn,
				sdkMetrics.rtcpPacketsOut,
				sdkMetrics.rtcpBytesOut,
				sdkMetrics.rtcpSendFailures,
				sinkMetrics.writtenAccessUnits,
				snapshot.nack_count,
				snapshot.pli_count,
				snapshot.retransmission_count,
				snapshot.dropped_retransmission_packets,
				snapshot.downlink_quality.rtt_ms,
				snapshot.downlink_quality.fraction_lost_q8,
				sdkMetrics.started,
				sdkMetrics.stopped,
				sdkHeartbeatAgeMs,
				sdkMetrics.loopGapMaxUs,
				sdkMetrics.packetInputFailures,
				sinkMetrics.queueDepth,
				sinkMetrics.queueMaxDepth,
				sinkMetrics.droppedAccessUnits,
				sinkMetrics.started,
				sinkMetrics.stopped,
				sinkHeartbeatAgeMs,
				sinkMetrics.loopGapMaxUs,
				sinkMetrics.injectedSinkDelayCount,
				sinkMetrics.injectedSinkDelayTotalMs);
			for (const auto& trackSnapshot : sdkMetrics.tracks) {
				const uint64_t trackOutputAu = sinkMetrics.writtenAccessUnitsByTrack.count(trackSnapshot.trackId) > 0
					? sinkMetrics.writtenAccessUnitsByTrack.at(trackSnapshot.trackId)
					: 0;
				const uint64_t trackEnqueuedAu = sinkMetrics.enqueuedAccessUnitsByTrack.count(trackSnapshot.trackId) > 0
					? sinkMetrics.enqueuedAccessUnitsByTrack.at(trackSnapshot.trackId)
					: 0;
				logger_->info(
					"play_track_metrics trackId={} senderSsrc={} snapshotAvailable={} enqueuedAu={} outputAu={} nack={} pli={} droppedFrames={} rttMs={} lossQ8={}",
					trackSnapshot.trackId,
					trackSnapshot.senderSsrc,
					trackSnapshot.snapshotAvailable,
					trackEnqueuedAu,
					trackOutputAu,
					trackSnapshot.snapshot.nack_count,
					trackSnapshot.snapshot.pli_count,
					trackSnapshot.snapshot.dropped_frames,
					trackSnapshot.snapshot.downlink_quality.rtt_ms,
					trackSnapshot.snapshot.downlink_quality.fraction_lost_q8);
			}
			if (options_.decodeQoe) {
				const auto& qoe = sinkMetrics.qoe;
				logger_->info(
					"qoe_metrics enabled={} accessUnitsIn={} keyframesIn={} decodedFrames={} decodeErrors={} freezeCount={} firstFrameDelayUs={} maxFrameGapUs={} outputFps={:.2f} width={} height={}",
					qoe.enabled,
					qoe.accessUnitsIn,
					qoe.keyframesIn,
					qoe.decodedFrames,
					qoe.decodeErrors,
					qoe.freezeCount,
					qoe.firstFrameDelayUs,
					qoe.maxFrameGapUs,
					qoe.outputFps,
					qoe.width,
					qoe.height);
				for (const auto& entry : sinkWorkers) {
					if (!entry.worker) continue;
					const auto trackSinkMetrics = entry.worker->metrics();
					const auto& trackQoe = trackSinkMetrics.qoe;
					logger_->info(
						"play_track_qoe_metrics trackId={} senderSsrc={} enabled={} accessUnitsIn={} keyframesIn={} decodedFrames={} decodeErrors={} freezeCount={} firstFrameDelayUs={} maxFrameGapUs={} outputFps={:.2f} width={} height={}",
						entry.trackId,
						entry.senderSsrc,
						trackQoe.enabled,
						trackQoe.accessUnitsIn,
						trackQoe.keyframesIn,
						trackQoe.decodedFrames,
						trackQoe.decodeErrors,
						trackQoe.freezeCount,
						trackQoe.firstFrameDelayUs,
						trackQoe.maxFrameGapUs,
						trackQoe.outputFps,
						trackQoe.width,
						trackQoe.height);
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(options_.processTickMs));
	}

	sdkThread.Stop();
	StopSinkWorkers(sinkWorkers);
	const auto finalSdkMetrics = sdkThread.metrics();
	const auto finalSinkMetrics = AggregateSinkMetrics(sinkWorkers);
	logger_->info(
		"play_runtime_stopped rtpPackets={} rtcpPackets={} rtcpPacketsOut={} rtcpBytesOut={} rtcpSendFailures={} outputAu={} packetInputFailures={} sdkStarted={} sdkStopped={} sdkStopReason={} sdkFatalError={} sdkLoopIterations={} sdkLoopGapMaxUs={} trackCount={} sinkQueueDroppedAu={} sinkQueueMaxDepth={} sinkStarted={} sinkStopped={} sinkStopReason={} sinkLoopIterations={} sinkLoopGapMaxUs={} sinkInjectedDelayCount={} sinkInjectedDelayTotalMs={}",
		finalSdkMetrics.rtpPacketsIn,
		finalSdkMetrics.rtcpPacketsIn,
		finalSdkMetrics.rtcpPacketsOut,
		finalSdkMetrics.rtcpBytesOut,
		finalSdkMetrics.rtcpSendFailures,
		finalSinkMetrics.writtenAccessUnits,
		finalSdkMetrics.packetInputFailures,
		finalSdkMetrics.started,
		finalSdkMetrics.stopped,
		finalSdkMetrics.stopReason,
		finalSdkMetrics.fatalError,
		finalSdkMetrics.loopIterations,
		finalSdkMetrics.loopGapMaxUs,
		finalSdkMetrics.tracks.size(),
		finalSinkMetrics.droppedAccessUnits,
		finalSinkMetrics.queueMaxDepth,
		finalSinkMetrics.started,
		finalSinkMetrics.stopped,
		finalSinkMetrics.stopReason,
		finalSinkMetrics.loopIterations,
		finalSinkMetrics.loopGapMaxUs,
		finalSinkMetrics.injectedSinkDelayCount,
		finalSinkMetrics.injectedSinkDelayTotalMs);
	for (const auto& trackSnapshot : finalSdkMetrics.tracks) {
		const uint64_t trackOutputAu = finalSinkMetrics.writtenAccessUnitsByTrack.count(trackSnapshot.trackId) > 0
			? finalSinkMetrics.writtenAccessUnitsByTrack.at(trackSnapshot.trackId)
			: 0;
		const uint64_t trackEnqueuedAu = finalSinkMetrics.enqueuedAccessUnitsByTrack.count(trackSnapshot.trackId) > 0
			? finalSinkMetrics.enqueuedAccessUnitsByTrack.at(trackSnapshot.trackId)
			: 0;
		logger_->info(
			"play_track_final trackId={} senderSsrc={} snapshotAvailable={} enqueuedAu={} outputAu={} nack={} pli={} droppedFrames={} rttMs={} lossQ8={}",
			trackSnapshot.trackId,
			trackSnapshot.senderSsrc,
			trackSnapshot.snapshotAvailable,
			trackEnqueuedAu,
			trackOutputAu,
			trackSnapshot.snapshot.nack_count,
			trackSnapshot.snapshot.pli_count,
			trackSnapshot.snapshot.dropped_frames,
			trackSnapshot.snapshot.downlink_quality.rtt_ms,
			trackSnapshot.snapshot.downlink_quality.fraction_lost_q8);
	}
	if (options_.decodeQoe) {
		const auto& qoe = finalSinkMetrics.qoe;
		logger_->info(
			"qoe_runtime_stopped enabled={} accessUnitsIn={} keyframesIn={} decodedFrames={} decodeErrors={} freezeCount={} firstFrameDelayUs={} maxFrameGapUs={} outputFps={:.2f} width={} height={}",
			qoe.enabled,
			qoe.accessUnitsIn,
			qoe.keyframesIn,
			qoe.decodedFrames,
			qoe.decodeErrors,
			qoe.freezeCount,
			qoe.firstFrameDelayUs,
			qoe.maxFrameGapUs,
			qoe.outputFps,
			qoe.width,
			qoe.height);
		for (const auto& entry : sinkWorkers) {
			if (!entry.worker) continue;
			const auto trackSinkMetrics = entry.worker->metrics();
			const auto& trackQoe = trackSinkMetrics.qoe;
			logger_->info(
				"play_track_qoe_final trackId={} senderSsrc={} enabled={} accessUnitsIn={} keyframesIn={} decodedFrames={} decodeErrors={} freezeCount={} firstFrameDelayUs={} maxFrameGapUs={} outputFps={:.2f} width={} height={}",
				entry.trackId,
				entry.senderSsrc,
				trackQoe.enabled,
				trackQoe.accessUnitsIn,
				trackQoe.keyframesIn,
				trackQoe.decodedFrames,
				trackQoe.decodeErrors,
				trackQoe.freezeCount,
				trackQoe.firstFrameDelayUs,
				trackQoe.maxFrameGapUs,
				trackQoe.outputFps,
				trackQoe.width,
				trackQoe.height);
		}
	}
	return 0;
}

} // namespace webrtc_qos_plain
