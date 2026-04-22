"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.PublisherQosController = void 0;
const constants_1 = require("./constants");
const planner_1 = require("./planner");
const planner_2 = require("./planner");
const probe_1 = require("./probe");
const sampler_1 = require("./sampler");
const signals_1 = require("./signals");
const stateMachine_1 = require("./stateMachine");
const trace_1 = require("./trace");
function resolvePeerMode(inAudioOnlyMode, kind) {
    if (inAudioOnlyMode || kind === 'audio') {
        return 'audio-only';
    }
    return 'audio-video';
}
function toError(error) {
    if (error instanceof Error) {
        return error;
    }
    return new Error(String(error));
}
function wasActionApplied(result) {
    if (result === undefined || result === true) {
        return true;
    }
    if (result === false) {
        return false;
    }
    if (typeof result === 'object' && result !== null) {
        if ('status' in result) {
            return result.status !== 'failed';
        }
        if ('applied' in result) {
            return result.applied !== false;
        }
    }
    return false;
}
function isStrongRecoverySignal(signals, profile) {
    const { thresholds } = profile;
    const stableJitterMs = Number.isFinite(thresholds.stableJitterMs)
        ? thresholds.stableJitterMs
        : Infinity;
    const warnJitterMs = Number.isFinite(thresholds.warnJitterMs)
        ? thresholds.warnJitterMs
        : stableJitterMs;
    const recoveryJitterMs = Math.max(stableJitterMs, warnJitterMs);
    const rawJitterMs = Number.isFinite(signals.jitterMs)
        ? Math.max(0, signals.jitterMs)
        : Number.POSITIVE_INFINITY;
    const targetBitrateBps = Math.max(0, signals.targetBitrateBps ?? 0);
    const sendReady = targetBitrateBps <= 0 ||
        signals.sendBitrateBps >= targetBitrateBps * 0.85;
    return (signals.lossEwma < thresholds.stableLossRate &&
        signals.rttEwma < thresholds.stableRttMs &&
        rawJitterMs < recoveryJitterMs &&
        sendReady &&
        !signals.bandwidthLimited &&
        !signals.cpuLimited);
}
class PublisherQosController {
    constructor(options) {
        this.seq = 0;
        this.running = false;
        this.cpuSampleCount = 0;
        this.recoveryProbeSuccessStreak = 0;
        this.recoveryProbeGraceUntilMs = undefined;
        this.clock = options.clock;
        this.profile = options.profile;
        this.statsProvider = options.statsProvider;
        this.actionExecutor = options.actionExecutor;
        this.signalChannel = options.signalChannel;
        this.trackId = options.trackId;
        this.kind = options.kind;
        this.producerId = options.producerId;
        this.currentLevel = Math.max(0, Math.floor(options.initialLevel ?? 0));
        this.inAudioOnlyMode = options.initialAudioOnlyMode === true;
        this.snapshotIntervalMs =
            options.snapshotIntervalMs ?? options.profile.snapshotIntervalMs;
        this.sampleIntervalMs =
            options.sampleIntervalMs ?? options.profile.sampleIntervalMs;
        this.allowAudioOnly = options.allowAudioOnly ?? true;
        this.allowVideoPause = options.allowVideoPause ?? true;
        this.stateContext = (0, stateMachine_1.createInitialQosStateMachineContext)(this.clock.nowMs());
        this.trace = new trace_1.QosTraceBuffer(options.traceCapacity);
        this.sampler = new sampler_1.IntervalQosSampler(this.clock, options.statsProvider, this.sampleIntervalMs);
        this.signalChannel?.onPolicy?.(policy => {
            this.handlePolicy(policy);
        });
        this.signalChannel?.onOverride?.(override => {
            this.handleOverride(override);
        });
    }
    get isRunning() {
        return this.running;
    }
    get level() {
        return this.currentLevel;
    }
    get state() {
        return this.stateContext.state;
    }
    getTraceEntries() {
        return this.trace.getTraceEntries();
    }
    getLastSampleError() {
        return this.lastSampleError;
    }
    getRuntimeSettings() {
        return {
            sampleIntervalMs: this.sampleIntervalMs,
            snapshotIntervalMs: this.snapshotIntervalMs,
            allowAudioOnly: this.allowAudioOnly,
            allowVideoPause: this.allowVideoPause,
            probeActive: this.probeContext?.active === true,
        };
    }
    getCoordinationOverride() {
        return this.coordinationOverride
            ? { ...this.coordinationOverride }
            : undefined;
    }
    getTrackState() {
        return {
            trackId: this.trackId,
            source: this.profile.source,
            kind: this.kind,
            state: this.stateContext.state,
            quality: this.previousSignals
                ? (0, stateMachine_1.mapStateToQuality)(this.stateContext.state, this.previousSignals)
                : 'excellent',
            level: this.currentLevel,
            inAudioOnlyMode: this.inAudioOnlyMode,
        };
    }
    setCoordinationOverride(override) {
        if (!override) {
            this.coordinationOverride = undefined;
            return;
        }
        const normalized = {};
        if (typeof override.maxLevelClamp === 'number') {
            normalized.maxLevelClamp = Math.max(0, Math.floor(override.maxLevelClamp));
        }
        if (override.disableRecovery === true) {
            normalized.disableRecovery = true;
        }
        if (override.forceAudioOnly === true) {
            normalized.forceAudioOnly = true;
        }
        if (override.pauseUpstream === true) {
            normalized.pauseUpstream = true;
        }
        if (override.resumeUpstream === true) {
            normalized.resumeUpstream = true;
        }
        this.coordinationOverride =
            Object.keys(normalized).length > 0 ? normalized : undefined;
    }
    start() {
        if (this.running) {
            return;
        }
        this.running = true;
        this.sampler.start(async (snapshot) => {
            await this.processSample(snapshot);
        }, error => {
            this.lastSampleError = error;
        });
    }
    stop() {
        if (!this.running) {
            return;
        }
        this.running = false;
        this.sampler.stop();
    }
    async sampleNow() {
        const snapshot = await this.sampler.sampleOnce();
        await this.processSample(snapshot);
    }
    handlePolicy(policy) {
        this.sampleIntervalMs = policy.sampleIntervalMs;
        this.snapshotIntervalMs = policy.snapshotIntervalMs;
        this.allowAudioOnly = policy.allowAudioOnly;
        this.allowVideoPause = policy.allowVideoPause;
        this.sampler.updateIntervalMs(policy.sampleIntervalMs);
        if (policy.profiles) {
            const profileName = policy.profiles[this.profile.source];
            if (profileName && profileName !== this.profile.name) {
                const resolved = require('./profiles').resolveProfileByName(this.profile.source, profileName);
                if (resolved) {
                    this.profile = resolved;
                }
            }
        }
    }
    handleOverride(override) {
        if (override.scope === 'track' && override.trackId !== this.trackId) {
            return;
        }
        const nowMs = this.clock.nowMs();
        const ttlMs = Math.max(0, Math.floor(override.ttlMs));
        if (ttlMs === 0) {
            // Clear: remove all overrides whose reason shares the same prefix
            // before the trailing "_clear" / "_expired" suffix.
            // e.g. "server_auto_clear" clears "server_auto_poor", "server_auto_lost".
            //      "server_room_pressure_clear" clears "server_room_pressure".
            //      "server_ttl_expired" clears everything (server-side TTL fallback).
            if (this.activeOverrides) {
                const reason = override.reason || '';
                if (reason === 'server_ttl_expired') {
                    this.activeOverrides.clear();
                } else if (reason.startsWith('downlink_v2_') || reason.startsWith('downlink_v3_')) {
                    // Server-generated downlink v2/v3 track clears must only remove
                    // their own coordination overrides, not unrelated app/manual ones.
                    for (const key of [...this.activeOverrides.keys()]) {
                        if (key.startsWith('downlink_v2_') || key.startsWith('downlink_v3_')) {
                            this.activeOverrides.delete(key);
                        }
                    }
                } else if (!reason.startsWith('server_')) {
                    // Manual clear semantics: ttlMs=0 from app/manual control should
                    // clear all non-server overrides, regardless of the exact reason
                    // label used for the clear request.
                    for (const key of [...this.activeOverrides.keys()]) {
                        if (!key.startsWith('server_')) {
                            this.activeOverrides.delete(key);
                        }
                    }
                } else {
                    const prefix = reason.replace(/_clear$/, '').replace(/_expired$/, '');
                    for (const key of [...this.activeOverrides.keys()]) {
                        if (key.startsWith(prefix)) {
                            this.activeOverrides.delete(key);
                        }
                    }
                }
            }
        } else {
            const key = override.reason || '_default';
            if (!this.activeOverrides) this.activeOverrides = new Map();
            this.activeOverrides.set(key, {
                data: override,
                expiresAtMs: nowMs + ttlMs,
            });
        }
        this.activeOverride = this._mergeOverrides(nowMs);
    }
    _mergeOverrides(nowMs) {
        if (!this.activeOverrides || this.activeOverrides.size === 0) return undefined;
        let merged;
        for (const [key, entry] of this.activeOverrides) {
            if (nowMs >= entry.expiresAtMs) {
                this.activeOverrides.delete(key);
                continue;
            }
            if (!merged) {
                merged = { ...entry.data };
                merged._expiresAtMs = entry.expiresAtMs;
                continue;
            }
            // Merge: take the most restrictive values.
            if (typeof entry.data.maxLevelClamp === 'number') {
                merged.maxLevelClamp = typeof merged.maxLevelClamp === 'number'
                    ? Math.min(merged.maxLevelClamp, entry.data.maxLevelClamp)
                    : entry.data.maxLevelClamp;
            }
            if (entry.data.forceAudioOnly === true) merged.forceAudioOnly = true;
            if (entry.data.disableRecovery === true) merged.disableRecovery = true;
            if (entry.data.pauseUpstream === true) merged.pauseUpstream = true;
            if (entry.data.resumeUpstream === true) merged.resumeUpstream = true;
            merged._expiresAtMs = Math.min(merged._expiresAtMs, entry.expiresAtMs);
        }
        if (!merged) return undefined;
        const expiresAtMs = merged._expiresAtMs;
        delete merged._expiresAtMs;
        return { data: merged, expiresAtMs };
    }
    getActiveOverride(nowMs) {
        // Re-merge to purge expired entries.
        this.activeOverride = this._mergeOverrides(nowMs);
        if (!this.activeOverride) {
            return undefined;
        }
        if (nowMs >= this.activeOverride.expiresAtMs) {
            this.activeOverride = undefined;
            return undefined;
        }
        return this.activeOverride.data;
    }
    getTransitionSignals(signals, override, nowMs) {
        const { thresholds } = this.profile;
        const stableJitterMs = Number.isFinite(thresholds.stableJitterMs)
            ? thresholds.stableJitterMs
            : undefined;
        const networkRecovered = signals.lossEwma < thresholds.stableLossRate &&
            signals.rttEwma < thresholds.stableRttMs &&
            !signals.bandwidthLimited &&
            !signals.cpuLimited;
        let transitionSignals = signals;
        const recoveryProbeGraceActive = typeof this.recoveryProbeGraceUntilMs === 'number' &&
            nowMs < this.recoveryProbeGraceUntilMs;
        if ((this.probeContext?.active === true || recoveryProbeGraceActive) && networkRecovered) {
            // Probe success ignores jitter-only spikes today. Keep state transitions
            // aligned so a local bitrate relaxation does not immediately flap back to
            // congested on encoder/BWE settling noise before the probe completes or
            // immediately after a successful recovery upgrade settles.
            transitionSignals = {
                ...transitionSignals,
                jitterEwma: typeof stableJitterMs === 'number'
                    ? Math.min(transitionSignals.jitterEwma, Math.max(0, stableJitterMs - 0.001))
                    : transitionSignals.jitterEwma,
            };
        }
        if (!this.inAudioOnlyMode || this.stateContext.state !== 'congested') {
            return transitionSignals;
        }
        if (override?.forceAudioOnly === true ||
            this.coordinationOverride?.forceAudioOnly === true) {
            return transitionSignals;
        }
        if (!networkRecovered) {
            return transitionSignals;
        }
        // Audio-only pauses the upstream track, so bitrate utilization drops to zero.
        // Ignore that local pause effect for state transitions so recovery probing can begin.
        return {
            ...transitionSignals,
            bitrateUtilization: Math.max(transitionSignals.bitrateUtilization, thresholds.stableBitrateUtilization),
            jitterEwma: typeof stableJitterMs === 'number'
                ? Math.min(transitionSignals.jitterEwma, Math.max(0, stableJitterMs - 0.001))
                : transitionSignals.jitterEwma,
        };
    }
    async processSample(snapshot) {
        const nowMs = this.clock.nowMs();
        const stateBefore = this.stateContext.state;
        const override = this.getActiveOverride(nowMs);
        const activeOverride = Boolean(override);
        const coordinationOverride = this.coordinationOverride;
        const combinedClampLevels = [
            override?.maxLevelClamp,
            coordinationOverride?.maxLevelClamp,
        ].filter((value) => typeof value === 'number');
        const combinedClampLevel = combinedClampLevels.length > 0
            ? Math.min(...combinedClampLevels)
            : undefined;
        const disableRecovery = coordinationOverride?.disableRecovery === true ||
            override?.disableRecovery === true ||
            (typeof this.recoverySuppressedUntilMs === 'number' &&
                nowMs < this.recoverySuppressedUntilMs);
        if (snapshot.qualityLimitationReason === 'cpu') {
            this.cpuSampleCount += 1;
        }
        else {
            this.cpuSampleCount = 0;
        }
        const signals = (0, signals_1.deriveSignals)(snapshot, this.previousSnapshot, this.previousSignals, {
            reasonContext: {
                activeOverride,
                cpuSampleCount: this.cpuSampleCount,
            },
        });
        const transitionSignals = this.getTransitionSignals(signals, override, nowMs);
        const transition = (0, stateMachine_1.evaluateStateTransition)(this.stateContext, transitionSignals, this.profile, nowMs);
        this.stateContext = transition.context;
        if (this.probeContext) {
            const probeEvaluation = (0, probe_1.evaluateProbe)(this.probeContext, signals, this.profile);
            if (probeEvaluation.result === 'failed') {
                await this.rollbackProbe(signals, nowMs, stateBefore);
                this.probeContext = undefined;
                this.recoveryProbeSuccessStreak = 0;
                this.recoveryProbeGraceUntilMs = undefined;
                this.recoverySuppressedUntilMs =
                    nowMs + this.profile.recoveryCooldownMs;
                return;
            }
            if (probeEvaluation.result === 'successful') {
                const strongRecoveryProbe = isStrongRecoverySignal(signals, this.profile) &&
                    (probeEvaluation.context?.badSamples ?? 0) === 0 &&
                    (stateBefore === 'recovering' ||
                        this.stateContext.state === 'recovering' ||
                        this.recoveryProbeSuccessStreak > 0) &&
                    this.currentLevel > 0;
                this.recoveryProbeSuccessStreak = strongRecoveryProbe
                    ? this.recoveryProbeSuccessStreak + 1
                    : 0;
                this.recoveryProbeGraceUntilMs = strongRecoveryProbe
                    ? nowMs + this.sampleIntervalMs * 3
                    : undefined;
                this.probeContext = undefined;
            }
            else {
                this.probeContext = probeEvaluation.context;
            }
        }
        if (this.probeContext?.active !== true &&
            (this.stateContext.state === 'congested' || this.currentLevel === 0)) {
            this.recoveryProbeSuccessStreak = 0;
        }
        const probeInProgress = this.probeContext?.active === true;
        const plannedActions = probeInProgress
            ? [
                {
                    type: 'noop',
                    level: this.currentLevel,
                    reason: 'probe-in-progress',
                },
            ]
            : this.stateContext.state === 'recovering' && !disableRecovery
                ? (0, planner_1.planRecovery)({
                    source: this.profile.source,
                    profile: this.profile,
                    state: this.stateContext.state,
                    currentLevel: this.currentLevel,
                    overrideClampLevel: combinedClampLevel,
                    inAudioOnlyMode: this.inAudioOnlyMode,
                    signals,
                })
                : (0, planner_1.planActions)({
                    source: this.profile.source,
                    profile: this.profile,
                    state: this.stateContext.state === 'recovering'
                        ? 'early_warning'
                        : this.stateContext.state,
                    currentLevel: this.currentLevel,
                    overrideClampLevel: combinedClampLevel,
                    inAudioOnlyMode: this.inAudioOnlyMode,
                    signals,
                });
        const filteredActions = this.filterActions(plannedActions, override);
        await this.executeActions(filteredActions, signals, nowMs, stateBefore, this.stateContext.state);
        const shouldPublish = transition.transitioned ||
            this.lastPublishedAtMs === undefined ||
            nowMs - this.lastPublishedAtMs >= this.snapshotIntervalMs;
        if (shouldPublish) {
            await this.publishSnapshot(snapshot, signals, transition.quality, nowMs);
            this.lastPublishedAtMs = nowMs;
        }
        this.previousSnapshot = snapshot;
        this.previousSignals = signals;
    }
    filterActions(actions, override) {
        const audioOnlyAllowed = this.allowAudioOnly && this.allowVideoPause;
        const maxLevel = Math.max(0, this.profile.levelCount - 1);
        const filtered = [];
        for (const action of actions) {
            if ((action.type === 'enterAudioOnly' || action.type === 'exitAudioOnly') &&
                !audioOnlyAllowed) {
                continue;
            }
            filtered.push(action);
        }
        // v3: pauseUpstream from server demand aggregation
        const pauseUpstreamActive = override?.pauseUpstream === true ||
            this.coordinationOverride?.pauseUpstream === true;
        const resumeUpstreamActive = override?.resumeUpstream === true ||
            this.coordinationOverride?.resumeUpstream === true;
        if (pauseUpstreamActive &&
            this.inAudioOnlyMode !== true &&
            this.profile.source === 'camera' &&
            audioOnlyAllowed) {
            filtered.unshift({
                type: 'enterAudioOnly',
                level: this.currentLevel,
                reason: 'downlink_v3_zero_demand_pause',
            });
        }
        if (resumeUpstreamActive &&
            this.inAudioOnlyMode === true &&
            this.profile.source === 'camera' &&
            audioOnlyAllowed &&
            !pauseUpstreamActive) {
            filtered.unshift({
                type: 'exitAudioOnly',
                level: this.currentLevel,
                reason: 'downlink_v3_demand_resumed',
            });
        }
        if (audioOnlyAllowed &&
            override?.forceAudioOnly &&
            this.inAudioOnlyMode !== true) {
            if (this.profile.source === 'camera') {
                filtered.unshift({
                    type: 'enterAudioOnly',
                    level: this.currentLevel,
                });
            }
        }
        if (audioOnlyAllowed &&
            this.coordinationOverride?.forceAudioOnly === true &&
            this.inAudioOnlyMode !== true &&
            this.profile.source === 'camera') {
            filtered.unshift({
                type: 'enterAudioOnly',
                level: this.currentLevel,
            });
        }
        const forceAudioOnlyActive = override?.forceAudioOnly === true ||
            this.coordinationOverride?.forceAudioOnly === true;
        const overrideDrivenAudioOnly = this.profile.source === 'camera' &&
            this.inAudioOnlyMode === true &&
            this.currentLevel < maxLevel;
        if (audioOnlyAllowed &&
            overrideDrivenAudioOnly &&
            !forceAudioOnlyActive &&
            !pauseUpstreamActive &&
            !filtered.some(action => action.type === 'exitAudioOnly')) {
            filtered.unshift({
                type: 'exitAudioOnly',
                level: this.currentLevel,
            });
        }
        if (filtered.length === 0) {
            return [
                {
                    type: 'noop',
                    level: this.currentLevel,
                    reason: 'filtered-by-policy',
                },
            ];
        }
        return filtered;
    }
    async executeActions(actions, signals, nowMs, stateBefore, stateAfter) {
        const levelBeforeActions = this.currentLevel;
        const audioOnlyBeforeActions = this.inAudioOnlyMode;
        for (const action of actions) {
            let applied = false;
            if (action.type === 'noop') {
                applied = true;
            }
            else {
                try {
                    const result = await this.actionExecutor.execute(action);
                    applied = wasActionApplied(result);
                }
                catch (error) {
                    this.lastSampleError = toError(error);
                    applied = false;
                }
            }
            this.lastAction = { type: action.type, applied };
            if (applied) {
                this.currentLevel = action.level;
                if (action.type === 'enterAudioOnly') {
                    this.inAudioOnlyMode = true;
                }
                else if (action.type === 'exitAudioOnly') {
                    this.inAudioOnlyMode = false;
                }
            }
            this.trace.appendTraceEntry({
                seq: this.seq + 1,
                tsMs: nowMs,
                trackId: this.trackId,
                source: this.profile.source,
                stateBefore,
                stateAfter,
                reason: signals.reason,
                signals: {
                    sendBitrateBps: signals.sendBitrateBps,
                    targetBitrateBps: signals.targetBitrateBps,
                    lossRate: signals.lossRate,
                    rttMs: signals.rttMs,
                    jitterMs: signals.jitterMs,
                },
                plannedAction: {
                    type: action.type,
                    level: action.level,
                },
                applied,
                probe: this.probeContext
                    ? {
                        active: true,
                        previousLevel: this.probeContext.previousLevel,
                        targetLevel: this.probeContext.targetLevel,
                    }
                    : undefined,
            });
        }
        if (actions.some(action => action.type !== 'noop') &&
            (this.currentLevel < levelBeforeActions ||
                (audioOnlyBeforeActions && !this.inAudioOnlyMode))) {
            let requiredHealthySamples = this.recoveryProbeSuccessStreak > 0 &&
                isStrongRecoverySignal(signals, this.profile)
                ? 2
                : 3;
            // Tail recovery can otherwise stall at stable/L1 long enough to miss
            // the final L0 restore inside short recovery windows such as T9.
            if (stateAfter === 'stable' && this.currentLevel === 1) {
                requiredHealthySamples = Math.min(requiredHealthySamples, 2);
            }
            this.probeContext = (0, probe_1.beginProbe)(levelBeforeActions, this.currentLevel, nowMs, audioOnlyBeforeActions, this.inAudioOnlyMode, {
                requiredHealthySamples,
            });
        }
    }
    async rollbackProbe(signals, nowMs, stateBefore) {
        if (!this.probeContext) {
            return;
        }
        const revertActions = (0, planner_2.planActionsForLevel)({
            source: this.profile.source,
            profile: this.profile,
            state: 'congested',
            currentLevel: this.currentLevel,
            overrideClampLevel: undefined,
            inAudioOnlyMode: this.inAudioOnlyMode,
            signals,
        }, this.probeContext.previousLevel);
        for (const action of revertActions) {
            const result = await this.actionExecutor.execute(action);
            const applied = wasActionApplied(result);
            if (applied) {
                this.currentLevel = action.level;
                if (action.type === 'enterAudioOnly') {
                    this.inAudioOnlyMode = true;
                }
                else if (action.type === 'exitAudioOnly') {
                    this.inAudioOnlyMode = false;
                }
            }
            this.trace.appendTraceEntry({
                seq: this.seq + 1,
                tsMs: nowMs,
                trackId: this.trackId,
                source: this.profile.source,
                stateBefore,
                stateAfter: 'congested',
                reason: signals.reason,
                signals: {
                    sendBitrateBps: signals.sendBitrateBps,
                    targetBitrateBps: signals.targetBitrateBps,
                    lossRate: signals.lossRate,
                    rttMs: signals.rttMs,
                    jitterMs: signals.jitterMs,
                },
                plannedAction: {
                    type: action.type,
                    level: action.level,
                },
                applied,
                probe: {
                    active: false,
                    result: 'failed',
                    previousLevel: this.probeContext.previousLevel,
                    targetLevel: this.probeContext.targetLevel,
                },
            });
        }
        this.stateContext = {
            ...this.stateContext,
            state: 'congested',
            enteredAtMs: nowMs,
            lastCongestedAtMs: nowMs,
        };
    }
    async publishSnapshot(rawSnapshot, signals, quality, nowMs) {
        if (!this.signalChannel) {
            return;
        }
        const nextSeq = this.seq >= constants_1.QOS_MAX_SEQ ? 0 : this.seq + 1;
        this.seq = nextSeq;
        const payload = {
            schema: constants_1.QOS_CLIENT_SCHEMA_V1,
            seq: nextSeq,
            tsMs: nowMs,
            peerState: {
                mode: resolvePeerMode(this.inAudioOnlyMode, this.kind),
                quality,
                stale: false,
            },
            tracks: [
                {
                    localTrackId: this.trackId,
                    producerId: this.producerId,
                    kind: this.kind,
                    source: this.profile.source,
                    state: this.stateContext.state,
                    reason: signals.reason,
                    quality,
                    ladderLevel: this.currentLevel,
                    signals: {
                        sendBitrateBps: signals.sendBitrateBps,
                        targetBitrateBps: signals.targetBitrateBps,
                        lossRate: signals.lossRate,
                        rttMs: signals.rttMs,
                        jitterMs: signals.jitterMs,
                        frameWidth: rawSnapshot.frameWidth,
                        frameHeight: rawSnapshot.frameHeight,
                        framesPerSecond: rawSnapshot.framesPerSecond,
                        qualityLimitationReason: rawSnapshot.qualityLimitationReason,
                    },
                    lastAction: this.lastAction,
                },
            ],
        };
        try {
            await this.signalChannel.publishSnapshot(payload);
        }
        catch (error) {
            this.lastSampleError = toError(error);
        }
    }
}
exports.PublisherQosController = PublisherQosController;
