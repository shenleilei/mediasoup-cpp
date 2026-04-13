"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const stateMachine_1 = require("../qos/stateMachine");
function makeProfile(overrides = {}) {
    return {
        name: 'test-camera',
        source: 'camera',
        levelCount: 5,
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 2000,
        recoveryCooldownMs: 8000,
        thresholds: {
            warnLossRate: 0.04,
            congestedLossRate: 0.08,
            warnRttMs: 220,
            congestedRttMs: 320,
            warnBitrateUtilization: 0.85,
            congestedBitrateUtilization: 0.65,
            stableLossRate: 0.03,
            stableRttMs: 180,
            stableBitrateUtilization: 0.92,
        },
        ladder: [],
        ...overrides,
    };
}
function makeSignals(overrides = {}) {
    return {
        packetsSentDelta: 100,
        packetsLostDelta: 0,
        retransmittedPacketsSentDelta: 0,
        sendBitrateBps: 900000,
        targetBitrateBps: 1000000,
        bitrateUtilization: 0.95,
        lossRate: 0.0,
        lossEwma: 0.01,
        rttMs: 120,
        rttEwma: 120,
        jitterMs: 10,
        jitterEwma: 10,
        cpuLimited: false,
        bandwidthLimited: false,
        reason: 'network',
        ...overrides,
    };
}
test('stable -> early_warning after two bandwidth-limited warning samples', () => {
    const profile = makeProfile();
    let context = (0, stateMachine_1.createInitialQosStateMachineContext)(0);
    const warningSignals = makeSignals({
        bitrateUtilization: 0.8,
        bandwidthLimited: true,
    });
    let result = (0, stateMachine_1.evaluateStateTransition)(context, warningSignals, profile, 1000);
    expect(result.context.state).toBe('stable');
    expect(result.transitioned).toBe(false);
    context = result.context;
    result = (0, stateMachine_1.evaluateStateTransition)(context, warningSignals, profile, 2000);
    expect(result.context.state).toBe('early_warning');
    expect(result.transitioned).toBe(true);
});
test('stable stays stable for low utilization alone when network signals are healthy', () => {
    const profile = makeProfile();
    let context = (0, stateMachine_1.createInitialQosStateMachineContext)(0);
    const lowUtilSignals = makeSignals({
        bitrateUtilization: 0.12,
        sendBitrateBps: 120000,
        targetBitrateBps: 1000000,
        reason: 'unknown',
    });
    let result = (0, stateMachine_1.evaluateStateTransition)(context, lowUtilSignals, profile, 1000);
    expect(result.context.state).toBe('stable');
    expect(result.transitioned).toBe(false);
    context = result.context;
    result = (0, stateMachine_1.evaluateStateTransition)(context, lowUtilSignals, profile, 2000);
    expect(result.context.state).toBe('stable');
    expect(result.transitioned).toBe(false);
});
test('early_warning -> congested after two congested samples', () => {
    const profile = makeProfile();
    let context = {
        ...(0, stateMachine_1.createInitialQosStateMachineContext)(0),
        state: 'early_warning',
    };
    const congestedSignals = makeSignals({
        bitrateUtilization: 0.5,
        lossEwma: 0.1,
    });
    let result = (0, stateMachine_1.evaluateStateTransition)(context, congestedSignals, profile, 1000);
    expect(result.context.state).toBe('early_warning');
    expect(result.transitioned).toBe(false);
    context = result.context;
    result = (0, stateMachine_1.evaluateStateTransition)(context, congestedSignals, profile, 2000);
    expect(result.context.state).toBe('congested');
    expect(result.transitioned).toBe(true);
    expect(result.context.lastCongestedAtMs).toBe(2000);
});
test('early_warning -> stable after three healthy samples', () => {
    const profile = makeProfile();
    let context = {
        ...(0, stateMachine_1.createInitialQosStateMachineContext)(0),
        state: 'early_warning',
    };
    const healthySignals = makeSignals();
    let result = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 1000);
    expect(result.context.state).toBe('early_warning');
    context = result.context;
    result = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 2000);
    expect(result.context.state).toBe('early_warning');
    context = result.context;
    result = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 3000);
    expect(result.context.state).toBe('stable');
    expect(result.transitioned).toBe(true);
});
test('congested -> recovering requires cooldown and five healthy samples', () => {
    const profile = makeProfile({ recoveryCooldownMs: 8000 });
    let context = {
        ...(0, stateMachine_1.createInitialQosStateMachineContext)(0),
        state: 'congested',
        lastCongestedAtMs: 1000,
    };
    const healthySignals = makeSignals();
    // Accumulate healthy samples before cooldown expiry.
    context = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 2000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 3000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 4000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 5000).context;
    let result = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 6000);
    expect(result.context.state).toBe('congested');
    // Cooldown elapsed and healthy count satisfied.
    context = result.context;
    result = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 9000);
    expect(result.context.state).toBe('recovering');
    expect(result.transitioned).toBe(true);
    expect(result.context.lastRecoveryAtMs).toBe(9000);
});
test('congested -> recovering uses wider recovery jitter gate than final stable gate', () => {
    const baseProfile = makeProfile();
    const profile = {
        ...baseProfile,
        thresholds: {
            ...baseProfile.thresholds,
            warnJitterMs: 28,
            stableJitterMs: 18,
        },
        recoveryCooldownMs: 8000,
    };
    let context = {
        ...(0, stateMachine_1.createInitialQosStateMachineContext)(0),
        state: 'congested',
        lastCongestedAtMs: 1000,
    };
    const recoveryCandidateSignals = makeSignals({
        jitterMs: 24,
        jitterEwma: 24,
    });
    context = (0, stateMachine_1.evaluateStateTransition)(context, recoveryCandidateSignals, profile, 5000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, recoveryCandidateSignals, profile, 6000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, recoveryCandidateSignals, profile, 7000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, recoveryCandidateSignals, profile, 8000).context;
    const result = (0, stateMachine_1.evaluateStateTransition)(context, recoveryCandidateSignals, profile, 9000);
    expect(result.context.state).toBe('recovering');
    expect(result.transitioned).toBe(true);
    expect(result.context.consecutiveHealthySamples).toBe(0);
    expect(result.context.consecutiveRecoverySamples).toBe(5);
});
test('congested -> recovering can use raw-jitter fast path when ewma tail lags', () => {
    const baseProfile = makeProfile();
    const profile = {
        ...baseProfile,
        thresholds: {
            ...baseProfile.thresholds,
            warnJitterMs: 28,
            stableJitterMs: 18,
        },
        recoveryCooldownMs: 8000,
    };
    let context = {
        ...(0, stateMachine_1.createInitialQosStateMachineContext)(0),
        state: 'congested',
        lastCongestedAtMs: 1000,
    };
    const fastRecoverySignals = makeSignals({
        jitterMs: 24,
        jitterEwma: 36,
        targetBitrateBps: 120000,
        sendBitrateBps: 120000,
    });
    context = (0, stateMachine_1.evaluateStateTransition)(context, fastRecoverySignals, profile, 9000).context;
    const result = (0, stateMachine_1.evaluateStateTransition)(context, fastRecoverySignals, profile, 10000);
    expect(result.context.state).toBe('recovering');
    expect(result.transitioned).toBe(true);
    expect(result.context.consecutiveRecoverySamples).toBe(0);
    expect(result.context.consecutiveFastRecoverySamples).toBe(2);
});
test('recovering -> stable after five healthy samples', () => {
    const profile = makeProfile();
    let context = {
        ...(0, stateMachine_1.createInitialQosStateMachineContext)(0),
        state: 'recovering',
    };
    const healthySignals = makeSignals();
    context = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 1000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 2000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 3000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 4000).context;
    const result = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 5000);
    expect(result.context.state).toBe('stable');
    expect(result.transitioned).toBe(true);
});
test('recovering stays recovering until jitter reaches strict stable threshold', () => {
    const baseProfile = makeProfile();
    const profile = {
        ...baseProfile,
        thresholds: {
            ...baseProfile.thresholds,
            warnJitterMs: 28,
            stableJitterMs: 18,
        },
    };
    let context = {
        ...(0, stateMachine_1.createInitialQosStateMachineContext)(0),
        state: 'recovering',
    };
    const recoveryCandidateSignals = makeSignals({
        jitterMs: 24,
        jitterEwma: 24,
    });
    context = (0, stateMachine_1.evaluateStateTransition)(context, recoveryCandidateSignals, profile, 1000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, recoveryCandidateSignals, profile, 2000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, recoveryCandidateSignals, profile, 3000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, recoveryCandidateSignals, profile, 4000).context;
    const result = (0, stateMachine_1.evaluateStateTransition)(context, recoveryCandidateSignals, profile, 5000);
    expect(result.context.state).toBe('recovering');
    expect(result.transitioned).toBe(false);
    expect(result.context.consecutiveHealthySamples).toBe(0);
    expect(result.context.consecutiveRecoverySamples).toBe(5);
});
test('recovering -> congested after two congested samples', () => {
    const profile = makeProfile();
    let context = {
        ...(0, stateMachine_1.createInitialQosStateMachineContext)(0),
        state: 'recovering',
    };
    const congestedSignals = makeSignals({
        bitrateUtilization: 0.5,
        lossEwma: 0.12,
    });
    context = (0, stateMachine_1.evaluateStateTransition)(context, congestedSignals, profile, 1000).context;
    const result = (0, stateMachine_1.evaluateStateTransition)(context, congestedSignals, profile, 2000);
    expect(result.context.state).toBe('congested');
    expect(result.transitioned).toBe(true);
});
test('stable -> early_warning after two high-jitter samples when jitter thresholds are configured', () => {
    const baseProfile = makeProfile();
    const profile = {
        ...baseProfile,
        thresholds: {
            ...baseProfile.thresholds,
            warnJitterMs: 30,
            stableJitterMs: 20,
        },
    };
    let context = (0, stateMachine_1.createInitialQosStateMachineContext)(0);
    const jitterSignals = makeSignals({
        jitterMs: 36,
        jitterEwma: 36,
    });
    let result = (0, stateMachine_1.evaluateStateTransition)(context, jitterSignals, profile, 1000);
    expect(result.context.state).toBe('stable');
    context = result.context;
    result = (0, stateMachine_1.evaluateStateTransition)(context, jitterSignals, profile, 2000);
    expect(result.context.state).toBe('early_warning');
    expect(result.transitioned).toBe(true);
});
test('illegal transition: stable never jumps directly to congested', () => {
    const profile = makeProfile();
    let context = (0, stateMachine_1.createInitialQosStateMachineContext)(0);
    const severeSignals = makeSignals({ bitrateUtilization: 0.2, lossEwma: 0.2 });
    const result = (0, stateMachine_1.evaluateStateTransition)(context, severeSignals, profile, 1000);
    expect(result.context.state).toBe('stable');
    context = result.context;
    const second = (0, stateMachine_1.evaluateStateTransition)(context, severeSignals, profile, 2000);
    expect(second.context.state).toBe('early_warning');
});
test('illegal transition: congested does not jump directly to stable', () => {
    const profile = makeProfile();
    let context = {
        ...(0, stateMachine_1.createInitialQosStateMachineContext)(0),
        state: 'congested',
        lastCongestedAtMs: 1000,
    };
    const healthySignals = makeSignals();
    context = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 10000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 11000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 12000).context;
    context = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 13000).context;
    const result = (0, stateMachine_1.evaluateStateTransition)(context, healthySignals, profile, 14000);
    expect(result.context.state).toBe('recovering');
    expect(result.context.state).not.toBe('stable');
});
test('mapStateToQuality returns expected values', () => {
    const healthy = makeSignals();
    const lostLike = makeSignals({ sendBitrateBps: 0 });
    expect((0, stateMachine_1.mapStateToQuality)('stable', healthy)).toBe('excellent');
    expect((0, stateMachine_1.mapStateToQuality)('early_warning', healthy)).toBe('good');
    expect((0, stateMachine_1.mapStateToQuality)('recovering', healthy)).toBe('good');
    expect((0, stateMachine_1.mapStateToQuality)('congested', healthy)).toBe('poor');
    expect((0, stateMachine_1.mapStateToQuality)('congested', lostLike)).toBe('lost');
});
