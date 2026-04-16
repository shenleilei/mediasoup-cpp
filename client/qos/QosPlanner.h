#pragma once
#include "QosTypes.h"
#include <algorithm>
#include <vector>

namespace qos {

inline int clampLevel(int level, int maxLevel) { return std::clamp(level, 0, maxLevel); }

inline int resolveTargetLevel(const PlannerInput& input) {
	int maxLevel = std::max(0, input.profile->levelCount - 1);
	int cur = clampLevel(input.currentLevel, maxLevel);
	int target = cur;
	switch (input.state) {
		case State::Stable:      target = cur > 0 ? cur - 1 : 0; break;
		case State::EarlyWarning: target = cur < 1 ? 1 : cur; break;
		case State::Congested:   target = cur + 1; break;
		case State::Recovering:  target = cur > 0 ? cur - 1 : 0; break;
	}
	target = clampLevel(target, maxLevel);
	if (input.overrideClampLevel.has_value())
		target = std::min(target, clampLevel(*input.overrideClampLevel, maxLevel));
	return target;
}

inline std::vector<PlannedAction> planActionsForLevel(const PlannerInput& input, int targetLevel) {
	int maxLevel = std::max(0, input.profile->levelCount - 1);
	int tgt = clampLevel(targetLevel, maxLevel);
	const LadderStep* step = nullptr;
	for (auto& s : input.profile->ladder)
		if (s.level == tgt) { step = &s; break; }
	if (!step) return {{ActionType::Noop, tgt, {}, {}, "unknown-level"}};

	std::vector<PlannedAction> actions;
	if (input.source == Source::Camera && input.currentLevel > step->level && input.inAudioOnlyMode)
		actions.push_back({ActionType::ExitAudioOnly, step->level});
	if (step->resumeUpstream)
		actions.push_back({ActionType::ResumeUpstream, step->level});
	if (step->encodingParameters.has_value())
		actions.push_back({ActionType::SetEncodingParameters, step->level, step->encodingParameters});
	if (step->spatialLayer.has_value())
		actions.push_back({ActionType::SetMaxSpatialLayer, step->level, {}, step->spatialLayer});
	if (step->enterAudioOnly)
		actions.push_back({ActionType::EnterAudioOnly, step->level});
	if (step->pauseUpstream)
		actions.push_back({ActionType::PauseUpstream, step->level});
	if (actions.empty())
		actions.push_back({ActionType::Noop, step->level, {}, {}, "empty-ladder-step"});
	return actions;
}

inline std::vector<PlannedAction> planActions(const PlannerInput& input) {
	int maxLevel = std::max(0, input.profile->levelCount - 1);
	int cur = clampLevel(input.currentLevel, maxLevel);
	int target = clampLevel(resolveTargetLevel(input), maxLevel);

	if (input.source == Source::Camera && cur == maxLevel
		&& input.state == State::Congested && !input.inAudioOnlyMode)
		return planActionsForLevel(input, maxLevel);

	if (target == cur) return {{ActionType::Noop, cur}};
	return planActionsForLevel(input, target);
}

inline std::vector<PlannedAction> planRecovery(const PlannerInput& input) {
	PlannerInput recoveryInput = input;
	recoveryInput.state = State::Recovering;
	return planActions(recoveryInput);
}

} // namespace qos
