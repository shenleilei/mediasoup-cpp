#pragma once
#include "QosTypes.h"
#include "QosConstants.h"
#include "QosSignals.h"
#include "QosProfiles.h"
#include "QosStateMachine.h"
#include "QosPlanner.h"
#include "QosProbe.h"
#include "QosExecutor.h"
#include "QosCoordinator.h"
#include "QosProtocol.h"
#include <functional>
#include <chrono>
#include <cstdio>

namespace qos {

using SendSnapshotFn = std::function<void(const nlohmann::json&)>;

class PublisherQosController {
public:
	struct Options {
		Source source = Source::Camera;
		std::string trackId = "video";
		std::string producerId;
		int initialLevel = 0;
		ActionSink actionSink;
		SendSnapshotFn sendSnapshot;
	};

	explicit PublisherQosController(const Options& opts)
		: source_(opts.source)
		, trackId_(opts.trackId)
		, producerId_(opts.producerId)
		, currentLevel_(opts.initialLevel)
		, profile_(getProfile(opts.source))
		, executor_(opts.actionSink)
		, sendSnapshot_(opts.sendSnapshot)
	{
		smCtx_.state = State::Stable;
		smCtx_.enteredAtMs = nowMs();
	}

	// Call this every ~1s with fresh stats
	void onSample(const RawSenderSnapshot& snapshot) {
		auto now = nowMs();

		// Warmup: skip first few samples (encoder startup + comedia lock)
		if (sampleCount_ < kWarmupSamples) {
			prevSnapshot_ = snapshot;
			hasPrev_ = true;
			sampleCount_++;
			return;
		}
		DerivedSignals signals = deriveSignals(snapshot, hasPrev_ ? &prevSnapshot_ : nullptr,
			hasPrevSig_ ? &prevSignals_ : nullptr);

		prevSnapshot_ = snapshot;
		prevSignals_ = signals;
		hasPrev_ = true;
		hasPrevSig_ = true;

		// State machine transition
		auto result = evaluateStateTransition(smCtx_, signals, profile_, now);
		smCtx_ = result.context;
		currentQuality_ = result.quality;

		// If probing, evaluate probe
		if (probeCtx_.active) {
			auto probeResult = evaluateProbe(probeCtx_, signals, profile_);
			if (probeResult == ProbeResult::Successful) {
				currentLevel_ = probeCtx_.targetLevel;
				inAudioOnlyMode_ = probeCtx_.targetAudioOnlyMode;
				probeCtx_.active = false;
				printf("[QoS] probe successful → level %d\n", currentLevel_);
			} else if (probeResult == ProbeResult::Failed) {
				currentLevel_ = probeCtx_.previousLevel;
				inAudioOnlyMode_ = probeCtx_.previousAudioOnlyMode;
				probeCtx_.active = false;
				printf("[QoS] probe failed → rollback level %d\n", currentLevel_);
			}
			// Inconclusive: keep probing
		}

		// Plan actions
		PlannerInput pi;
		pi.source = source_;
		pi.profile = &profile_;
		pi.state = smCtx_.state;
		pi.currentLevel = currentLevel_;
		pi.inAudioOnlyMode = inAudioOnlyMode_;
		pi.signals = signals;
		if (overrideClampLevel_ >= 0) pi.overrideClampLevel = overrideClampLevel_;

		auto actions = planActions(pi);
		int prevLevel = currentLevel_;

		for (auto& action : actions) {
			if (action.type == ActionType::Noop) continue;

			// Start probe for recovery (level decrease)
			if (smCtx_.state == State::Recovering && action.level < currentLevel_ && !probeCtx_.active) {
				int64_t cooldown = profile_.recoveryCooldownMs;
				if (now - smCtx_.lastCongestedAtMs >= cooldown) {
					probeCtx_ = beginProbe(currentLevel_, action.level,
						inAudioOnlyMode_, action.type == ActionType::EnterAudioOnly, now);
					printf("[QoS] probe started: level %d → %d\n", currentLevel_, action.level);
				}
			}

			executor_.execute(action);
			currentLevel_ = action.level;

			if (action.type == ActionType::EnterAudioOnly) inAudioOnlyMode_ = true;
			if (action.type == ActionType::ExitAudioOnly) inAudioOnlyMode_ = false;
		}

		lastAction_ = actions.empty() ? PlannedAction{} : actions.back();

		if (currentLevel_ != prevLevel) {
			printf("[QoS] %s level %d→%d (loss=%.1f%% rtt=%.0fms state=%s)\n",
				sourceStr(source_), prevLevel, currentLevel_,
				signals.lossEwma * 100, signals.rttEwma, stateStr(smCtx_.state));
		}

		sampleCount_++;

		// Snapshot reporting (every snapshotIntervalMs)
		if (sendSnapshot_ && (now - lastSnapshotMs_) >= profile_.snapshotIntervalMs) {
			lastSnapshotMs_ = now;
			PeerTrackState ts{trackId_, source_, TrackKind::Video, smCtx_.state, currentQuality_, currentLevel_, inAudioOnlyMode_};
			std::map<std::string, DerivedSignals> sigMap{{trackId_, signals}};
			std::map<std::string, PlannedAction> actMap{{trackId_, lastAction_}};
			auto snap = serializeSnapshot(snapshotSeq_++, now, currentQuality_, false, {ts}, sigMap, actMap);
			sendSnapshot_(snap);
		}
	}

	void setOverrideClampLevel(int level) { overrideClampLevel_ = level; }
	void clearOverride() { overrideClampLevel_ = -1; }

	int currentLevel() const { return currentLevel_; }
	State currentState() const { return smCtx_.state; }
	Quality currentQualityValue() const { return currentQuality_; }

private:
	static int64_t nowMs() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	Source source_;
	std::string trackId_;
	std::string producerId_;
	int currentLevel_ = 0;
	bool inAudioOnlyMode_ = false;
	int overrideClampLevel_ = -1;
	Profile profile_;
	StateMachineContext smCtx_;
	Quality currentQuality_ = Quality::Excellent;
	QosActionExecutor executor_;
	SendSnapshotFn sendSnapshot_;
	ProbeContext probeCtx_;
	PlannedAction lastAction_;

	RawSenderSnapshot prevSnapshot_;
	DerivedSignals prevSignals_;
	bool hasPrev_ = false;
	bool hasPrevSig_ = false;

	int sampleCount_ = 0;
	static constexpr int kWarmupSamples = 5; // skip first 5 seconds
	int snapshotSeq_ = 0;
	int64_t lastSnapshotMs_ = 0;
};

} // namespace qos
