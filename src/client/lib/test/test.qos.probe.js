"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const profiles_1 = require("../qos/profiles");
const probe_1 = require("../qos/probe");
function signals(overrides = {}) {
    return {
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
        jitterMs: 5,
        jitterEwma: 5,
        cpuLimited: false,
        bandwidthLimited: false,
        reason: 'unknown',
        ...overrides,
    };
}
test('beginProbe initializes probe context', () => {
    const probe = (0, probe_1.beginProbe)(3, 2, 1000, false, false);
    expect(probe.active).toBe(true);
    expect(probe.previousLevel).toBe(3);
    expect(probe.targetLevel).toBe(2);
    expect(probe.healthySamples).toBe(0);
    expect(probe.badSamples).toBe(0);
});
test('probe succeeds after three healthy samples', () => {
    const profile = (0, profiles_1.getDefaultCameraProfile)();
    let probe = (0, probe_1.beginProbe)(3, 2, 1000, false, false);
    let evaluation = (0, probe_1.evaluateProbe)(probe, signals(), profile);
    expect(evaluation.result).toBe('inconclusive');
    probe = evaluation.context;
    evaluation = (0, probe_1.evaluateProbe)(probe, signals(), profile);
    expect(evaluation.result).toBe('inconclusive');
    probe = evaluation.context;
    evaluation = (0, probe_1.evaluateProbe)(probe, signals(), profile);
    expect(evaluation.result).toBe('successful');
});
test('probe fails after two bad samples', () => {
    const profile = (0, profiles_1.getDefaultCameraProfile)();
    let probe = (0, probe_1.beginProbe)(3, 2, 1000, false, false);
    const badSignals = signals({
        bitrateUtilization: 0.5,
        lossEwma: 0.1,
        bandwidthLimited: true,
    });
    let evaluation = (0, probe_1.evaluateProbe)(probe, badSignals, profile);
    expect(evaluation.result).toBe('inconclusive');
    probe = evaluation.context;
    evaluation = (0, probe_1.evaluateProbe)(probe, badSignals, profile);
    expect(evaluation.result).toBe('failed');
});
test('probe stays inconclusive for mixed samples below completion thresholds', () => {
    const profile = (0, profiles_1.getDefaultCameraProfile)();
    let probe = (0, probe_1.beginProbe)(3, 2, 1000, false, false);
    let evaluation = (0, probe_1.evaluateProbe)(probe, signals(), profile);
    expect(evaluation.result).toBe('inconclusive');
    probe = evaluation.context;
    evaluation = (0, probe_1.evaluateProbe)(probe, signals({
        bitrateUtilization: 0.8,
        lossEwma: 0.02,
        rttEwma: 150,
    }), profile);
    expect(evaluation.result).toBe('inconclusive');
    expect(evaluation.context?.healthySamples).toBe(2);
    expect(evaluation.context?.badSamples).toBe(0);
});
test('probe succeeds on low utilization alone when bandwidth pressure is absent', () => {
    const profile = (0, profiles_1.getDefaultCameraProfile)();
    let probe = (0, probe_1.beginProbe)(3, 2, 1000, false, false);
    const lowUtilSignals = signals({
        bitrateUtilization: 0.2,
        sendBitrateBps: 180000,
        targetBitrateBps: 900000,
        reason: 'unknown',
    });
    let evaluation = (0, probe_1.evaluateProbe)(probe, lowUtilSignals, profile);
    expect(evaluation.result).toBe('inconclusive');
    probe = evaluation.context;
    evaluation = (0, probe_1.evaluateProbe)(probe, lowUtilSignals, profile);
    expect(evaluation.result).toBe('inconclusive');
    probe = evaluation.context;
    evaluation = (0, probe_1.evaluateProbe)(probe, lowUtilSignals, profile);
    expect(evaluation.result).toBe('successful');
});
