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
#include <map>

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
		int warmupSamples = 5;
		int snapshotIntervalMs = DEFAULT_SNAPSHOT_INTERVAL_MS;
		std::function<int64_t()> monotonicNowMs;
		std::function<int64_t()> wallNowMs;
		TrackKind kind = TrackKind::Video;
		bool allowAudioOnly = true;
		bool allowVideoPause = true;
		bool peerHasAudioTrack = true;
	};

	explicit PublisherQosController(const Options& opts)
		: source_(opts.source)
		, trackId_(opts.trackId)
		, producerId_(opts.producerId)
		, kind_(opts.kind)
		, currentLevel_(opts.initialLevel)
		, profile_(getProfile(opts.source))
		, executor_(opts.actionSink)
		, sendSnapshot_(opts.sendSnapshot)
		, warmupSamples_(std::max(0, opts.warmupSamples))
		, snapshotIntervalMs_(opts.snapshotIntervalMs > 0 ? opts.snapshotIntervalMs : getProfile(opts.source).snapshotIntervalMs)
		, monotonicNowMs_(opts.monotonicNowMs ? opts.monotonicNowMs : defaultMonotonicNowMs)
		, wallNowMs_(opts.wallNowMs ? opts.wallNowMs : defaultWallNowMs)
		, allowAudioOnly_(opts.allowAudioOnly)
		, allowVideoPause_(opts.allowVideoPause)
		, peerHasAudioTrack_(opts.peerHasAudioTrack)
	{
		smCtx_ = createInitialQosStateMachineContext(nowMs());
	}

	void handlePolicy(const QosPolicy& policy) {
		sampleIntervalMs_ = policy.sampleIntervalMs;
		snapshotIntervalMs_ = policy.snapshotIntervalMs;
		allowAudioOnly_ = policy.allowAudioOnly;
		allowVideoPause_ = policy.allowVideoPause;

		std::string profileName;
		switch (source_) {
			case Source::Camera: profileName = policy.profiles.camera; break;
			case Source::ScreenShare: profileName = policy.profiles.screenShare; break;
			case Source::Audio: profileName = policy.profiles.audio; break;
		}
		if (!profileName.empty() && profileName != profile_.name) {
			auto resolved = resolveProfileByName(source_, profileName);
			if (resolved.has_value()) profile_ = *resolved;
		}
	}

	void handleOverride(const QosOverride& overrideData) {
		if (overrideData.scope == OverrideScope::Track && overrideData.trackId.has_value()
			&& *overrideData.trackId != trackId_) {
			return;
		}

		const auto now = nowMs();
		if (overrideData.ttlMs == 0) {
			const auto& reason = overrideData.reason;
			if (reason == "server_ttl_expired") {
				activeOverrides_.clear();
			} else if (startsWith(reason, "downlink_v2_") || startsWith(reason, "downlink_v3_")) {
				for (auto it = activeOverrides_.begin(); it != activeOverrides_.end(); ) {
					if (startsWith(it->first, "downlink_v2_") || startsWith(it->first, "downlink_v3_")) {
						it = activeOverrides_.erase(it);
					} else {
						++it;
					}
				}
			} else if (!startsWith(reason, "server_")) {
				for (auto it = activeOverrides_.begin(); it != activeOverrides_.end(); ) {
					if (!startsWith(it->first, "server_")) {
						it = activeOverrides_.erase(it);
					} else {
						++it;
					}
				}
			} else {
				std::string prefix = trimSuffix(trimSuffix(reason, "_clear"), "_expired");
				for (auto it = activeOverrides_.begin(); it != activeOverrides_.end(); ) {
					if (startsWith(it->first, prefix)) {
						it = activeOverrides_.erase(it);
					} else {
						++it;
					}
				}
			}
		} else {
			const std::string key = overrideData.reason.empty() ? "_default" : overrideData.reason;
			activeOverrides_[key] = {overrideData, now + overrideData.ttlMs};
		}
		activeOverride_ = mergeOverrides(now);
	}

	void onSample(const RawSenderSnapshot& snapshot) {
		auto now = nowMs();
		auto activeOverride = getActiveOverride(now);

		if (sampleCount_ < warmupSamples_) {
			applyImmediateOverrideDuringWarmup(activeOverride);
			prevSnapshot_ = snapshot;
			hasPrev_ = true;
			sampleCount_++;
			return;
		}

		// Combined clamp level
		std::optional<int> combinedClampLevel;
		if (overrideClampLevel_ >= 0) combinedClampLevel = overrideClampLevel_;
		if (coordinationOverride_.has_value() && coordinationOverride_->maxLevelClamp.has_value()) {
			combinedClampLevel = combinedClampLevel.has_value()
				? std::optional<int>(std::min(*combinedClampLevel, *coordinationOverride_->maxLevelClamp))
				: coordinationOverride_->maxLevelClamp;
		}
		if (activeOverride.has_value() && activeOverride->maxLevelClamp.has_value()) {
			combinedClampLevel = combinedClampLevel.has_value()
				? std::optional<int>(std::min(*combinedClampLevel, *activeOverride->maxLevelClamp))
				: activeOverride->maxLevelClamp;
		}
		const int maxLevel = std::max(0, profile_.levelCount - 1);
		if (source_ == Source::Camera && !(allowAudioOnly_ && allowVideoPause_) && maxLevel > 0) {
			const int maxVideoLevel = maxLevel - 1;
			combinedClampLevel = combinedClampLevel.has_value()
				? std::optional<int>(std::min(*combinedClampLevel, maxVideoLevel))
				: std::optional<int>(maxVideoLevel);
		}

		bool disableRecovery = (coordinationOverride_.has_value() && coordinationOverride_->disableRecovery.value_or(false))
			|| (activeOverride.has_value() && activeOverride->disableRecovery.value_or(false))
			|| (recoverySuppressedUntilMs_.has_value() && now < *recoverySuppressedUntilMs_);

		// CPU sample tracking
		if (snapshot.qualityLimitationReason == QualityLimitationReason::Cpu) cpuSampleCount_++;
		else cpuSampleCount_ = 0;

		DeriveContext deriveContext;
		deriveContext.activeOverride = activeOverride.has_value();
		deriveContext.cpuSampleCount = cpuSampleCount_;
		DerivedSignals signals = deriveSignals(snapshot, hasPrev_ ? &prevSnapshot_ : nullptr,
			hasPrevSig_ ? &prevSignals_ : nullptr, deriveContext);

		prevSnapshot_ = snapshot;
		prevSignals_ = signals;
		hasPrev_ = true;
		hasPrevSig_ = true;

		// State transition with adjusted signals
		auto transitionSignals = getTransitionSignals(signals, activeOverride, now);
		auto result = evaluateStateTransition(smCtx_, transitionSignals, profile_, now);
		smCtx_ = result.context;
		currentQuality_ = result.quality;

		// Probe evaluation
		if (probeCtx_.active) {
			auto probeResult = evaluateProbe(probeCtx_, signals, profile_);
			if (probeResult == ProbeResult::Failed) {
				// Rollback
				PlannerInput rpi;
				rpi.source = source_; rpi.profile = &profile_; rpi.state = State::Congested;
				rpi.currentLevel = currentLevel_; rpi.inAudioOnlyMode = inAudioOnlyMode_; rpi.signals = signals;
				auto revertActions = planActionsForLevel(rpi, probeCtx_.previousLevel);
				for (auto& a : revertActions) {
					bool ok = executor_.execute(a);
					if (ok) {
						currentLevel_ = a.level;
						if (a.type == ActionType::EnterAudioOnly) inAudioOnlyMode_ = true;
						if (a.type == ActionType::ExitAudioOnly) inAudioOnlyMode_ = false;
					}
				}
				probeCtx_.active = false;
				recoveryProbeSuccessStreak_ = 0;
				recoveryProbeGraceUntilMs_ = std::nullopt;
				recoverySuppressedUntilMs_ = now + profile_.recoveryCooldownMs;
				smCtx_.state = State::Congested;
				smCtx_.enteredAtMs = now;
				smCtx_.lastCongestedAtMs = now;
				printf("[QoS] probe failed → rollback level %d, recovery suppressed\n", currentLevel_);
				sampleCount_++;
				return;
			}
			if (probeResult == ProbeResult::Successful) {
				bool strongSignal = isStrongRecoverySignal(signals) && probeCtx_.badSamples == 0
					&& (smCtx_.state == State::Recovering || recoveryProbeSuccessStreak_ > 0)
					&& currentLevel_ > 0;
				recoveryProbeSuccessStreak_ = strongSignal ? recoveryProbeSuccessStreak_ + 1 : 0;
				recoveryProbeGraceUntilMs_ = strongSignal ? std::optional<int64_t>(now + sampleIntervalMs_ * 3) : std::nullopt;
				currentLevel_ = probeCtx_.targetLevel;
				inAudioOnlyMode_ = probeCtx_.targetAudioOnlyMode;
				probeCtx_.active = false;
				printf("[QoS] probe successful → level %d\n", currentLevel_);
			}
		}

		// Reset streaks when not probing and congested or at level 0
		if (!probeCtx_.active && (smCtx_.state == State::Congested || currentLevel_ == 0))
			recoveryProbeSuccessStreak_ = 0;

		// Plan actions
		PlannerInput pi;
		pi.source = source_; pi.profile = &profile_; pi.state = smCtx_.state;
		pi.currentLevel = currentLevel_; pi.inAudioOnlyMode = inAudioOnlyMode_;
		pi.signals = signals;
		if (combinedClampLevel.has_value()) pi.overrideClampLevel = combinedClampLevel;

		std::vector<PlannedAction> actions;
		if (probeCtx_.active) {
			actions = {{ActionType::Noop, currentLevel_, {}, {}, "probe-in-progress"}};
		} else if (smCtx_.state == State::Recovering && !disableRecovery) {
			actions = planRecovery(pi);
		} else {
			PlannerInput planPi = pi;
			if (smCtx_.state == State::Recovering) planPi.state = State::EarlyWarning;
			actions = planActions(planPi);
		}

		auto filteredActions = filterActions(actions, activeOverride);
		filteredActions = applyCoordinationBudgetCap(filteredActions);

		// Execute with applied tracking
		int prevLevel = currentLevel_;
		bool prevAudioOnly = inAudioOnlyMode_;
		bool appliedEncodingAction = false;
		for (auto& action : filteredActions) {
			if (action.type == ActionType::Noop) {
				lastAction_ = action;
				lastActionApplied_ = true;
				continue;
			}
			bool ok = executor_.execute(action);
			lastAction_ = action;
			lastActionApplied_ = ok;
			if (ok) {
				currentLevel_ = action.level;
				if (action.type == ActionType::EnterAudioOnly) inAudioOnlyMode_ = true;
				if (action.type == ActionType::ExitAudioOnly) inAudioOnlyMode_ = false;
				if (action.type == ActionType::SetEncodingParameters) appliedEncodingAction = true;
			}
		}
		if (appliedEncodingAction) coordinationBitrateCapDirty_ = false;

		// Start probe on recovery (level decreased or exited audio-only)
		bool levelDecreased = currentLevel_ < prevLevel;
		bool exitedAudioOnly = prevAudioOnly && !inAudioOnlyMode_;
		if ((levelDecreased || exitedAudioOnly) && !probeCtx_.active) {
			int reqHealthy = (recoveryProbeSuccessStreak_ > 0 && isStrongRecoverySignal(signals)) ? 2 : 3;
			probeCtx_ = beginProbe(prevLevel, currentLevel_, prevAudioOnly, inAudioOnlyMode_, now);
			probeCtx_.requiredHealthySamples = reqHealthy;
			printf("[QoS] probe started: level %d → %d\n", prevLevel, currentLevel_);
		}

		if (currentLevel_ != prevLevel) {
			printf("[QoS] %s level %d→%d (loss=%.1f%% rtt=%.0fms state=%s)\n",
				sourceStr(source_), prevLevel, currentLevel_,
				signals.lossEwma * 100, signals.rttEwma, stateStr(smCtx_.state));
		}

		sampleCount_++;

		// Snapshot reporting
		bool snapshotDue = (now - lastSnapshotMs_) >= snapshotIntervalMs_;
		if (sendSnapshot_ && (snapshotDue || result.transitioned)) {
			lastSnapshotMs_ = now;
			PeerTrackState ts{trackId_, source_, kind_, smCtx_.state, currentQuality_, currentLevel_, inAudioOnlyMode_, producerId_, signals.reason};
			std::map<std::string, DerivedSignals> sigMap{{trackId_, signals}};
			std::map<std::string, PlannedAction> actMap{{trackId_, lastAction_}};
			std::map<std::string, RawSenderSnapshot> rawMap{{trackId_, snapshot}};
			std::map<std::string, bool> appliedMap{{trackId_, lastActionApplied_}};
			auto snap = serializeSnapshot(
				snapshotSeq_++,
				wallNowMs(),
				currentQuality_,
				false,
				{ts},
				sigMap,
				actMap,
				rawMap,
				appliedMap,
				peerHasAudioTrack_);
			sendSnapshot_(snap);
		}
	}

	void setOverrideClampLevel(int level) { overrideClampLevel_ = level; }
	void clearOverride() { overrideClampLevel_ = -1; }
	void resetExecutor() { executor_.reset(); }

	// For async command model: confirm that a previously queued action was applied by the worker.
	// This advances controller state that was NOT advanced at queue time (because sink returned false).
	void confirmAction(const PlannedAction& action) {
		int prevLevel = currentLevel_;
		bool prevAudioOnly = inAudioOnlyMode_;
		if (action.type == ActionType::SetEncodingParameters) {
			currentLevel_ = action.level;
			executor_.recordKey(action);
		} else if (action.type == ActionType::EnterAudioOnly || action.type == ActionType::PauseUpstream) {
			inAudioOnlyMode_ = true;
			currentLevel_ = action.level;
			executor_.recordKey(action);
		} else if (action.type == ActionType::ExitAudioOnly || action.type == ActionType::ResumeUpstream) {
			inAudioOnlyMode_ = false;
			currentLevel_ = action.level;
			executor_.recordKey(action);
		}
		lastActionApplied_ = true;
		// Start probe if level decreased or exited audio-only (same as onSample path)
		bool levelDecreased = currentLevel_ < prevLevel;
		bool exitedAudioOnly = prevAudioOnly && !inAudioOnlyMode_;
		if ((levelDecreased || exitedAudioOnly) && !probeCtx_.active) {
			probeCtx_ = beginProbe(prevLevel, currentLevel_, prevAudioOnly, inAudioOnlyMode_, nowMs());
		}
	}

	int currentLevel() const { return currentLevel_; }
	State currentState() const { return smCtx_.state; }
	Quality currentQualityValue() const { return currentQuality_; }
	const PlannedAction& lastAction() const { return lastAction_; }
	bool lastActionWasApplied() const { return lastActionApplied_; }
	std::optional<DerivedSignals> lastSignals() const {
		return hasPrevSig_ ? std::optional<DerivedSignals>(prevSignals_) : std::nullopt;
	}
	RuntimeSettings getRuntimeSettings() const {
		return {sampleIntervalMs_, snapshotIntervalMs_, allowAudioOnly_, allowVideoPause_, probeCtx_.active};
	}
	std::optional<CoordinationOverride> getCoordinationOverride() const {
		return coordinationOverride_;
	}
	PeerTrackState getTrackState() const {
		return {trackId_, source_, kind_, smCtx_.state, currentQuality_, currentLevel_, inAudioOnlyMode_, producerId_, hasPrevSig_ ? prevSignals_.reason : Reason::Unknown};
	}
	void setCoordinationOverride(const std::optional<CoordinationOverride>& override = std::nullopt) {
		auto previousCap = coordinationOverride_.has_value()
			? coordinationOverride_->maxBitrateCapBps
			: std::optional<uint32_t>{};
		if (!override.has_value()) {
			coordinationOverride_ = std::nullopt;
		} else {
			CoordinationOverride normalized;
			if (override->maxLevelClamp.has_value())
				normalized.maxLevelClamp = std::max(0, *override->maxLevelClamp);
			if (override->disableRecovery.value_or(false)) normalized.disableRecovery = true;
			if (override->forceAudioOnly.value_or(false)) normalized.forceAudioOnly = true;
			if (override->pauseUpstream.value_or(false)) normalized.pauseUpstream = true;
			if (override->resumeUpstream.value_or(false)) normalized.resumeUpstream = true;
			if (override->maxBitrateCapBps.has_value() && *override->maxBitrateCapBps > 0)
				normalized.maxBitrateCapBps = *override->maxBitrateCapBps;
			if (normalized.pauseUpstream.value_or(false) && normalized.resumeUpstream.value_or(false))
				normalized.resumeUpstream = false;
			if (normalized.maxLevelClamp.has_value() || normalized.disableRecovery.has_value()
				|| normalized.forceAudioOnly.has_value() || normalized.pauseUpstream.has_value()
				|| normalized.resumeUpstream.has_value() || normalized.maxBitrateCapBps.has_value()) {
				coordinationOverride_ = normalized;
			} else {
				coordinationOverride_ = std::nullopt;
			}
		}
		auto nextCap = coordinationOverride_.has_value()
			? coordinationOverride_->maxBitrateCapBps
			: std::optional<uint32_t>{};
		if (previousCap != nextCap) coordinationBitrateCapDirty_ = true;
	}
	const Profile& profileConfig() const { return profile_; }
	std::optional<QosOverride> getActiveOverride(int64_t nowMs) {
		activeOverride_ = mergeOverrides(nowMs);
		return activeOverride_;
	}

