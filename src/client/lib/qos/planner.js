"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.planActionsForLevel = planActionsForLevel;
exports.planActions = planActions;
exports.planRecovery = planRecovery;
function clampLevel(level, maxLevel) {
    if (level < 0) {
        return 0;
    }
    if (level > maxLevel) {
        return maxLevel;
    }
    return level;
}
function resolveMaxLevel(profile) {
    return Math.max(0, profile.levelCount - 1);
}
function resolveTargetLevel(input) {
    const maxLevel = resolveMaxLevel(input.profile);
    const currentLevel = clampLevel(input.currentLevel, maxLevel);
    let targetLevel = currentLevel;
    switch (input.state) {
        case 'stable': {
            targetLevel = currentLevel > 0 ? currentLevel - 1 : 0;
            break;
        }
        case 'early_warning': {
            targetLevel = currentLevel < 1 ? 1 : currentLevel;
            break;
        }
        case 'congested': {
            targetLevel = currentLevel + 1;
            break;
        }
        case 'recovering': {
            targetLevel = currentLevel > 0 ? currentLevel - 1 : 0;
            break;
        }
    }
    targetLevel = clampLevel(targetLevel, maxLevel);
    if (typeof input.overrideClampLevel === 'number') {
        targetLevel = Math.min(targetLevel, clampLevel(input.overrideClampLevel, maxLevel));
    }
    return targetLevel;
}
function stepToActions(step, source, currentLevel, inAudioOnlyMode) {
    const actions = [];
    if (source === 'camera' && currentLevel > step.level && inAudioOnlyMode) {
        actions.push({
            type: 'exitAudioOnly',
            level: step.level,
        });
    }
    if (step.resumeUpstream) {
        actions.push({
            type: 'resumeUpstream',
            level: step.level,
        });
    }
    if (step.encodingParameters) {
        actions.push({
            type: 'setEncodingParameters',
            level: step.level,
            encodingParameters: step.encodingParameters,
        });
    }
    if (typeof step.spatialLayer === 'number') {
        actions.push({
            type: 'setMaxSpatialLayer',
            level: step.level,
            spatialLayer: step.spatialLayer,
        });
    }
    if (step.enterAudioOnly) {
        actions.push({
            type: 'enterAudioOnly',
            level: step.level,
        });
    }
    if (step.pauseUpstream) {
        actions.push({
            type: 'pauseUpstream',
            level: step.level,
        });
    }
    if (actions.length === 0) {
        actions.push({
            type: 'noop',
            level: step.level,
            reason: 'empty-ladder-step',
        });
    }
    return actions;
}
function actionsForLevel(input, targetLevel) {
    const step = input.profile.ladder.find(entry => entry.level === targetLevel);
    if (!step) {
        return [
            {
                type: 'noop',
                level: targetLevel,
                reason: 'unknown-level',
            },
        ];
    }
    return stepToActions(step, input.source, input.currentLevel, input.inAudioOnlyMode === true);
}
function planActionsForLevel(input, targetLevel) {
    return actionsForLevel({
        ...input,
        currentLevel: clampLevel(input.currentLevel, resolveMaxLevel(input.profile)),
    }, clampLevel(targetLevel, resolveMaxLevel(input.profile)));
}
function planActions(input) {
    const maxLevel = resolveMaxLevel(input.profile);
    const currentLevel = clampLevel(input.currentLevel, maxLevel);
    const targetLevel = clampLevel(resolveTargetLevel(input), maxLevel);
    if (input.source === 'camera' &&
        currentLevel === maxLevel &&
        input.state === 'congested' &&
        input.inAudioOnlyMode !== true) {
        return planActionsForLevel({
            ...input,
            currentLevel,
        }, maxLevel);
    }
    if (targetLevel === currentLevel) {
        return [
            {
                type: 'noop',
                level: currentLevel,
            },
        ];
    }
    return planActionsForLevel({
        ...input,
        currentLevel,
    }, targetLevel);
}
function planRecovery(input) {
    return planActions({
        ...input,
        state: 'recovering',
    });
}
