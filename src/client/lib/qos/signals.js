"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.computeCounterDelta = computeCounterDelta;
exports.computeBitrateBps = computeBitrateBps;
exports.computeLossRate = computeLossRate;
exports.computeEwma = computeEwma;
exports.computeBitrateUtilization = computeBitrateUtilization;
exports.classifyReason = classifyReason;
exports.deriveSignals = deriveSignals;
const DEFAULT_EWMA_ALPHA = 0.35;
const DEFAULT_NETWORK_WARN_LOSS_RATE = 0.04;
const DEFAULT_NETWORK_WARN_RTT_MS = 220;
const DEFAULT_NETWORK_WARN_UTILIZATION = 0.85;
const DEFAULT_NETWORK_CONGESTED_UTILIZATION = 0.65;
const DEFAULT_CPU_REASON_MIN_SAMPLES = 2;
function asFiniteOrZero(value) {
    return typeof value === 'number' && Number.isFinite(value) ? value : 0;
}
function clampToUnitRange(value) {
    if (value < 0) {
        return 0;
    }
    else if (value > 1) {
        return 1;
    }
    return value;
}
/**
 * Returns non-negative counter delta.
 * Missing/invalid values, clock skew or counter reset all return 0.
 */
function computeCounterDelta(currentValue, previousValue) {
    const current = asFiniteOrZero(currentValue);
    const previous = asFiniteOrZero(previousValue);
    if (current <= previous) {
        return 0;
    }
    return current - previous;
}
/**
 * Computes sender bitrate in bps from byte counters and timestamps.
 */
function computeBitrateBps(currentBytesSent, previousBytesSent, currentTimestampMs, previousTimestampMs) {
    const deltaBytes = computeCounterDelta(currentBytesSent, previousBytesSent);
    const previousTs = asFiniteOrZero(previousTimestampMs);
    if (!Number.isFinite(currentTimestampMs) ||
        currentTimestampMs <= previousTs ||
        deltaBytes <= 0) {
        return 0;
    }
    const deltaMs = currentTimestampMs - previousTs;
    return (deltaBytes * 8 * 1000) / deltaMs;
}
/**
 * Uses media-level loss ratio = lost / (sent + lost).
 */
function computeLossRate(packetsSentDelta, packetsLostDelta) {
    const sent = Math.max(0, asFiniteOrZero(packetsSentDelta));
    const lost = Math.max(0, asFiniteOrZero(packetsLostDelta));
    const total = sent + lost;
    if (total <= 0) {
        return 0;
    }
    return clampToUnitRange(lost / total);
}
function computeEwma(currentValue, previousEwma, alpha = DEFAULT_EWMA_ALPHA) {
    const current = asFiniteOrZero(currentValue);
    const prev = asFiniteOrZero(previousEwma);
    const safeAlpha = clampToUnitRange(asFiniteOrZero(alpha));
    if (safeAlpha <= 0) {
        return prev;
    }
    else if (safeAlpha >= 1) {
        return current;
    }
    else if (previousEwma === undefined || !Number.isFinite(previousEwma)) {
        return current;
    }
    return safeAlpha * current + (1 - safeAlpha) * prev;
}
function computeBitrateUtilization(sendBitrateBps, targetBitrateBps, configuredBitrateBps) {
    const send = Math.max(0, asFiniteOrZero(sendBitrateBps));
    const target = Math.max(0, asFiniteOrZero(targetBitrateBps));
    const configured = Math.max(0, asFiniteOrZero(configuredBitrateBps));
    const denominator = target > 0 ? target : configured;
    if (denominator <= 0) {
        return send > 0 ? 1 : 0;
    }
    return send / denominator;
}
function classifyReason(signals, context = {}) {
    if (context.activeOverride) {
        return 'server_override';
    }
    else if (context.manual) {
        return 'manual';
    }
    const cpuSampleCount = Math.floor(asFiniteOrZero(context.cpuSampleCount));
    if (signals.cpuLimited && cpuSampleCount >= DEFAULT_CPU_REASON_MIN_SAMPLES) {
        return 'cpu';
    }
    const networkLikely = signals.bandwidthLimited ||
        signals.lossEwma >= DEFAULT_NETWORK_WARN_LOSS_RATE ||
        signals.rttEwma >= DEFAULT_NETWORK_WARN_RTT_MS;
    if (networkLikely) {
        return 'network';
    }
    return 'unknown';
}
function deriveSignals(current, previous, previousSignals, options = {}) {
    const packetsSentDelta = computeCounterDelta(current.packetsSent, previous?.packetsSent);
    const packetsLostDelta = computeCounterDelta(current.packetsLost, previous?.packetsLost);
    const retransmittedPacketsSentDelta = computeCounterDelta(current.retransmittedPacketsSent, previous?.retransmittedPacketsSent);
    const sendBitrateBps = computeBitrateBps(current.bytesSent, previous?.bytesSent, asFiniteOrZero(current.timestampMs), previous?.timestampMs);
    const targetBitrateBps = Math.max(0, asFiniteOrZero(current.targetBitrateBps));
    const bitrateUtilization = computeBitrateUtilization(sendBitrateBps, targetBitrateBps, current.configuredBitrateBps);
    const lossRate = computeLossRate(packetsSentDelta, packetsLostDelta);
    const lossEwma = computeEwma(lossRate, previousSignals?.lossEwma, options.ewmaAlpha);
    const rttMs = Math.max(0, asFiniteOrZero(current.roundTripTimeMs));
    const rttEwma = computeEwma(rttMs, previousSignals?.rttEwma, options.ewmaAlpha);
    const jitterMs = Math.max(0, asFiniteOrZero(current.jitterMs));
    const jitterEwma = computeEwma(jitterMs, previousSignals?.jitterEwma, options.ewmaAlpha);
    const qualityReason = current.qualityLimitationReason ?? 'unknown';
    const cpuLimited = qualityReason === 'cpu';
    // Browsers may report "bandwidth" after our own local bitrate cap changes.
    // Treat it as a real network-pressure signal only when utilization is also low.
    const bandwidthLimited = qualityReason === 'bandwidth' &&
        bitrateUtilization < DEFAULT_NETWORK_CONGESTED_UTILIZATION;
    const reason = classifyReason({
        bandwidthLimited,
        cpuLimited,
        bitrateUtilization,
        lossEwma,
        rttEwma,
    }, options.reasonContext);
    return {
        packetsSentDelta,
        packetsLostDelta,
        retransmittedPacketsSentDelta,
        sendBitrateBps,
        targetBitrateBps,
        bitrateUtilization,
        lossRate,
        lossEwma,
        rttMs,
        rttEwma,
        jitterMs,
        jitterEwma,
        cpuLimited,
        bandwidthLimited,
        reason,
    };
}