private:
	struct ActiveOverrideRecord {
		QosOverride data;
		int64_t expiresAtMs = 0;
	};

	static bool startsWith(const std::string& value, const std::string& prefix) {
		return value.rfind(prefix, 0) == 0;
	}

	static std::string trimSuffix(const std::string& value, const std::string& suffix) {
		if (suffix.empty() || value.size() < suffix.size()) return value;
		if (value.compare(value.size() - suffix.size(), suffix.size(), suffix) != 0) return value;
		return value.substr(0, value.size() - suffix.size());
	}

	void applyImmediateOverrideDuringWarmup(const std::optional<QosOverride>& override) {
		const bool audioOnlyAllowed = allowAudioOnly_ && allowVideoPause_;
		if (!audioOnlyAllowed || source_ != Source::Camera) return;

		const int maxLevel = std::max(0, profile_.levelCount - 1);
		const bool pauseActive = (override.has_value() && override->pauseUpstream.value_or(false))
			|| (coordinationOverride_.has_value() && coordinationOverride_->pauseUpstream.value_or(false));
		const bool resumeActive = ((override.has_value() && override->resumeUpstream.value_or(false))
			|| (coordinationOverride_.has_value() && coordinationOverride_->resumeUpstream.value_or(false))) && !pauseActive;
		const bool forceAudioOnly = (override.has_value() && override->forceAudioOnly.value_or(false))
			|| (coordinationOverride_.has_value() && coordinationOverride_->forceAudioOnly.value_or(false));
		const bool overrideDrivenAudioOnly = inAudioOnlyMode_ && currentLevel_ < maxLevel;

		auto apply = [&](const PlannedAction& action) {
			lastAction_ = action;
			lastActionApplied_ = executor_.execute(action);
			if (!lastActionApplied_) return;
			if (action.type == ActionType::EnterAudioOnly) inAudioOnlyMode_ = true;
			if (action.type == ActionType::ExitAudioOnly) inAudioOnlyMode_ = false;
			currentLevel_ = action.level;
		};

		if ((pauseActive || forceAudioOnly) && !inAudioOnlyMode_) {
			apply({ActionType::EnterAudioOnly, currentLevel_});
			return;
		}
		if (resumeActive && inAudioOnlyMode_) {
			apply({ActionType::ExitAudioOnly, currentLevel_});
			return;
		}
		if (overrideDrivenAudioOnly && !forceAudioOnly && !pauseActive) {
			apply({ActionType::ExitAudioOnly, currentLevel_});
		}
	}

	std::optional<QosOverride> mergeOverrides(int64_t nowMs) {
		std::optional<QosOverride> merged;
		for (auto it = activeOverrides_.begin(); it != activeOverrides_.end(); ) {
			if (nowMs >= it->second.expiresAtMs) {
				it = activeOverrides_.erase(it);
				continue;
			}
			const auto& data = it->second.data;
			if (!merged.has_value()) {
				merged = data;
			} else {
				if (data.maxLevelClamp.has_value()) {
					merged->maxLevelClamp = merged->maxLevelClamp.has_value()
						? std::optional<int>(std::min(*merged->maxLevelClamp, *data.maxLevelClamp))
						: data.maxLevelClamp;
				}
				if (data.forceAudioOnly.value_or(false)) merged->forceAudioOnly = true;
				if (data.disableRecovery.value_or(false)) merged->disableRecovery = true;
				if (data.pauseUpstream.value_or(false)) merged->pauseUpstream = true;
				if (data.resumeUpstream.value_or(false)) merged->resumeUpstream = true;
			}
			++it;
		}
		if (merged.has_value() && merged->pauseUpstream.value_or(false) && merged->resumeUpstream.value_or(false))
			merged->resumeUpstream = false;
		return merged;
	}

	bool isStrongRecoverySignal(const DerivedSignals& signals) const {
		auto& t = profile_.thresholds;
		double stableJitter = t.stableJitterMs < 1e8 ? t.stableJitterMs : 1e9;
		double warnJitter = t.warnJitterMs < 1e8 ? t.warnJitterMs : stableJitter;
		double recoveryJitter = std::max(stableJitter, warnJitter);
		double rawJitter = std::isfinite(signals.jitterMs) ? std::max(0.0, signals.jitterMs) : 1e18;
		double targetBps = std::max(0.0, signals.targetBitrateBps);
		bool sendReady = targetBps <= 0 || signals.sendBitrateBps >= targetBps * 0.85;
		return signals.lossEwma < t.stableLossRate && signals.rttEwma < t.stableRttMs
			&& rawJitter < recoveryJitter && sendReady && !signals.bandwidthLimited && !signals.cpuLimited;
	}

	DerivedSignals getTransitionSignals(const DerivedSignals& signals, const std::optional<QosOverride>& override, int64_t now) {
		auto& t = profile_.thresholds;
		bool networkRecovered = signals.lossEwma < t.stableLossRate && signals.rttEwma < t.stableRttMs
			&& !signals.bandwidthLimited && !signals.cpuLimited;
		DerivedSignals ts = signals;

		bool probeGraceActive = recoveryProbeGraceUntilMs_.has_value() && now < *recoveryProbeGraceUntilMs_;
		if ((probeCtx_.active || probeGraceActive) && networkRecovered) {
			double stableJitter = t.stableJitterMs < 1e8 ? t.stableJitterMs : 1e9;
			ts.jitterEwma = std::min(ts.jitterEwma, std::max(0.0, stableJitter - 0.001));
		}

		if (!inAudioOnlyMode_ || smCtx_.state != State::Congested) return ts;
		if ((override.has_value() && override->forceAudioOnly.value_or(false))
			|| (coordinationOverride_.has_value() && coordinationOverride_->forceAudioOnly.value_or(false))) return ts;
		if (!networkRecovered) return ts;

		ts.bitrateUtilization = std::max(ts.bitrateUtilization, t.stableBitrateUtilization);
		double sj = t.stableJitterMs < 1e8 ? t.stableJitterMs : 1e9;
		ts.jitterEwma = std::min(ts.jitterEwma, std::max(0.0, sj - 0.001));
		return ts;
	}

	std::vector<PlannedAction> filterActions(const std::vector<PlannedAction>& actions, const std::optional<QosOverride>& override) {
		const bool audioOnlyAllowed = allowAudioOnly_ && allowVideoPause_;
		const int maxLevel = std::max(0, profile_.levelCount - 1);
		std::vector<PlannedAction> filtered;

		for (auto& action : actions) {
			if ((action.type == ActionType::EnterAudioOnly || action.type == ActionType::ExitAudioOnly) && !audioOnlyAllowed)
				continue;
			filtered.push_back(action);
		}

		// v3: pauseUpstream / resumeUpstream from server demand aggregation
		bool pauseActive = (override.has_value() && override->pauseUpstream.value_or(false))
			|| (coordinationOverride_.has_value() && coordinationOverride_->pauseUpstream.value_or(false));
		bool resumeActive = ((override.has_value() && override->resumeUpstream.value_or(false))
			|| (coordinationOverride_.has_value() && coordinationOverride_->resumeUpstream.value_or(false))) && !pauseActive;

		if (pauseActive && !inAudioOnlyMode_ && source_ == Source::Camera && audioOnlyAllowed)
			filtered.insert(filtered.begin(), {ActionType::EnterAudioOnly, currentLevel_, {}, {}, "downlink_v3_zero_demand_pause"});
		if (resumeActive && inAudioOnlyMode_ && source_ == Source::Camera && audioOnlyAllowed && !pauseActive)
			filtered.insert(filtered.begin(), {ActionType::ExitAudioOnly, currentLevel_, {}, {}, "downlink_v3_demand_resumed"});

		// forceAudioOnly from override
		bool forceAudioOnly = (override.has_value() && override->forceAudioOnly.value_or(false))
			|| (coordinationOverride_.has_value() && coordinationOverride_->forceAudioOnly.value_or(false));
		if (audioOnlyAllowed && forceAudioOnly && !inAudioOnlyMode_ && source_ == Source::Camera)
			filtered.insert(filtered.begin(), {ActionType::EnterAudioOnly, currentLevel_});

		// Auto exitAudioOnly when override clears (override-driven audio-only at non-max level)
		bool overrideDrivenAudioOnly = source_ == Source::Camera && inAudioOnlyMode_ && currentLevel_ < maxLevel;
		if (audioOnlyAllowed && overrideDrivenAudioOnly && !forceAudioOnly && !pauseActive) {
			bool hasExit = false;
			for (auto& a : filtered) if (a.type == ActionType::ExitAudioOnly) { hasExit = true; break; }
			if (!hasExit) filtered.insert(filtered.begin(), {ActionType::ExitAudioOnly, currentLevel_});
		}

		if (filtered.empty()) filtered.push_back({ActionType::Noop, currentLevel_, {}, {}, "filtered-by-policy"});
		return filtered;
	}

	std::optional<EncodingParameters> getLevelEncodingParameters(int level) const {
		for (const auto& step : profile_.ladder) {
			if (step.level != level || !step.encodingParameters.has_value()) continue;
			return step.encodingParameters;
		}
		return std::nullopt;
	}

	std::optional<EncodingParameters> capEncodingParameters(
		const std::optional<EncodingParameters>& encodingParameters) const
	{
		if (!encodingParameters.has_value()) return std::nullopt;
		if (!coordinationOverride_.has_value() || !coordinationOverride_->maxBitrateCapBps.has_value())
			return encodingParameters;
		auto capped = *encodingParameters;
		const uint32_t capBps = *coordinationOverride_->maxBitrateCapBps;
		if (capped.maxBitrateBps.has_value())
			capped.maxBitrateBps = std::min(*capped.maxBitrateBps, capBps);
		else
			capped.maxBitrateBps = capBps;
		return capped;
	}

	std::vector<PlannedAction> applyCoordinationBudgetCap(const std::vector<PlannedAction>& actions) const {
		std::vector<PlannedAction> cappedActions = actions;
		bool hasEncodingAction = false;
		for (auto& action : cappedActions) {
			if (action.type != ActionType::SetEncodingParameters) continue;
			action.encodingParameters = capEncodingParameters(action.encodingParameters);
			hasEncodingAction = true;
		}
		if (hasEncodingAction) return cappedActions;
		if (!coordinationBitrateCapDirty_) return cappedActions;
		if (currentLevel_ >= std::max(0, profile_.levelCount - 1) && inAudioOnlyMode_) return cappedActions;
		auto encodingParameters = capEncodingParameters(getLevelEncodingParameters(currentLevel_));
		if (!encodingParameters.has_value()) return cappedActions;

		PlannedAction budgetAction{
			ActionType::SetEncodingParameters,
			currentLevel_,
			encodingParameters,
			{},
			coordinationOverride_.has_value() && coordinationOverride_->maxBitrateCapBps.has_value()
				? "coordination-budget-cap"
				: "coordination-budget-restore"
		};

		if (cappedActions.size() == 1 && cappedActions[0].type == ActionType::Noop) {
			cappedActions[0] = budgetAction;
		} else {
			cappedActions.insert(cappedActions.begin(), budgetAction);
		}
		return cappedActions;
	}

	static int64_t defaultMonotonicNowMs() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	static int64_t defaultWallNowMs() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	}

	int64_t nowMs() const { return monotonicNowMs_(); }
	int64_t wallNowMs() const { return wallNowMs_(); }

	Source source_;
	std::string trackId_;
	std::string producerId_;
	TrackKind kind_ = TrackKind::Video;
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
	bool lastActionApplied_ = true;

	RawSenderSnapshot prevSnapshot_;
	DerivedSignals prevSignals_;
	bool hasPrev_ = false;
	bool hasPrevSig_ = false;

	int sampleCount_ = 0;
	int warmupSamples_ = 5;
		int snapshotSeq_ = 0;
		int sampleIntervalMs_ = DEFAULT_SAMPLE_INTERVAL_MS;
		int snapshotIntervalMs_ = DEFAULT_SNAPSHOT_INTERVAL_MS;
		int64_t lastSnapshotMs_ = 0;
		std::function<int64_t()> monotonicNowMs_;
		std::function<int64_t()> wallNowMs_;
		bool allowAudioOnly_ = true;
		bool allowVideoPause_ = true;
		bool peerHasAudioTrack_ = true;
		std::map<std::string, ActiveOverrideRecord> activeOverrides_;
		std::optional<QosOverride> activeOverride_;
		std::optional<CoordinationOverride> coordinationOverride_;
	bool coordinationBitrateCapDirty_ = false;

	// JS parity: recovery/probe tracking
	int cpuSampleCount_ = 0;
	int recoveryProbeSuccessStreak_ = 0;
	std::optional<int64_t> recoveryProbeGraceUntilMs_;
	std::optional<int64_t> recoverySuppressedUntilMs_;
};

} // namespace qos
