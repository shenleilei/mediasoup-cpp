"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.beginProbe = beginProbe;
exports.evaluateProbe = evaluateProbe;
function isProbeHealthy(signals, profile) {
    const { thresholds } = profile;
    return (signals.lossEwma < thresholds.stableLossRate &&
        signals.rttEwma < thresholds.stableRttMs &&
        !signals.bandwidthLimited &&
        !signals.cpuLimited);
}
function isProbeBad(signals, profile) {
    const { thresholds } = profile;
    return (signals.bandwidthLimited ||
        signals.lossEwma >= thresholds.congestedLossRate ||
        signals.rttEwma >= thresholds.congestedRttMs ||
        signals.cpuLimited);
}
function beginProbe(previousLevel, targetLevel, startedAtMs, previousAudioOnlyMode, targetAudioOnlyMode, options = {}) {
    return {
        active: true,
        startedAtMs,
        previousLevel,
        targetLevel,
        previousAudioOnlyMode,
        targetAudioOnlyMode,
        healthySamples: 0,
        badSamples: 0,
        requiredHealthySamples: Math.max(1, Math.floor(options.requiredHealthySamples ?? 3)),
        requiredBadSamples: Math.max(1, Math.floor(options.requiredBadSamples ?? 2)),
    };
}
function evaluateProbe(context, signals, profile) {
    if (!context?.active) {
        return { result: 'inconclusive' };
    }
    let nextContext = { ...context };
    if (isProbeHealthy(signals, profile)) {
        nextContext = {
            ...nextContext,
            healthySamples: nextContext.healthySamples + 1,
            badSamples: 0,
        };
    }
    else if (isProbeBad(signals, profile)) {
        nextContext = {
            ...nextContext,
            healthySamples: 0,
            badSamples: nextContext.badSamples + 1,
        };
    }
    const requiredBadSamples = Math.max(1, Math.floor(nextContext.requiredBadSamples ?? 2));
    const requiredHealthySamples = Math.max(1, Math.floor(nextContext.requiredHealthySamples ?? 3));
    if (nextContext.badSamples >= requiredBadSamples) {
        return {
            context: nextContext,
            result: 'failed',
        };
    }
    if (nextContext.healthySamples >= requiredHealthySamples) {
        return {
            context: nextContext,
            result: 'successful',
        };
    }
    return {
        context: nextContext,
        result: 'inconclusive',
    };
}
