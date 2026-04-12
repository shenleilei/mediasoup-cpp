"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.createInitialQosStateMachineContext = createInitialQosStateMachineContext;
exports.mapStateToQuality = mapStateToQuality;
exports.evaluateStateTransition = evaluateStateTransition;
function isWarning(signals, profile) {
    const { thresholds } = profile;
    const warnJitterMs = Number.isFinite(thresholds.warnJitterMs)
        ? thresholds.warnJitterMs
        : Infinity;
    // Low sender utilization alone is content-dependent (for example canvas/static scenes).
    // Treat it as a warning only when the browser also reports bandwidth pressure.
    return (signals.lossEwma >= thresholds.warnLossRate ||
        signals.rttEwma >= thresholds.warnRttMs ||
        signals.jitterEwma >= warnJitterMs ||
        signals.bandwidthLimited ||
        signals.cpuLimited);
}
function isCongested(signals, profile) {
    const { thresholds } = profile;
    const congestedJitterMs = Number.isFinite(thresholds.congestedJitterMs)
        ? thresholds.congestedJitterMs
        : Infinity;
    return (signals.bandwidthLimited ||
        signals.lossEwma >= thresholds.congestedLossRate ||
        signals.rttEwma >= thresholds.congestedRttMs ||
        signals.jitterEwma >= congestedJitterMs);
}
function isHealthy(signals, profile) {
    const { thresholds } = profile;
    const stableJitterMs = Number.isFinite(thresholds.stableJitterMs)
        ? thresholds.stableJitterMs
        : Infinity;
    return (signals.lossEwma < thresholds.stableLossRate &&
        signals.rttEwma < thresholds.stableRttMs &&
        signals.jitterEwma < stableJitterMs &&
        !signals.bandwidthLimited &&
        !signals.cpuLimited);
}
function nextCounter(current, matched) {
    return matched ? current + 1 : 0;
}
function createInitialQosStateMachineContext(nowMs) {
    return {
        state: 'stable',
        enteredAtMs: nowMs,
        consecutiveHealthySamples: 0,
        consecutiveWarningSamples: 0,
        consecutiveCongestedSamples: 0,
    };
}
function mapStateToQuality(state, signals) {
    if (state === 'congested' && signals.sendBitrateBps <= 0) {
        return 'lost';
    }
    switch (state) {
        case 'stable': {
            return 'excellent';
        }
        case 'early_warning': {
            return 'good';
        }
        case 'congested': {
            return 'poor';
        }
        case 'recovering': {
            return 'good';
        }
    }
}
function evaluateStateTransition(context, signals, profile, nowMs) {
    const healthy = isHealthy(signals, profile);
    const warning = isWarning(signals, profile);
    const congested = isCongested(signals, profile);
    let nextContext = {
        ...context,
        consecutiveHealthySamples: nextCounter(context.consecutiveHealthySamples, healthy),
        consecutiveWarningSamples: nextCounter(context.consecutiveWarningSamples, warning),
        consecutiveCongestedSamples: nextCounter(context.consecutiveCongestedSamples, congested),
    };
    let nextState = context.state;
    let transitioned = false;
    switch (context.state) {
        case 'stable': {
            if (nextContext.consecutiveWarningSamples >= 2) {
                nextState = 'early_warning';
            }
            break;
        }
        case 'early_warning': {
            if (nextContext.consecutiveCongestedSamples >= 2) {
                nextState = 'congested';
            }
            else if (nextContext.consecutiveHealthySamples >= 3) {
                nextState = 'stable';
            }
            break;
        }
        case 'congested': {
            const congestedSinceMs = context.lastCongestedAtMs ?? context.enteredAtMs;
            const cooldownElapsed = nowMs - congestedSinceMs >= profile.recoveryCooldownMs;
            if (cooldownElapsed && nextContext.consecutiveHealthySamples >= 5) {
                nextState = 'recovering';
            }
            break;
        }
        case 'recovering': {
            if (nextContext.consecutiveCongestedSamples >= 2) {
                nextState = 'congested';
            }
            else if (nextContext.consecutiveHealthySamples >= 5) {
                nextState = 'stable';
            }
            break;
        }
    }
    if (nextState !== context.state) {
        transitioned = true;
        nextContext = {
            ...nextContext,
            state: nextState,
            enteredAtMs: nowMs,
        };
        if (nextState === 'congested') {
            nextContext.lastCongestedAtMs = nowMs;
        }
        if (nextState === 'recovering') {
            nextContext.lastRecoveryAtMs = nowMs;
        }
    }
    return {
        context: nextContext,
        quality: mapStateToQuality(nextContext.state, signals),
        transitioned,
    };
}
