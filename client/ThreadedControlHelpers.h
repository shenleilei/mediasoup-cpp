// ThreadedControlHelpers.h — Shared helper logic for threaded control/QoS bridge.
#pragma once

#include "ThreadTypes.h"
#include "qos/QosController.h"

#include <chrono>

namespace mt {

struct PendingTrackCommand {
	uint64_t commandId = 0;
	qos::PlannedAction action;
	bool pending = false;
};

struct ThreadedTrackStatsState {
	SenderStatsSnapshot latest;
	bool hasData = false;
	uint64_t lastConsumedGeneration = 0;
	uint32_t lastObservedProbePacketCount = 0;
	int localProbeSuppressionSamples = 0;
	int actualWidth = 0;
	int actualHeight = 0;
};

struct ThreadedTrackControlState {
	int encBitrate = 0;
	int configuredVideoFps = 0;
	double scaleResolutionDownBy = 1.0;
	bool videoSuppressed = false;
	uint64_t configGeneration = 0;
	qos::PublisherQosController* qosCtrl = nullptr;
};

struct ProbeSampleSuppressionDecision {
	bool statsFresh = false;
	bool hasServerStats = false;
	bool suppressed = false;
};

inline int64_t threadedControlNowMs() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline bool enqueueTrackAction(
	const qos::PlannedAction& action,
	uint32_t trackIndex,
	uint64_t& nextCommandId,
	SpscQueue<TrackControlCommand, kControlCommandQueueCapacity>& ctrlQueue,
	PendingTrackCommand& pending,
	uint64_t currentConfigGeneration)
{
	TrackControlCommand cmd;
	if (action.type == qos::ActionType::SetEncodingParameters && action.encodingParameters.has_value()) {
		cmd.type = TrackCommandType::SetEncodingParameters;
		auto& ep = *action.encodingParameters;
		cmd.bitrateBps = ep.maxBitrateBps.value_or(0);
		cmd.fps = ep.maxFramerate.has_value() ? std::max(1, static_cast<int>(*ep.maxFramerate)) : 0;
		cmd.scaleResolutionDownBy = ep.scaleResolutionDownBy.value_or(1.0);
	} else if (action.type == qos::ActionType::EnterAudioOnly || action.type == qos::ActionType::PauseUpstream) {
		cmd.type = TrackCommandType::PauseTrack;
	} else if (action.type == qos::ActionType::ExitAudioOnly || action.type == qos::ActionType::ResumeUpstream) {
		cmd.type = TrackCommandType::ResumeTrack;
	} else {
		return false;
	}

	cmd.trackIndex = trackIndex;
	cmd.commandId = nextCommandId++;
	cmd.configGeneration = currentConfigGeneration;
	cmd.issuedAtMs = threadedControlNowMs();
	if (!ctrlQueue.tryPush(std::move(cmd))) return false;

	pending = {cmd.commandId, action, true};
	return true;
}

inline bool enqueueNetworkTrackAction(
	const qos::PlannedAction& action,
	uint32_t trackIndex,
	uint32_t ssrc,
	uint8_t payloadType,
	uint64_t& nextCommandId,
	SpscQueue<NetworkControlCommand, kControlCommandQueueCapacity>& netQueue,
	PendingTrackCommand& pending)
{
	NetworkControlCommand cmd;
	if (action.type == qos::ActionType::EnterAudioOnly || action.type == qos::ActionType::PauseUpstream) {
		cmd.type = NetworkControlCommand::PauseTrack;
	} else if (action.type == qos::ActionType::ExitAudioOnly || action.type == qos::ActionType::ResumeUpstream) {
		cmd.type = NetworkControlCommand::ResumeTrack;
	} else {
		return false;
	}

	cmd.trackIndex = trackIndex;
	cmd.commandId = nextCommandId++;
	cmd.ssrc = ssrc;
	cmd.payloadType = payloadType;
	if (!netQueue.tryPush(std::move(cmd))) return false;

	pending = {cmd.commandId, action, true};
	return true;
}

inline bool enqueueTrackTransportConfig(
	uint32_t trackIndex,
	uint32_t ssrc,
	uint8_t payloadType,
	uint32_t targetBitrateBps,
	bool paused,
	SpscQueue<NetworkControlCommand, kControlCommandQueueCapacity>& netQueue)
{
	NetworkControlCommand cmd;
	cmd.type = NetworkControlCommand::TrackTransportConfig;
	cmd.trackIndex = trackIndex;
	cmd.ssrc = ssrc;
	cmd.payloadType = payloadType;
	cmd.targetBitrateBps = targetBitrateBps;
	cmd.paused = paused;
	return netQueue.tryPush(std::move(cmd));
}

inline bool shouldSampleTrack(bool statsFresh, bool hasServerStatsForTrack) {
	return statsFresh || hasServerStatsForTrack;
}

inline ProbeSampleSuppressionDecision applyProbeSampleSuppression(
	ThreadedTrackStatsState& statsState,
	bool statsFresh,
	bool hasServerStats)
{
	ProbeSampleSuppressionDecision decision;
	decision.statsFresh = statsFresh;
	decision.hasServerStats = hasServerStats;

	if (statsFresh) {
		const auto& latest = statsState.latest;
		const bool probeStatsPresent =
			latest.probeActive ||
			latest.probePacketCount != statsState.lastObservedProbePacketCount;
		statsState.lastObservedProbePacketCount = latest.probePacketCount;
		if (probeStatsPresent) {
			statsState.localProbeSuppressionSamples = std::max(
				statsState.localProbeSuppressionSamples,
				2);
		}
	}

	if (statsState.localProbeSuppressionSamples > 0) {
		decision.statsFresh = false;
		decision.hasServerStats = false;
		decision.suppressed = true;
		statsState.localProbeSuppressionSamples--;
	}

	return decision;
}

inline bool applyCommandAck(
	const CommandAck& ack,
	PendingTrackCommand& pending,
	ThreadedTrackControlState& trackState,
	ThreadedTrackStatsState& statsState)
{
	if (!pending.pending || ack.commandId != pending.commandId) return false;
	if (ack.applied && ack.type == TrackCommandType::SetEncodingParameters
		&& ack.configGeneration < trackState.configGeneration) {
		return false;
	}
	pending.pending = false;

	if (ack.applied) {
		if (ack.type == TrackCommandType::SetEncodingParameters) {
			trackState.encBitrate = ack.actualBitrateBps;
			trackState.configuredVideoFps = ack.actualFps;
			trackState.scaleResolutionDownBy = ack.actualScale;
			trackState.configGeneration = ack.configGeneration;
			statsState.actualWidth = ack.actualWidth;
			statsState.actualHeight = ack.actualHeight;
		} else if (ack.type == TrackCommandType::PauseTrack) {
			trackState.videoSuppressed = true;
		} else if (ack.type == TrackCommandType::ResumeTrack) {
			trackState.videoSuppressed = false;
		}

		if (trackState.qosCtrl) trackState.qosCtrl->confirmAction(pending.action);
	} else {
		if (trackState.qosCtrl) trackState.qosCtrl->resetExecutor();
	}

	return true;
}

} // namespace mt
