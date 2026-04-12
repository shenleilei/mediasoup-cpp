"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const signals_1 = require("../qos/signals");
function makeSnapshot(overrides = {}) {
    return {
        timestampMs: 1000,
        source: 'camera',
        kind: 'video',
        bytesSent: 0,
        packetsSent: 0,
        packetsLost: 0,
        retransmittedPacketsSent: 0,
        targetBitrateBps: 800000,
        configuredBitrateBps: 800000,
        roundTripTimeMs: 80,
        jitterMs: 10,
        qualityLimitationReason: 'none',
        ...overrides,
    };
}
test('computeCounterDelta handles normal increments and resets', () => {
    expect((0, signals_1.computeCounterDelta)(150, 100)).toBe(50);
    expect((0, signals_1.computeCounterDelta)(100, 150)).toBe(0);
    expect((0, signals_1.computeCounterDelta)(undefined, 100)).toBe(0);
    expect((0, signals_1.computeCounterDelta)(100, undefined)).toBe(100);
});
test('computeBitrateBps computes bits/sec and guards invalid timestamp deltas', () => {
    // (3000 - 1000) bytes over 1000ms => 16000 bps.
    expect((0, signals_1.computeBitrateBps)(3000, 1000, 2000, 1000)).toBe(16000);
    expect((0, signals_1.computeBitrateBps)(3000, 1000, 1000, 1000)).toBe(0);
    expect((0, signals_1.computeBitrateBps)(3000, 1000, 900, 1000)).toBe(0);
    expect((0, signals_1.computeBitrateBps)(undefined, 1000, 2000, 1000)).toBe(0);
});
test('computeLossRate uses lost / (sent + lost)', () => {
    expect((0, signals_1.computeLossRate)(80, 20)).toBeCloseTo(0.2, 6);
    expect((0, signals_1.computeLossRate)(0, 0)).toBe(0);
    expect((0, signals_1.computeLossRate)(-10, -20)).toBe(0);
});
test('computeEwma uses expected smoothing behavior', () => {
    expect((0, signals_1.computeEwma)(100, undefined, 0.35)).toBe(100);
    expect((0, signals_1.computeEwma)(100, 50, 0.35)).toBeCloseTo(67.5, 6);
    expect((0, signals_1.computeEwma)(100, 50, 0)).toBe(50);
    expect((0, signals_1.computeEwma)(100, 50, 1)).toBe(100);
});
test('computeBitrateUtilization prefers target, then configured', () => {
    expect((0, signals_1.computeBitrateUtilization)(400000, 800000, 900000)).toBeCloseTo(0.5, 6);
    expect((0, signals_1.computeBitrateUtilization)(400000, 0, 1000000)).toBeCloseTo(0.4, 6);
    expect((0, signals_1.computeBitrateUtilization)(0, 0, 0)).toBe(0);
    expect((0, signals_1.computeBitrateUtilization)(10, 0, 0)).toBe(1);
});
test('classifyReason priority order: override > manual > cpu > network > unknown', () => {
    const baseSignals = {
        bandwidthLimited: false,
        cpuLimited: false,
        bitrateUtilization: 1,
        lossEwma: 0,
        rttEwma: 10,
    };
    expect((0, signals_1.classifyReason)({
        ...baseSignals,
        cpuLimited: true,
        bandwidthLimited: true,
    }, { activeOverride: true, cpuSampleCount: 10 })).toBe('server_override');
    expect((0, signals_1.classifyReason)({
        ...baseSignals,
        cpuLimited: true,
        bandwidthLimited: true,
    }, { manual: true, cpuSampleCount: 10 })).toBe('manual');
    expect((0, signals_1.classifyReason)({
        ...baseSignals,
        cpuLimited: true,
    }, { cpuSampleCount: 2 })).toBe('cpu');
    expect((0, signals_1.classifyReason)({
        ...baseSignals,
        bandwidthLimited: true,
    })).toBe('network');
    expect((0, signals_1.classifyReason)({
        ...baseSignals,
        bitrateUtilization: 0.6,
    })).toBe('unknown');
    expect((0, signals_1.classifyReason)(baseSignals)).toBe('unknown');
});
test('deriveSignals computes deltas, rates, ewma, flags and reason', () => {
    const previous = makeSnapshot({
        timestampMs: 1000,
        bytesSent: 1000,
        packetsSent: 100,
        packetsLost: 10,
        retransmittedPacketsSent: 4,
        targetBitrateBps: 800000,
        roundTripTimeMs: 100,
        jitterMs: 8,
        qualityLimitationReason: 'none',
    });
    const current = makeSnapshot({
        timestampMs: 2000,
        bytesSent: 21000,
        packetsSent: 220,
        packetsLost: 30,
        retransmittedPacketsSent: 10,
        targetBitrateBps: 1000000,
        roundTripTimeMs: 300,
        jitterMs: 20,
        qualityLimitationReason: 'bandwidth',
    });
    const previousSignals = {
        packetsSentDelta: 0,
        packetsLostDelta: 0,
        retransmittedPacketsSentDelta: 0,
        sendBitrateBps: 0,
        targetBitrateBps: 800000,
        bitrateUtilization: 1,
        lossRate: 0,
        lossEwma: 0.02,
        rttMs: 100,
        rttEwma: 100,
        jitterMs: 8,
        jitterEwma: 8,
        cpuLimited: false,
        bandwidthLimited: false,
        reason: 'unknown',
    };
    const derived = (0, signals_1.deriveSignals)(current, previous, previousSignals, {
        ewmaAlpha: 0.5,
    });
    expect(derived.packetsSentDelta).toBe(120);
    expect(derived.packetsLostDelta).toBe(20);
    expect(derived.retransmittedPacketsSentDelta).toBe(6);
    // (21000 - 1000) bytes over 1000ms => 160000 bps
    expect(derived.sendBitrateBps).toBe(160000);
    expect(derived.targetBitrateBps).toBe(1000000);
    expect(derived.bitrateUtilization).toBeCloseTo(0.16, 6);
    expect(derived.lossRate).toBeCloseTo(20 / 140, 6);
    expect(derived.lossEwma).toBeCloseTo((20 / 140 + 0.02) / 2, 6);
    expect(derived.rttMs).toBe(300);
    expect(derived.rttEwma).toBe(200);
    expect(derived.jitterMs).toBe(20);
    expect(derived.jitterEwma).toBe(14);
    expect(derived.cpuLimited).toBe(false);
    expect(derived.bandwidthLimited).toBe(true);
    expect(derived.reason).toBe('network');
});
test('deriveSignals supports cpu reason when enough cpu samples', () => {
    const current = makeSnapshot({
        timestampMs: 2000,
        bytesSent: 10000,
        packetsSent: 100,
        packetsLost: 0,
        qualityLimitationReason: 'cpu',
    });
    const derived = (0, signals_1.deriveSignals)(current, makeSnapshot(), undefined, {
        reasonContext: { cpuSampleCount: 3 },
    });
    expect(derived.cpuLimited).toBe(true);
    expect(derived.bandwidthLimited).toBe(false);
    expect(derived.reason).toBe('cpu');
});
test('deriveSignals ignores bandwidth limitation reason when utilization is still healthy', () => {
    const previous = makeSnapshot({
        timestampMs: 1000,
        bytesSent: 1000,
        packetsSent: 100,
    });
    const current = makeSnapshot({
        timestampMs: 2000,
        bytesSent: 86000,
        packetsSent: 220,
        targetBitrateBps: 800000,
        qualityLimitationReason: 'bandwidth',
    });
    const derived = (0, signals_1.deriveSignals)(current, previous);
    expect(derived.bitrateUtilization).toBeGreaterThanOrEqual(0.85);
    expect(derived.bandwidthLimited).toBe(false);
});
test('deriveSignals ignores bandwidth limitation reason at moderate utilization', () => {
    const previous = makeSnapshot({
        timestampMs: 1000,
        bytesSent: 1000,
        packetsSent: 100,
    });
    const current = makeSnapshot({
        timestampMs: 2000,
        bytesSent: 91000,
        packetsSent: 220,
        targetBitrateBps: 1000000,
        qualityLimitationReason: 'bandwidth',
    });
    const derived = (0, signals_1.deriveSignals)(current, previous);
    expect(derived.bitrateUtilization).toBeGreaterThan(0.65);
    expect(derived.bitrateUtilization).toBeLessThan(0.85);
    expect(derived.bandwidthLimited).toBe(false);
});
test('deriveSignals guards missing counters and negative deltas', () => {
    const previous = makeSnapshot({
        timestampMs: 2000,
        bytesSent: 5000,
        packetsSent: 100,
        packetsLost: 10,
        retransmittedPacketsSent: 5,
    });
    const current = makeSnapshot({
        timestampMs: 3000,
        bytesSent: 4000,
        packetsSent: 50,
        packetsLost: 5,
        retransmittedPacketsSent: 1,
        targetBitrateBps: 0,
        configuredBitrateBps: 0,
    });
    const derived = (0, signals_1.deriveSignals)(current, previous);
    expect(derived.sendBitrateBps).toBe(0);
    expect(derived.packetsSentDelta).toBe(0);
    expect(derived.packetsLostDelta).toBe(0);
    expect(derived.retransmittedPacketsSentDelta).toBe(0);
    expect(derived.lossRate).toBe(0);
    expect(derived.bitrateUtilization).toBe(0);
});
