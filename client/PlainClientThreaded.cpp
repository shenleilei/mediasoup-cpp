#include "PlainClientApp.h"

#include "NetworkThread.h"
#include "SourceWorker.h"
#include "ThreadTypes.h"
#include "ThreadedControlHelpers.h"

#include <limits>
#include <map>
#include <memory>
#include <set>

namespace msff = mediasoup::ffmpeg;

int PlainClientApp::RunThreadedMode()
{
	std::printf("[threaded] starting multi-thread pipeline (%zu tracks)\n", videoTracks_.size());
	rtcpContext_ = nullptr;

	struct PerTrackQueues {
		mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
		mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity> ctrlQueue;
		mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netCmdQueue;
		mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity> ackQueue;
	};
	std::vector<std::unique_ptr<PerTrackQueues>> queues(videoTracks_.size());
	for (auto& queue : queues)
		queue = std::make_unique<PerTrackQueues>();

	mt::SpscQueue<mt::SenderStatsSnapshot, mt::kStatsQueueCapacity> statsQueue;
	mt::SpscQueue<mt::NetworkControlCommand, mt::kControlCommandQueueCapacity> networkControlQueue;
	mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity> networkAckQueue;

	NetworkThread::Config networkConfig;
	networkConfig.udpFd = udpFd_;
	networkConfig.audioSsrc = audioSsrc_;
	networkConfig.audioPt = audioPt_;
	NetworkThread netThread(networkConfig);
	netThread.statsQueue = &statsQueue;
	netThread.controlQueue = &networkControlQueue;
	netThread.commandAckQueue = &networkAckQueue;

	for (size_t i = 0; i < videoTracks_.size(); ++i) {
		netThread.registerVideoTrack(
			static_cast<uint32_t>(i),
			videoTracks_[i].ssrc,
			videoTracks_[i].payloadType);
		NetworkThread::SourceInput input;
		input.auQueue = &queues[i]->auQueue;
		input.keyframeQueue = &queues[i]->netCmdQueue;
		netThread.addSourceInput(input);
	}

	netThread.sendComediaProbe();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> audioAuQueue;
	std::atomic<bool> audioRunning{false};
	std::atomic<bool> audioActuallyStarted{false};
	std::thread audioThread;
	const bool hasAudioPrecondition =
		(audIdx_ >= 0 && audioDecoder_.has_value() && audioEncoder_.has_value());
	if (hasAudioPrecondition) {
		netThread.audioAuQueue = &audioAuQueue;
		audioRunning = true;
		audioThread = std::thread([&, aSsrc = audioSsrc_, aPt = audioPt_]() {
			std::optional<msff::InputFormat> audioInput;
			std::optional<msff::Decoder> threadAudioDecoder;
			std::optional<msff::Encoder> threadAudioEncoder;
			msff::FramePtr threadAudioFrame;
			auto threadPacket = msff::MakePacket();
			auto t0 = std::chrono::steady_clock::now();
			double firstPts = -1;

			try {
				audioInput = msff::InputFormat::Open(mp4Path_);
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
				audioActuallyStarted = true;

				while (audioRunning.load() && audioInput->ReadPacket(threadPacket.get())) {
					if (threadPacket->stream_index != audioIndex) {
						msff::PacketUnref(threadPacket.get());
						continue;
					}
					double pts = threadPacket->pts * av_q2d(audioInput->StreamAt(audioIndex)->time_base);
					if (firstPts < 0) firstPts = pts;
					auto target =
						t0 + std::chrono::microseconds((int64_t)((pts - firstPts) * 1e6));
					std::this_thread::sleep_until(target);
					if (!audioRunning.load()) {
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
							accessUnit.rtpTimestamp = (uint32_t)(pts * 48000);
							accessUnit.assign(encodedPacket->data, encodedPacket->size);
							audioAuQueue.tryPush(std::move(accessUnit));
							netThread.wakeup();
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

	netThread.start();

	if (hasAudioPrecondition) {
		for (int w = 0; w < 20 && !audioActuallyStarted.load() && audioRunning.load(); ++w)
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	const bool hasAudio = audioActuallyStarted.load();
	if (hasAudioPrecondition && !hasAudio)
		std::printf("[threaded] WARN: audio thread failed to initialize, treating as no-audio peer\n");

	std::vector<std::unique_ptr<SourceWorker>> workers;
	for (size_t i = 0; i < videoTracks_.size(); ++i) {
		SourceWorker::Config sourceConfig;
		sourceConfig.trackIndex = static_cast<uint32_t>(i);
		sourceConfig.ssrc = videoTracks_[i].ssrc;
		sourceConfig.payloadType = videoTracks_[i].payloadType;
		sourceConfig.initialBitrate = videoTracks_[i].encBitrate;
		sourceConfig.initialFps = videoTracks_[i].configuredVideoFps;
		sourceConfig.scaleResolutionDownBy = videoTracks_[i].scaleResolutionDownBy;

		if (i < videoSourcePaths_.size() && videoSourcePaths_[i].rfind("/dev/", 0) == 0) {
			sourceConfig.inputType = SourceWorker::InputType::V4L2Camera;
			sourceConfig.inputPath = videoSourcePaths_[i];
			sourceConfig.captureWidth = 1280;
			sourceConfig.captureHeight = 720;
			sourceConfig.captureFps = sourceConfig.initialFps;
			std::printf("[threaded] track %zu → camera %s\n", i, sourceConfig.inputPath.c_str());
		} else {
			sourceConfig.inputType = SourceWorker::InputType::File;
			sourceConfig.inputPath =
				(i < videoSourcePaths_.size()) ? videoSourcePaths_[i] : mp4Path_;
			std::printf("[threaded] track %zu → file %s\n", i, sourceConfig.inputPath.c_str());
		}

		auto worker = std::make_unique<SourceWorker>(sourceConfig);
		worker->outputQueue = &queues[i]->auQueue;
		worker->controlQueue = &queues[i]->ctrlQueue;
		worker->networkCmdQueue = &queues[i]->netCmdQueue;
		worker->ackQueue = &queues[i]->ackQueue;
		worker->networkWakeupFn = [&netThread]() { netThread.wakeup(); };
		worker->start();
		workers.push_back(std::move(worker));
	}

	std::vector<mt::PendingTrackCommand> pendingCommands(videoTracks_.size());
	std::vector<mt::ThreadedTrackControlState> trackControlStates(videoTracks_.size());
	for (size_t i = 0; i < videoTracks_.size(); ++i) {
		trackControlStates[i].encBitrate = videoTracks_[i].encBitrate;
		trackControlStates[i].configuredVideoFps = videoTracks_[i].configuredVideoFps;
		trackControlStates[i].scaleResolutionDownBy = videoTracks_[i].scaleResolutionDownBy;
		trackControlStates[i].videoSuppressed = videoTracks_[i].videoSuppressed;
		trackControlStates[i].configGeneration = 0;
	}
	uint64_t nextCommandId = 1;

	for (size_t i = 0; i < videoTracks_.size(); ++i) {
		auto& track = videoTracks_[i];
		if (copyMode_) continue;
		auto* controlQueue = &queues[i]->ctrlQueue;
		qos::PublisherQosController::Options qosOptions;
		qosOptions.source = qos::Source::Camera;
		qosOptions.trackId = track.trackId;
		qosOptions.producerId = track.producerId;
		qosOptions.initialLevel = 0;
		qosOptions.peerHasAudioTrack = hasAudio;
		auto* trackControl = &trackControlStates[i];
		qosOptions.actionSink = [controlQueue, &networkControlQueue, &netThread, ssrc = track.ssrc,
			payloadType = track.payloadType, idx = static_cast<uint32_t>(i),
			&nextCommandId, &pendingCommands, trackControl](const qos::PlannedAction& action) -> bool {
			if (idx >= pendingCommands.size()) return false;
			const bool queued =
				(action.type == qos::ActionType::SetEncodingParameters && action.encodingParameters.has_value())
					? mt::enqueueTrackAction(
						action,
						idx,
						nextCommandId,
						*controlQueue,
						pendingCommands[idx],
						trackControl->configGeneration)
					: mt::enqueueNetworkTrackAction(
						action,
						idx,
						ssrc,
						payloadType,
						nextCommandId,
						networkControlQueue,
						pendingCommands[idx]);
			if (!queued) return false;
			netThread.wakeup();
			return false;
		};
		qosOptions.sendSnapshot = [&track](const nlohmann::json&) {
			track.snapshotRequested = true;
		};
		track.qosCtrl = std::make_unique<qos::PublisherQosController>(qosOptions);
		trackControlStates[i].qosCtrl = track.qosCtrl.get();
	}

	std::vector<mt::ThreadedTrackStatsState> trackStats(videoTracks_.size());
	int64_t lastPeerQosSampleMs = 0;

	while (running_.load()) {
		ws_.dispatchNotifications();

		mt::SenderStatsSnapshot senderSnapshot;
		while (statsQueue.tryPop(senderSnapshot)) {
			if (senderSnapshot.trackIndex < videoTracks_.size()) {
				trackStats[senderSnapshot.trackIndex].latest = senderSnapshot;
				trackStats[senderSnapshot.trackIndex].hasData = true;
			}
		}

		mt::CommandAck networkAck;
		while (networkAckQueue.tryPop(networkAck)) {
			if (networkAck.trackIndex >= videoTracks_.size()) continue;
			auto& track = videoTracks_[networkAck.trackIndex];
			auto& pending = pendingCommands[networkAck.trackIndex];
			auto& trackState = trackControlStates[networkAck.trackIndex];
			if (!mt::applyCommandAck(networkAck, pending, trackState, trackStats[networkAck.trackIndex]))
				continue;
			track.encBitrate = trackState.encBitrate;
			track.configuredVideoFps = trackState.configuredVideoFps;
			track.scaleResolutionDownBy = trackState.scaleResolutionDownBy;
			track.videoSuppressed = trackState.videoSuppressed;
		}

		for (size_t i = 0; i < queues.size(); ++i) {
			mt::CommandAck ack;
			while (queues[i]->ackQueue.tryPop(ack)) {
				if (ack.trackIndex >= videoTracks_.size()) continue;
				auto& track = videoTracks_[ack.trackIndex];
				auto& pending = pendingCommands[ack.trackIndex];
				auto& trackState = trackControlStates[ack.trackIndex];
				if (!mt::applyCommandAck(ack, pending, trackState, trackStats[ack.trackIndex]))
					continue;
				track.encBitrate = trackState.encBitrate;
				track.configuredVideoFps = trackState.configuredVideoFps;
				track.scaleResolutionDownBy = trackState.scaleResolutionDownBy;
				track.videoSuppressed = trackState.videoSuppressed;
				if (!ack.applied && !ack.reason.empty()) {
					std::printf("[QoS:%s] command %llu rejected: %s\n",
						track.trackId.c_str(),
						(unsigned long long)ack.commandId,
						ack.reason.c_str());
				}
			}
		}

		bool hasVideoQos = false;
		int sampleIntervalMs = std::numeric_limits<int>::max();
		for (const auto& track : videoTracks_) {
			if (!track.qosCtrl) continue;
			hasVideoQos = true;
			sampleIntervalMs = std::min(
				sampleIntervalMs,
				track.qosCtrl->getRuntimeSettings().sampleIntervalMs);
		}

		if (hasVideoQos && sampleIntervalMs < std::numeric_limits<int>::max()) {
			auto now = steadyNowMs();
			if (now - lastPeerQosSampleMs >= sampleIntervalMs) {
				lastPeerQosSampleMs = now;
				RequestServerProducerStats();

				std::optional<json> cachedPeerStats;
				{
					std::lock_guard<std::mutex> lock(cachedServerStatsResponse_->mutex);
					const int64_t maxStatsAgeMs =
						std::max<int64_t>(5000, static_cast<int64_t>(sampleIntervalMs) * 4);
					if (cachedServerStatsResponse_->latestPeerStats.has_value() &&
						now - cachedServerStatsResponse_->updatedAtSteadyMs <= maxStatsAgeMs) {
						cachedPeerStats = *cachedServerStatsResponse_->latestPeerStats;
					}
				}

				std::set<std::string> sampledTrackIds;
				for (size_t ti = 0; ti < videoTracks_.size(); ++ti) {
					auto& track = videoTracks_[ti];
					if (!track.qosCtrl) continue;

					qos::RawSenderSnapshot qSnap;
					qSnap.timestampMs = trackStats[ti].hasData ? trackStats[ti].latest.timestampMs : now;
					qSnap.trackId = track.trackId;
					qSnap.producerId = track.producerId;
					qSnap.source = qos::Source::Camera;
					qSnap.kind = qos::TrackKind::Video;
					qSnap.targetBitrateBps = track.encBitrate;
					qSnap.configuredBitrateBps = track.encBitrate;
					LossCounterSource lossCounterSource = LossCounterSource::None;

					bool statsFresh = trackStats[ti].hasData &&
						trackStats[ti].latest.generation > trackStats[ti].lastConsumedGeneration;

					std::optional<ServerProducerStats> serverStats;
					if (cachedPeerStats.has_value() && !track.producerId.empty())
						serverStats = parseServerProducerStats(*cachedPeerStats, track.producerId, "video");
					if (forcedStaleTrackIndex_.has_value() && ti == *forcedStaleTrackIndex_) {
						statsFresh = false;
						serverStats = std::nullopt;
					}

					if (!mt::shouldSampleTrack(statsFresh, serverStats.has_value()))
						continue;

					if (statsFresh) {
						trackStats[ti].lastConsumedGeneration = trackStats[ti].latest.generation;
						auto& ns = trackStats[ti].latest;
						qSnap.bytesSent = ns.octetCount;
						qSnap.packetsSent = ns.packetCount;
						qSnap.packetsLost = ns.rrCumulativeLost;
						qSnap.roundTripTimeMs = ns.rrRttMs;
						qSnap.jitterMs = ns.rrJitterMs;
						lossCounterSource = LossCounterSource::LocalRtcp;
					}

					double observedBitrateBps = 0.0;
					bool usingMatrixProfile = false;
					if (serverStats.has_value()) {
						auto parsed = *serverStats;
						if (!track.lossBaseInitialized) {
							track.lossBase = parsed.packetsLost;
							track.lossBaseInitialized = true;
						}
						if (parsed.packetsLost >= track.lossBase)
							parsed.packetsLost -= track.lossBase;
						else {
							track.lossBase = parsed.packetsLost;
							parsed.packetsLost = 0;
						}
						if (parsed.byteCount > 0) qSnap.bytesSent = parsed.byteCount;
						if (parsed.packetCount > 0) qSnap.packetsSent = parsed.packetCount;
						qSnap.packetsLost = parsed.packetsLost;
						if (parsed.roundTripTimeMs >= 0) qSnap.roundTripTimeMs = parsed.roundTripTimeMs;
						if (parsed.jitterMs >= 0) qSnap.jitterMs = parsed.jitterMs;
						observedBitrateBps = parsed.bitrateBps;
						lossCounterSource = LossCounterSource::ServerStats;
					}

					if (matrixTestProfile_.has_value() && track.index == 0) {
						if (auto syntheticSendBps = applyMatrixTestProfile(
								qSnap, track.encBitrate, *matrixTestProfile_, matrixTestRuntime_, now)) {
							observedBitrateBps = *syntheticSendBps;
							usingMatrixProfile = true;
						}
					}

					if (trackStats[ti].actualWidth > 0) {
						qSnap.frameWidth = trackStats[ti].actualWidth;
						qSnap.frameHeight = trackStats[ti].actualHeight;
					}
					qSnap.framesPerSecond = track.configuredVideoFps;
					if (observedBitrateBps > 0.0 &&
						qSnap.targetBitrateBps > 0 &&
						observedBitrateBps < qSnap.targetBitrateBps * qos::NETWORK_CONGESTED_UTILIZATION) {
						qSnap.qualityLimitationReason = qos::QualityLimitationReason::Bandwidth;
					}

					if (track.lossCounterSource != LossCounterSource::None &&
						lossCounterSource != LossCounterSource::None &&
						track.lossCounterSource != lossCounterSource) {
						track.qosCtrl->primeSnapshotBaseline(qSnap);
					}
					if (lossCounterSource != LossCounterSource::None)
						track.lossCounterSource = lossCounterSource;

					track.qosCtrl->onSample(qSnap);
					std::printf("[QOS_TRACE] tsMs=%lld track=%s state=%s level=%d mode=%s sample=%s bitrateBps=%d sendBps=%.0f packetsLost=%llu rttMs=%.1f jitterMs=%.1f width=%d height=%d fps=%d suppressed=%d\n",
						static_cast<long long>(wallNowMs()),
						track.trackId.c_str(),
						qos::stateStr(track.qosCtrl->currentState()),
						track.qosCtrl->currentLevel(),
						track.videoSuppressed ? "audio-only" : "audio-video",
						usingMatrixProfile ? "matrix" :
							(serverStats.has_value() ? "server" : (statsFresh ? "local" : "stale")),
						track.encBitrate,
						observedBitrateBps > 0.0 ? observedBitrateBps : qSnap.targetBitrateBps,
						static_cast<unsigned long long>(qSnap.packetsLost),
						qSnap.roundTripTimeMs,
						qSnap.jitterMs,
						qSnap.frameWidth,
						qSnap.frameHeight,
						static_cast<int>(qSnap.framesPerSecond),
						track.videoSuppressed ? 1 : 0);
					sampledTrackIds.insert(track.trackId);
				}

				std::vector<qos::PeerTrackState> peerTrackStates;
				std::map<std::string, qos::DerivedSignals> peerTrackSignals;
				std::map<std::string, qos::PlannedAction> peerLastActions;
				std::map<std::string, qos::RawSenderSnapshot> peerRawSnapshots;
				std::map<std::string, bool> peerActionApplied;
				std::vector<qos::TrackBudgetRequest> trackBudgetRequests;
				uint64_t totalDesiredBudgetBps = 0;
				uint64_t totalObservedBudgetBps = 0;
				bool anyBandwidthLimited = false;
				bool peerSnapshotRequested = false;

				for (auto& track : videoTracks_) {
					if (!track.qosCtrl) continue;
					if (sampledTrackIds.find(track.trackId) == sampledTrackIds.end()) continue;
					peerTrackStates.push_back(track.qosCtrl->getTrackState());
					if (auto sig = track.qosCtrl->lastSignals())
						peerTrackSignals[track.trackId] = *sig;
					peerLastActions[track.trackId] = track.qosCtrl->lastAction();
					peerActionApplied[track.trackId] = track.qosCtrl->lastActionWasApplied();
					peerSnapshotRequested = peerSnapshotRequested || track.snapshotRequested;
					track.snapshotRequested = false;

					qos::RawSenderSnapshot raw;
					raw.timestampMs = steadyNowMs();
					raw.trackId = track.trackId;
					raw.producerId = track.producerId;
					raw.source = qos::Source::Camera;
					raw.kind = qos::TrackKind::Video;
					raw.targetBitrateBps = track.encBitrate;
					raw.configuredBitrateBps = track.encBitrate;
					raw.framesPerSecond = track.configuredVideoFps;
					size_t ti = track.index;
					if (ti < trackStats.size() && trackStats[ti].hasData) {
						auto& ns = trackStats[ti].latest;
						raw.bytesSent = ns.octetCount;
						raw.packetsSent = ns.packetCount;
						raw.packetsLost = ns.rrCumulativeLost;
						raw.roundTripTimeMs = ns.rrRttMs;
						raw.jitterMs = ns.rrJitterMs;
					}
					if (ti < trackStats.size()) {
						raw.frameWidth = trackStats[ti].actualWidth;
						raw.frameHeight = trackStats[ti].actualHeight;
					}
					peerRawSnapshots[track.trackId] = raw;

					const uint32_t desiredBps = GetTrackDesiredBitrateBps(track);
					totalDesiredBudgetBps += desiredBps;
					if (auto sig = track.qosCtrl->lastSignals()) {
						if (sig->bandwidthLimited) anyBandwidthLimited = true;
						totalObservedBudgetBps += sig->sendBitrateBps > 0
							? static_cast<uint64_t>(std::llround(sig->sendBitrateBps))
							: desiredBps;
					} else {
						totalObservedBudgetBps += desiredBps;
					}
					trackBudgetRequests.push_back({
						track.trackId,
						qos::Source::Camera,
						qos::TrackKind::Video,
						GetTrackMinBitrateBps(track),
						desiredBps,
						desiredBps,
						track.weight,
						false
					});
				}

				if (trackBudgetRequests.size() > 1) {
					uint32_t totalBudgetBps = static_cast<uint32_t>(
						std::min<uint64_t>(std::numeric_limits<uint32_t>::max(), totalDesiredBudgetBps));
					if (anyBandwidthLimited && totalObservedBudgetBps > 0)
						totalBudgetBps = std::min<uint32_t>(totalBudgetBps, static_cast<uint32_t>(
							std::min<uint64_t>(std::numeric_limits<uint32_t>::max(), totalObservedBudgetBps)));
					auto budgetDecision = qos::allocatePeerTrackBudgets(totalBudgetBps, trackBudgetRequests);
					auto overrides = qos::buildCoordinationOverrides(peerTrackStates, budgetDecision);
					for (auto& track : videoTracks_) {
						if (!track.qosCtrl) continue;
						auto it = overrides.find(track.trackId);
						track.qosCtrl->setCoordinationOverride(
							it != overrides.end()
								? it->second
								: std::optional<qos::CoordinationOverride>{});
					}
				} else {
					for (auto& track : videoTracks_) {
						if (track.qosCtrl)
							track.qosCtrl->setCoordinationOverride(std::nullopt);
					}
				}

				if (peerSnapshotRequested && !peerTrackStates.empty()) {
					if (ws_.pendingRequestCount() < 8) {
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
							hasAudio);
						ws_.requestAsync("clientStats", snapshot,
							[](bool ok, const json&, const std::string& err) {
								if (!ok) std::printf("[QoS] clientStats failed: %s\n", err.c_str());
							});
					}
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	for (auto& worker : workers) worker->stop();
	audioRunning = false;
	if (audioThread.joinable()) audioThread.join();
	netThread.stop();
	std::printf("[threaded] pipeline stopped\n");
	return 0;
}
