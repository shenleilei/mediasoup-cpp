"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const profiles_1 = require("../qos/profiles");
const planner_1 = require("../qos/planner");
function makeInput(partial) {
    return {
        source: 'camera',
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        state: 'stable',
        currentLevel: 0,
        signals: {
            packetsSentDelta: 100,
            packetsLostDelta: 0,
            retransmittedPacketsSentDelta: 0,
            sendBitrateBps: 900000,
            targetBitrateBps: 900000,
            bitrateUtilization: 1,
            lossRate: 0,
            lossEwma: 0,
            rttMs: 80,
            rttEwma: 80,
            jitterMs: 4,
            jitterEwma: 4,
            cpuLimited: false,
            bandwidthLimited: false,
            reason: 'unknown',
        },
        ...partial,
    };
}
test('planner degrades audio source with bitrate-focused actions', () => {
    const actions = (0, planner_1.planActions)(makeInput({
        source: 'audio',
        profile: (0, profiles_1.getDefaultAudioProfile)(),
        currentLevel: 1,
        state: 'congested',
    }));
    expect(actions[0].type).toBe('setEncodingParameters');
    expect(actions[0].level).toBe(2);
    expect(actions[0]).toMatchObject({
        encodingParameters: {
            maxBitrateBps: 24000,
        },
    });
});
test('planner pauses screenshare upstream at max level', () => {
    const actions = (0, planner_1.planActions)(makeInput({
        source: 'screenShare',
        profile: (0, profiles_1.getDefaultScreenShareProfile)(),
        currentLevel: 3,
        state: 'congested',
    }));
    expect(actions).toHaveLength(1);
    expect(actions[0]).toEqual({
        type: 'pauseUpstream',
        level: 4,
    });
});
test('planner degrades on congested state', () => {
    const actions = (0, planner_1.planActions)(makeInput({
        currentLevel: 1,
        state: 'congested',
    }));
    expect(actions.length).toBeGreaterThan(0);
    expect(actions[0].type).toBe('setEncodingParameters');
    expect(actions[0].level).toBe(2);
});
test('planner escalates to audio-only at max level', () => {
    const actions = (0, planner_1.planActions)(makeInput({
        currentLevel: 4,
        state: 'congested',
        inAudioOnlyMode: false,
    }));
    expect(actions).toHaveLength(1);
    expect(actions[0]).toEqual({
        type: 'enterAudioOnly',
        level: 4,
    });
});
test('planner exits audio-only when recovering from max level', () => {
    const actions = (0, planner_1.planActions)(makeInput({
        currentLevel: 4,
        state: 'recovering',
        inAudioOnlyMode: true,
    }));
    expect(actions.length).toBeGreaterThanOrEqual(2);
    expect(actions[0]).toEqual({
        type: 'exitAudioOnly',
        level: 3,
    });
    expect(actions[1].type).toBe('setEncodingParameters');
    expect(actions[1].level).toBe(3);
});
test('planner resumes screenshare on recovery from paused state', () => {
    const actions = (0, planner_1.planActions)(makeInput({
        source: 'screenShare',
        profile: (0, profiles_1.getDefaultScreenShareProfile)(),
        currentLevel: 4,
        state: 'recovering',
    }));
    expect(actions[0]).toEqual({
        type: 'resumeUpstream',
        level: 3,
    });
    expect(actions[1]).toMatchObject({
        type: 'setEncodingParameters',
        level: 3,
    });
});
test('planner applies override clamp', () => {
    const actions = (0, planner_1.planActions)(makeInput({
        currentLevel: 2,
        state: 'congested',
        overrideClampLevel: 2,
    }));
    expect(actions).toHaveLength(1);
    expect(actions[0].type).toBe('noop');
    expect(actions[0].level).toBe(2);
});
test('planRecovery forces recovering path', () => {
    const actions = (0, planner_1.planRecovery)(makeInput({
        currentLevel: 2,
        state: 'congested',
    }));
    expect(actions.length).toBeGreaterThan(0);
    expect(actions[0].level).toBe(1);
});
