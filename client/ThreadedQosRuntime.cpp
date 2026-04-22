#include "ThreadedQosRuntime.h"

#include <chrono>
#include <cmath>
#include <limits>
#include <map>

namespace msff = mediasoup::ffmpeg;

ThreadedQosRuntime::ThreadedQosRuntime(PlainClientApp& app)
	: app_(app)
	, pendingCommands_(app.videoTracks_.size())
	, trackControlStates_(app.videoTracks_.size())
	, trackStats_(app.videoTracks_.size())
{
	setupQueues();
	setupTrackControlState();
}

ThreadedQosRuntime::~ThreadedQosRuntime()
{
	stop();
}

int ThreadedQosRuntime::Run()
{
	std::printf("[threaded] starting multi-thread pipeline (%zu tracks)\n", app_.videoTracks_.size());

	setupNetworkThread();
	startAudioThreadIfNeeded();
	if (netThread_) {
		netThread_->start();
	}
	resolveAudioAvailability();
	startSourceWorkers();
	createQosControllers();

	while (PlainClientApp::running_.load()) {
		app_.ws_.dispatchNotifications();
		processStatsQueue();
		processNetworkAckQueue();
		processWorkerAckQueues();
		maybeRunSampling();
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	stop();
	std::printf("[threaded] pipeline stopped\n");
	return 0;
}

void ThreadedQosRuntime::setupQueues()
{
	queues_.resize(app_.videoTracks_.size());
	for (auto& queue : queues_) {
		queue = std::make_unique<PerTrackQueues>();
	}
}

void ThreadedQosRuntime::setupTrackControlState()
{
	for (size_t i = 0; i < app_.videoTracks_.size(); ++i) {
		trackControlStates_[i].encBitrate = app_.videoTracks_[i].encBitrate;
		trackControlStates_[i].configuredVideoFps = app_.videoTracks_[i].configuredVideoFps;
		trackControlStates_[i].scaleResolutionDownBy = app_.videoTracks_[i].scaleResolutionDownBy;
		trackControlStates_[i].videoSuppressed = app_.videoTracks_[i].videoSuppressed;
		trackControlStates_[i].configGeneration = 0;
	}
}

void ThreadedQosRuntime::setupNetworkThread()
{
	NetworkThread::Config config;
	config.udpFd = app_.udpFd_;
	config.audioSsrc = app_.audioSsrc_;
	config.audioPt = app_.audioPt_;
	config.audioTransportCcExtensionId = app_.audioTransportCcExtensionId_;
	config.enableTransportController = app_.transportControllerEnabled_;
	config.enableTransportEstimate = app_.transportEstimateEnabled_;

	netThread_ = std::make_unique<NetworkThread>(config);
	netThread_->statsQueue = &statsQueue_;
	netThread_->controlQueue = &networkControlQueue_;
	netThread_->commandAckQueue = &networkAckQueue_;

	for (size_t i = 0; i < app_.videoTracks_.size(); ++i) {
		auto& track = app_.videoTracks_[i];
		netThread_->registerVideoTrack(
			static_cast<uint32_t>(i),
			track.ssrc,
			track.payloadType,
			track.transportCcExtensionId);
		NetworkThread::SourceInput input;
		input.auQueue = &queues_[i]->auQueue;
		input.keyframeQueue = &queues_[i]->netCmdQueue;
		netThread_->addSourceInput(input);
		(void)mt::enqueueTrackTransportConfig(
			static_cast<uint32_t>(i),
			track.ssrc,
			track.payloadType,
			static_cast<uint32_t>(std::max(0, track.encBitrate)),
			track.videoSuppressed,
			networkControlQueue_);
	}

	netThread_->sendComediaProbe();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void ThreadedQosRuntime::startAudioThreadIfNeeded()
{
	const bool hasAudioPrecondition =
		(app_.audIdx_ >= 0 && app_.audioDecoder_.has_value() && app_.audioEncoder_.has_value());
	if (!hasAudioPrecondition || !netThread_) {
		return;
	}

	netThread_->audioAuQueue = &audioAuQueue_;
	audioRunning_ = true;
	audioThread_ = std::thread([this, aSsrc = app_.audioSsrc_, aPt = app_.audioPt_]() {
		std::optional<msff::InputFormat> audioInput;
		std::optional<msff::Decoder> threadAudioDecoder;
		std::optional<msff::Encoder> threadAudioEncoder;
		msff::FramePtr threadAudioFrame;
		auto threadPacket = msff::MakePacket();
		auto t0 = std::chrono::steady_clock::now();
		double firstPts = -1;

		try {
			audioInput = msff::InputFormat::Open(app_.mp4Path_);
			audioInput->FindStreamInfo();
			const int audioIndex = audioInput->FindFirstStreamIndex(AVMEDIA_TYPE_AUDIO);
			if (audioIndex < 0) return;

			auto* params = audioInput->StreamAt(audioIndex)->codecpar;
			threadAudioDecoder = msff::Decoder::OpenFromParameters(params);
			threadAudioEncoder = msff::Encoder::Create(AV_CODEC_ID_OPUS, [](AVCodecContext* ctx) {
				ctx->sample_rate = 48000;
				ctx->channels = 2;
				ctx->channel_layout = AV_CH_LAYOUT_STEREO;
				ctx->bit_rate = 64000;
				ctx->sample_fmt = AV_SAMPLE_FMT_FLT;
			});
			threadAudioFrame = msff::MakeFrame();
			audioActuallyStarted_ = true;

			while (audioRunning_.load() && audioInput->ReadPacket(threadPacket.get())) {
				if (threadPacket->stream_index != audioIndex) {
					msff::PacketUnref(threadPacket.get());
					continue;
				}
				double pts = threadPacket->pts * av_q2d(audioInput->StreamAt(audioIndex)->time_base);
				if (firstPts < 0) firstPts = pts;
				auto target =
					t0 + std::chrono::microseconds(static_cast<int64_t>((pts - firstPts) * 1e6));
				std::this_thread::sleep_until(target);
				if (!audioRunning_.load()) {
					msff::PacketUnref(threadPacket.get());
					break;
				}

				threadAudioDecoder->SendPacket(threadPacket.get());
				while (threadAudioDecoder->ReceiveFrame(threadAudioFrame.get())) {
					threadAudioEncoder->SendFrame(threadAudioFrame.get());
					auto encodedPacket = msff::MakePacket();
					while (threadAudioEncoder->ReceivePacket(encodedPacket.get())) {
						mt::EncodedAccessUnit accessUnit;
						accessUnit.ssrc = aSsrc;
						accessUnit.payloadType = aPt;
						accessUnit.rtpTimestamp = static_cast<uint32_t>(pts * 48000);
						accessUnit.assign(encodedPacket->data, encodedPacket->size);
						audioAuQueue_.tryPush(std::move(accessUnit));
						if (netThread_) {
							netThread_->wakeup();
						}
						msff::PacketUnref(encodedPacket.get());
					}
				}
				msff::PacketUnref(threadPacket.get());
			}
		} catch (const std::exception& e) {
			std::printf("[audio] ffmpeg init/runtime failed: %s\n", e.what());
		}
		std::printf("[audio] thread finished\n");
	});
}

void ThreadedQosRuntime::resolveAudioAvailability()
{
	const bool hasAudioPrecondition =
		(app_.audIdx_ >= 0 && app_.audioDecoder_.has_value() && app_.audioEncoder_.has_value());
	if (hasAudioPrecondition) {
		for (int w = 0; w < 20 && !audioActuallyStarted_.load() && audioRunning_.load(); ++w) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

	hasAudio_ = audioActuallyStarted_.load();
	if (hasAudioPrecondition && !hasAudio_) {
		std::printf("[threaded] WARN: audio thread failed to initialize, treating as no-audio peer\n");
	}
}

void ThreadedQosRuntime::startSourceWorkers()
{
	for (size_t i = 0; i < app_.videoTracks_.size(); ++i) {
		auto& track = app_.videoTracks_[i];
		SourceWorker::Config sourceConfig;
		sourceConfig.trackIndex = static_cast<uint32_t>(i);
		sourceConfig.ssrc = track.ssrc;
		sourceConfig.payloadType = track.payloadType;
		sourceConfig.initialBitrate = track.encBitrate;
		sourceConfig.initialFps = track.configuredVideoFps;
		sourceConfig.scaleResolutionDownBy = track.scaleResolutionDownBy;

		if (i < app_.videoSourcePaths_.size() && app_.videoSourcePaths_[i].rfind("/dev/", 0) == 0) {
			sourceConfig.inputType = SourceWorker::InputType::V4L2Camera;
			sourceConfig.inputPath = app_.videoSourcePaths_[i];
			sourceConfig.captureWidth = 1280;
			sourceConfig.captureHeight = 720;
			sourceConfig.captureFps = sourceConfig.initialFps;
			std::printf("[threaded] track %zu → camera %s\n", i, sourceConfig.inputPath.c_str());
		} else {
			sourceConfig.inputType = SourceWorker::InputType::File;
			sourceConfig.inputPath =
				(i < app_.videoSourcePaths_.size()) ? app_.videoSourcePaths_[i] : app_.mp4Path_;
			std::printf("[threaded] track %zu → file %s\n", i, sourceConfig.inputPath.c_str());
		}

		auto worker = std::make_unique<SourceWorker>(sourceConfig);
		worker->outputQueue = &queues_[i]->auQueue;
		worker->controlQueue = &queues_[i]->ctrlQueue;
		worker->networkCmdQueue = &queues_[i]->netCmdQueue;
		worker->ackQueue = &queues_[i]->ackQueue;
		worker->networkWakeupFn = [this]() {
			if (netThread_) {
				netThread_->wakeup();
			}
		};
		worker->start();
		workers_.push_back(std::move(worker));
	}
}

void ThreadedQosRuntime::createQosControllers()
{
	for (size_t i = 0; i < app_.videoTracks_.size(); ++i) {
		auto& track = app_.videoTracks_[i];
		auto* trackRuntime = &track;

		auto* controlQueue = &queues_[i]->ctrlQueue;
		auto* trackControl = &trackControlStates_[i];
		qos::PublisherQosController::Options qosOptions;
		qosOptions.source = qos::Source::Camera;
		qosOptions.trackId = track.trackId;
		qosOptions.producerId = track.producerId;
		qosOptions.initialLevel = 0;
		qosOptions.peerHasAudioTrack = hasAudio_;
		qosOptions.actionSink =
			[this, controlQueue, ssrc = track.ssrc, payloadType = track.payloadType,
				idx = static_cast<uint32_t>(i), trackControl](const qos::PlannedAction& action) -> bool {
				if (idx >= pendingCommands_.size()) return false;
				const bool queued =
					(action.type == qos::ActionType::SetEncodingParameters && action.encodingParameters.has_value())
						? mt::enqueueTrackAction(
							action,
							idx,
							nextCommandId_,
							*controlQueue,
							pendingCommands_[idx],
							trackControl->configGeneration)
						: mt::enqueueNetworkTrackAction(
							action,
							idx,
							ssrc,
							payloadType,
							nextCommandId_,
							networkControlQueue_,
							pendingCommands_[idx]);
				if (!queued) return false;
				if (netThread_) {
					netThread_->wakeup();
				}
				return false;
			};
		qosOptions.sendSnapshot = [trackRuntime](const nlohmann::json&) {
			trackRuntime->snapshotRequested = true;
		};
		track.qosCtrl = std::make_unique<qos::PublisherQosController>(qosOptions);
		trackControlStates_[i].qosCtrl = track.qosCtrl.get();
	}
}

void ThreadedQosRuntime::processStatsQueue()
{
	mt::SenderStatsSnapshot senderSnapshot;
	while (statsQueue_.tryPop(senderSnapshot)) {
		if (senderSnapshot.trackIndex < app_.videoTracks_.size()) {
			trackStats_[senderSnapshot.trackIndex].latest = senderSnapshot;
			trackStats_[senderSnapshot.trackIndex].hasData = true;
		}
	}
}

void ThreadedQosRuntime::processNetworkAckQueue()
{
	mt::CommandAck ack;
	while (networkAckQueue_.tryPop(ack)) {
		applyAck(ack, false);
	}
}

void ThreadedQosRuntime::processWorkerAckQueues()
{
	for (auto& queue : queues_) {
		mt::CommandAck ack;
		while (queue->ackQueue.tryPop(ack)) {
			applyAck(ack, true);
		}
	}
}

void ThreadedQosRuntime::applyAck(const mt::CommandAck& ack, bool logRejectReason)
{
	if (ack.trackIndex >= app_.videoTracks_.size()) return;

	auto& track = app_.videoTracks_[ack.trackIndex];
	auto& pending = pendingCommands_[ack.trackIndex];
	auto& trackState = trackControlStates_[ack.trackIndex];
	if (!mt::applyCommandAck(ack, pending, trackState, trackStats_[ack.trackIndex])) {
		return;
	}

	track.encBitrate = trackState.encBitrate;
	track.configuredVideoFps = trackState.configuredVideoFps;
	track.scaleResolutionDownBy = trackState.scaleResolutionDownBy;
	track.videoSuppressed = trackState.videoSuppressed;
	syncTransportHint(ack.trackIndex);

	if (logRejectReason && !ack.applied && !ack.reason.empty()) {
		std::printf("[QoS:%s] command %llu rejected: %s\n",
			track.trackId.c_str(),
			static_cast<unsigned long long>(ack.commandId),
			ack.reason.c_str());
	}
}

void ThreadedQosRuntime::syncTransportHint(size_t index)
{
	if (index >= app_.videoTracks_.size()) return;

	auto& track = app_.videoTracks_[index];
	(void)mt::enqueueTrackTransportConfig(
		static_cast<uint32_t>(index),
		track.ssrc,
		track.payloadType,
		static_cast<uint32_t>(std::max(0, track.encBitrate)),
		track.videoSuppressed,
		networkControlQueue_);
	if (netThread_) {
		netThread_->wakeup();
	}
}

void ThreadedQosRuntime::maybeRunSampling()
{
	bool hasVideoQos = false;
	int sampleIntervalMs = std::numeric_limits<int>::max();
	for (const auto& track : app_.videoTracks_) {
		if (!track.qosCtrl) continue;
		hasVideoQos = true;
		sampleIntervalMs = std::min(
			sampleIntervalMs,
			track.qosCtrl->getRuntimeSettings().sampleIntervalMs);
	}
	if (!hasVideoQos || sampleIntervalMs == std::numeric_limits<int>::max()) {
		return;
	}

	const auto nowMs = steadyNowMs();
	if (nowMs - lastPeerQosSampleMs_ < sampleIntervalMs) {
		return;
	}
	lastPeerQosSampleMs_ = nowMs;

	std::set<std::string> sampledTrackIds;
	sampleTracks(nowMs, sampledTrackIds);
	applyPeerCoordinationAndMaybeUpload(sampledTrackIds);
}

void ThreadedQosRuntime::sampleTracks(int64_t nowMs, std::set<std::string>& sampledTrackIds)
{
	for (size_t ti = 0; ti < app_.videoTracks_.size(); ++ti) {
		auto& track = app_.videoTracks_[ti];
		if (!track.qosCtrl) continue;

		auto snapshot = makeCanonicalTransportSnapshotBase(
			trackStats_[ti].hasData ? trackStats_[ti].latest.timestampMs : nowMs,
			track.trackId,
			track.producerId,
			qos::Source::Camera,
			qos::TrackKind::Video,
			track.encBitrate,
			track.encBitrate);

		bool statsFresh = trackStats_[ti].hasData &&
			trackStats_[ti].latest.generation > trackStats_[ti].lastConsumedGeneration;
		if (!mt::shouldSampleTrack(statsFresh)) {
			continue;
		}

		const auto probeSuppression = mt::applyProbeSampleSuppression(trackStats_[ti], statsFresh);
		if (!probeSuppression.statsFresh) {
			continue;
		}

		trackStats_[ti].lastConsumedGeneration = trackStats_[ti].latest.generation;
		populateCanonicalTransportSnapshotFromThreadedStats(
			snapshot,
			trackStats_[ti].latest,
			trackStats_[ti].actualWidth,
			trackStats_[ti].actualHeight,
			track.configuredVideoFps);

		track.qosCtrl->onSample(snapshot);
		double observedBitrateBps = 0.0;
		if (auto signals = track.qosCtrl->lastSignals()) {
			observedBitrateBps = signals->sendBitrateBps;
		}
		logTrackSample(track, ti, snapshot, observedBitrateBps);
		sampledTrackIds.insert(track.trackId);
	}
}

void ThreadedQosRuntime::applyPeerCoordinationAndMaybeUpload(
	const std::set<std::string>& sampledTrackIds)
{
	std::vector<qos::PeerTrackState> peerTrackStates;
	std::map<std::string, qos::DerivedSignals> peerTrackSignals;
	std::map<std::string, qos::PlannedAction> peerLastActions;
	std::map<std::string, qos::CanonicalTransportSnapshot> peerRawSnapshots;
	std::map<std::string, bool> peerActionApplied;
	std::vector<qos::TrackBudgetRequest> trackBudgetRequests;
	uint64_t totalDesiredBudgetBps = 0;
	uint64_t totalObservedBudgetBps = 0;
	bool anyBandwidthLimited = false;
	bool peerSnapshotRequested = false;

	for (auto& track : app_.videoTracks_) {
		if (!track.qosCtrl) continue;
		if (sampledTrackIds.find(track.trackId) == sampledTrackIds.end()) continue;

		peerTrackStates.push_back(track.qosCtrl->getTrackState());
		if (auto signals = track.qosCtrl->lastSignals()) {
			peerTrackSignals[track.trackId] = *signals;
		}
		peerLastActions[track.trackId] = track.qosCtrl->lastAction();
		peerActionApplied[track.trackId] = track.qosCtrl->lastActionWasApplied();
		peerSnapshotRequested = peerSnapshotRequested || track.snapshotRequested;
		track.snapshotRequested = false;

		auto rawSnapshot = makeCanonicalTransportSnapshotBase(
			steadyNowMs(),
			track.trackId,
			track.producerId,
			qos::Source::Camera,
			qos::TrackKind::Video,
			track.encBitrate,
			track.encBitrate);
		const size_t trackIndex = track.index;
		if (trackIndex < trackStats_.size() && trackStats_[trackIndex].hasData) {
			populateCanonicalTransportSnapshotFromThreadedStats(
				rawSnapshot,
				trackStats_[trackIndex].latest,
				trackStats_[trackIndex].actualWidth,
				trackStats_[trackIndex].actualHeight,
				track.configuredVideoFps);
		}
		peerRawSnapshots[track.trackId] = rawSnapshot;

		const uint32_t desiredBitrateBps = PlainClientApp::GetTrackDesiredBitrateBps(track);
		totalDesiredBudgetBps += desiredBitrateBps;
		if (auto signals = track.qosCtrl->lastSignals()) {
			if (signals->bandwidthLimited) anyBandwidthLimited = true;
			totalObservedBudgetBps += signals->sendBitrateBps > 0
				? static_cast<uint64_t>(std::llround(signals->sendBitrateBps))
				: desiredBitrateBps;
		} else {
			totalObservedBudgetBps += desiredBitrateBps;
		}
		trackBudgetRequests.push_back({
			track.trackId,
			qos::Source::Camera,
			qos::TrackKind::Video,
			PlainClientApp::GetTrackMinBitrateBps(track),
			desiredBitrateBps,
			desiredBitrateBps,
			track.weight,
			false
		});
	}

	if (trackBudgetRequests.size() > 1) {
		uint32_t totalBudgetBps = static_cast<uint32_t>(
			std::min<uint64_t>(std::numeric_limits<uint32_t>::max(), totalDesiredBudgetBps));
		if (anyBandwidthLimited && totalObservedBudgetBps > 0) {
			totalBudgetBps = std::min<uint32_t>(
				totalBudgetBps,
				static_cast<uint32_t>(std::min<uint64_t>(
					std::numeric_limits<uint32_t>::max(),
					totalObservedBudgetBps)));
		}
		auto budgetDecision = qos::allocatePeerTrackBudgets(totalBudgetBps, trackBudgetRequests);
		auto overrides = qos::buildCoordinationOverrides(peerTrackStates, budgetDecision);
		for (auto& track : app_.videoTracks_) {
			if (!track.qosCtrl) continue;
			auto it = overrides.find(track.trackId);
			track.qosCtrl->setCoordinationOverride(
				it != overrides.end()
					? it->second
					: std::optional<qos::CoordinationOverride>{});
		}
	} else {
		clearCoordinationOverrides();
	}

	if (peerSnapshotRequested && !peerTrackStates.empty()) {
		publishPeerSnapshot(
			peerTrackStates,
			peerTrackSignals,
			peerLastActions,
			peerRawSnapshots,
			peerActionApplied);
	}
}

void ThreadedQosRuntime::clearCoordinationOverrides()
{
	for (auto& track : app_.videoTracks_) {
		if (track.qosCtrl) {
			track.qosCtrl->setCoordinationOverride(std::nullopt);
		}
	}
}

void ThreadedQosRuntime::publishPeerSnapshot(
	const std::vector<qos::PeerTrackState>& peerTrackStates,
	const std::map<std::string, qos::DerivedSignals>& peerTrackSignals,
	const std::map<std::string, qos::PlannedAction>& peerLastActions,
	const std::map<std::string, qos::CanonicalTransportSnapshot>& peerRawSnapshots,
	const std::map<std::string, bool>& peerActionApplied)
{
	if (app_.ws_.pendingRequestCount() >= 8) {
		return;
	}

	auto peerDecision = qos::buildPeerDecision(peerTrackStates);
	auto snapshot = qos::serializeSnapshot(
		peerSnapshotSeq_++,
		wallNowMs(),
		peerDecision.peerQuality,
		false,
		peerTrackStates,
		peerTrackSignals,
		peerLastActions,
		peerRawSnapshots,
		peerActionApplied,
		hasAudio_);
	app_.ws_.requestAsync("clientStats", snapshot,
		[](bool ok, const json&, const std::string& err) {
			if (!ok) std::printf("[QoS] clientStats failed: %s\n", err.c_str());
		});
}

void ThreadedQosRuntime::logTrackSample(
	const PlainClientApp::VideoTrackRuntime& track,
	size_t trackIndex,
	const qos::CanonicalTransportSnapshot& snapshot,
	double observedBitrateBps) const
{
	const double traceLossRate = track.qosCtrl && track.qosCtrl->lastSignals().has_value()
		? track.qosCtrl->lastSignals()->lossRate
		: 0.0;
	const bool traceBandwidthLimited = track.qosCtrl && track.qosCtrl->lastSignals().has_value()
		? track.qosCtrl->lastSignals()->bandwidthLimited
		: false;
	const char* traceSenderPressure = qos::senderPressureStateStr(snapshot.senderPressureState);
	const auto& latestStats = trackStats_[trackIndex].latest;
	std::printf("[QOS_TRACE] tsMs=%lld track=%s state=%s level=%d mode=%s sample=%s bitrateBps=%d sendBps=%.0f lossRate=%.6f packetsLost=%llu rttMs=%.1f jitterMs=%.1f senderUsageBps=%u transportEstimateBps=%u effectivePacingBps=%u feedbackReports=%llu probePackets=%u probeActive=%d probeClusterStarts=%llu probeClusterCompletes=%llu probeClusterEarlyStops=%llu probeBytesSent=%llu wouldBlockTotal=%llu queuedVideoRetentions=%llu audioDeadlineDrops=%llu retransmissionDrops=%llu retransmissionSent=%llu queuedFreshVideoPackets=%u queuedAudioPackets=%u queuedRetransmissionPackets=%u senderPressure=%s bandwidthLimited=%d width=%d height=%d fps=%d suppressed=%d\n",
		static_cast<long long>(wallNowMs()),
		track.trackId.c_str(),
		qos::stateStr(track.qosCtrl->currentState()),
		track.qosCtrl->currentLevel(),
		track.videoSuppressed ? "audio-only" : "audio-video",
		"local",
		track.encBitrate,
		observedBitrateBps > 0.0 ? observedBitrateBps : snapshot.targetBitrateBps,
		traceLossRate,
		static_cast<unsigned long long>(snapshot.packetsLost),
		snapshot.roundTripTimeMs,
		snapshot.jitterMs,
		latestStats.senderUsageBitrateBps,
		latestStats.transportEstimatedBitrateBps,
		latestStats.effectivePacingBitrateBps,
		static_cast<unsigned long long>(latestStats.transportCcFeedbackReports),
		latestStats.probePacketCount,
		latestStats.probeActive ? 1 : 0,
		static_cast<unsigned long long>(latestStats.probeClusterStartCount),
		static_cast<unsigned long long>(latestStats.probeClusterCompleteCount),
		static_cast<unsigned long long>(latestStats.probeClusterEarlyStopCount),
		static_cast<unsigned long long>(latestStats.probeBytesSent),
		static_cast<unsigned long long>(latestStats.transportWouldBlockTotal),
		static_cast<unsigned long long>(latestStats.queuedVideoRetentions),
		static_cast<unsigned long long>(latestStats.audioDeadlineDrops),
		static_cast<unsigned long long>(latestStats.retransmissionDrops),
		static_cast<unsigned long long>(latestStats.retransmissionSent),
		latestStats.queuedFreshVideoPackets,
		latestStats.queuedAudioPackets,
		latestStats.queuedRetransmissionPackets,
		traceSenderPressure,
		traceBandwidthLimited ? 1 : 0,
		snapshot.frameWidth,
		snapshot.frameHeight,
		static_cast<int>(snapshot.framesPerSecond),
		track.videoSuppressed ? 1 : 0);
}

void ThreadedQosRuntime::stop()
{
	if (stopped_) return;
	stopped_ = true;

	for (auto& worker : workers_) {
		worker->stop();
	}
	audioRunning_ = false;
	if (audioThread_.joinable()) {
		audioThread_.join();
	}
	if (netThread_) {
		netThread_->stop();
	}
	for (auto& track : app_.videoTracks_) {
		track.qosCtrl.reset();
		track.snapshotRequested = false;
	}
}
