"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
const fs = __importStar(require("node:fs"));
const path = __importStar(require("node:path"));
const profiles_1 = require("../qos/profiles");
const planner_1 = require("../qos/planner");
function loadReplayFixture(name) {
    const fixturePath = path.join(__dirname, 'fixtures', 'qos', `${name}.json`);
    const content = fs.readFileSync(fixturePath, 'utf8');
    const parsed = JSON.parse(content);
    return parsed;
}
function makePlannerInput(level, state, inAudioOnlyMode) {
    return {
        source: 'camera',
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        state,
        currentLevel: level,
        inAudioOnlyMode,
        signals: {
            packetsSentDelta: 100,
            packetsLostDelta: 0,
            retransmittedPacketsSentDelta: 0,
            sendBitrateBps: 800000,
            targetBitrateBps: 900000,
            bitrateUtilization: 0.9,
            lossRate: 0.01,
            lossEwma: 0.01,
            rttMs: 120,
            rttEwma: 120,
            jitterMs: 8,
            jitterEwma: 8,
            cpuLimited: false,
            bandwidthLimited: false,
            reason: 'network',
        },
    };
}
function applyActions(level, inAudioOnlyMode, actions) {
    let nextLevel = level;
    let nextAudioOnly = inAudioOnlyMode;
    for (const action of actions) {
        switch (action.type) {
            case 'setEncodingParameters':
            case 'setMaxSpatialLayer':
            case 'noop': {
                nextLevel = action.level;
                break;
            }
            case 'enterAudioOnly': {
                nextLevel = action.level;
                nextAudioOnly = true;
                break;
            }
            case 'exitAudioOnly': {
                nextLevel = action.level;
                nextAudioOnly = false;
                break;
            }
        }
    }
    return { nextLevel, nextAudioOnly };
}
function assertScenario(name) {
    const scenario = loadReplayFixture(name);
    let level = scenario.initialLevel;
    let inAudioOnlyMode = scenario.initialAudioOnly;
    const primaryActions = [];
    for (const step of scenario.steps) {
        const input = makePlannerInput(level, step.state, inAudioOnlyMode);
        const actions = (0, planner_1.planActions)(input);
        expect(actions.length).toBeGreaterThan(0);
        primaryActions.push(actions[0].type);
        expect(actions[0].type).toBe(step.expectedPrimaryAction);
        const next = applyActions(level, inAudioOnlyMode, actions);
        level = next.nextLevel;
        inAudioOnlyMode = next.nextAudioOnly;
        expect(level).toBe(step.expectedLevel);
    }
    expect(level).toBe(scenario.expectedFinalLevel);
    expect(inAudioOnlyMode).toBe(scenario.expectedFinalAudioOnly);
    expect(primaryActions.length).toBe(scenario.steps.length);
}
test('replay camera_mild_congestion scenario', () => {
    expect.hasAssertions();
    assertScenario('camera_mild_congestion');
});
test('replay camera_persistent_congestion scenario', () => {
    expect.hasAssertions();
    assertScenario('camera_persistent_congestion');
});
test('replay camera_recovery scenario', () => {
    expect.hasAssertions();
    assertScenario('camera_recovery');
});
