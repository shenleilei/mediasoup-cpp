export type QosSource = 'camera' | 'screenShare' | 'audio';
export type QosReason = 'network' | 'cpu' | 'manual' | 'server_override' | 'unknown';
export type QosState = 'stable' | 'early_warning' | 'congested' | 'recovering';
export type QosQuality = 'excellent' | 'good' | 'poor' | 'lost';
export type QosSchemaName = 'mediasoup.qos.client.v1' | 'mediasoup.qos.policy.v1' | 'mediasoup.qos.override.v1';
export type QosTrackKind = 'audio' | 'video';
export type QosTrackMode = 'audio-only' | 'audio-video' | 'video-only';
export type QosActionType = 'setEncodingParameters' | 'setMaxSpatialLayer' | 'enterAudioOnly' | 'exitAudioOnly' | 'pauseUpstream' | 'resumeUpstream' | 'noop';
export type QosQualityLimitationReason = 'bandwidth' | 'cpu' | 'other' | 'none' | 'unknown';
export type RawSenderSnapshot = {
    timestampMs: number;
    trackId?: string;
    producerId?: string;
    source: QosSource;
    kind: QosTrackKind;
    bytesSent?: number;
    packetsSent?: number;
    packetsLost?: number;
    retransmittedPacketsSent?: number;
    targetBitrateBps?: number;
    configuredBitrateBps?: number;
    roundTripTimeMs?: number;
    jitterMs?: number;
    frameWidth?: number;
    frameHeight?: number;
    framesPerSecond?: number;
    qualityLimitationReason?: QosQualityLimitationReason;
    qualityLimitationDurationsSec?: Record<string, number>;
};
export type DerivedQosSignals = {
    packetsSentDelta: number;
    packetsLostDelta: number;
    retransmittedPacketsSentDelta: number;
    sendBitrateBps: number;
    targetBitrateBps: number;
    bitrateUtilization: number;
    lossRate: number;
    lossEwma: number;
    rttMs: number;
    rttEwma: number;
    jitterMs: number;
    jitterEwma: number;
    cpuLimited: boolean;
    bandwidthLimited: boolean;
    reason: QosReason;
};
export type ClientQosPeerState = {
    mode: QosTrackMode;
    quality: QosQuality;
    stale: boolean;
};
export type ClientQosTrackSignals = {
    sendBitrateBps: number;
    targetBitrateBps?: number;
    lossRate?: number;
    rttMs?: number;
    jitterMs?: number;
    frameWidth?: number;
    frameHeight?: number;
    framesPerSecond?: number;
    qualityLimitationReason?: QosQualityLimitationReason;
};
export type ClientQosTrackAction = {
    type: QosActionType;
    applied: boolean;
};
export type ClientQosTrackSnapshot = {
    localTrackId: string;
    producerId?: string;
    kind: QosTrackKind;
    source: QosSource;
    state: QosState;
    reason: QosReason;
    quality: QosQuality;
    ladderLevel: number;
    signals: ClientQosTrackSignals;
    lastAction?: ClientQosTrackAction;
};
export type ClientQosSnapshot = {
    schema: 'mediasoup.qos.client.v1';
    seq: number;
    tsMs: number;
    peerState: ClientQosPeerState;
    tracks: ClientQosTrackSnapshot[];
};
export type QosConnectionQualityUpdate = {
    quality: QosQuality | 'unknown';
    stale: boolean;
    lost: boolean;
    lastUpdatedMs?: number;
};
export type QosRoomState = {
    peerCount: number;
    stalePeers: number;
    poorPeers: number;
    lostPeers: number;
    quality: QosQuality | 'unknown';
};
export type QosPolicyProfiles = {
    camera: string;
    screenShare: string;
    audio: string;
};
export type QosPolicy = {
    schema: 'mediasoup.qos.policy.v1';
    sampleIntervalMs: number;
    snapshotIntervalMs: number;
    allowAudioOnly: boolean;
    allowVideoPause: boolean;
    profiles: QosPolicyProfiles;
};
export type QosOverrideScope = 'peer' | 'track';
export type QosOverride = {
    schema: 'mediasoup.qos.override.v1';
    scope: QosOverrideScope;
    trackId?: string | null;
    maxLevelClamp?: number;
    forceAudioOnly?: boolean;
    disableRecovery?: boolean;
    ttlMs: number;
    reason: string;
};
export type QosTraceEntry = {
    seq: number;
    tsMs: number;
    trackId: string;
    source: QosSource;
    stateBefore: QosState;
    stateAfter: QosState;
    reason: QosReason;
    signals: Partial<ClientQosTrackSignals>;
    plannedAction: {
        type: QosActionType;
        level?: number;
    };
    applied: boolean;
    probe?: {
        active: boolean;
        result?: 'successful' | 'failed' | 'inconclusive';
        previousLevel?: number;
        targetLevel?: number;
    };
};
export type QosEncodingParameters = {
    maxBitrateBps?: number;
    maxFramerate?: number;
    scaleResolutionDownBy?: number;
    adaptivePtime?: boolean;
};
export type QosThresholds = {
    warnLossRate: number;
    congestedLossRate: number;
    warnRttMs: number;
    congestedRttMs: number;
    warnJitterMs?: number;
    congestedJitterMs?: number;
    warnBitrateUtilization: number;
    congestedBitrateUtilization: number;
    stableLossRate: number;
    stableRttMs: number;
    stableJitterMs?: number;
    stableBitrateUtilization: number;
};
export type QosLadderStep = {
    level: number;
    description: string;
    encodingParameters?: QosEncodingParameters;
    spatialLayer?: number;
    enterAudioOnly?: boolean;
    exitAudioOnly?: boolean;
    pauseUpstream?: boolean;
    resumeUpstream?: boolean;
};
export type QosProfile = {
    name: string;
    source: QosSource;
    levelCount: number;
    sampleIntervalMs: number;
    snapshotIntervalMs: number;
    recoveryCooldownMs: number;
    thresholds: QosThresholds;
    ladder: QosLadderStep[];
};
export type QosStateMachineContext = {
    state: QosState;
    enteredAtMs: number;
    lastCongestedAtMs?: number;
    lastRecoveryAtMs?: number;
    consecutiveHealthySamples: number;
    consecutiveRecoverySamples: number;
    consecutiveFastRecoverySamples?: number;
    consecutiveWarningSamples: number;
    consecutiveCongestedSamples: number;
};
export type QosStateTransitionResult = {
    context: QosStateMachineContext;
    quality: QosQuality;
    transitioned: boolean;
};
export type PlannedQosAction = {
    type: 'setEncodingParameters';
    level: number;
    encodingParameters: QosEncodingParameters;
} | {
    type: 'setMaxSpatialLayer';
    level: number;
    spatialLayer: number;
} | {
    type: 'enterAudioOnly';
    level: number;
} | {
    type: 'exitAudioOnly';
    level: number;
} | {
    type: 'pauseUpstream';
    level: number;
} | {
    type: 'resumeUpstream';
    level: number;
} | {
    type: 'noop';
    level: number;
    reason?: string;
};
export type QosPlannerInput = {
    source: QosSource;
    profile: QosProfile;
    state: QosState;
    currentLevel: number;
    previousLevel?: number;
    overrideClampLevel?: number;
    inAudioOnlyMode?: boolean;
    signals: DerivedQosSignals;
};
export type QosProbeContext = {
    active: boolean;
    startedAtMs: number;
    previousLevel: number;
    targetLevel: number;
    previousAudioOnlyMode: boolean;
    targetAudioOnlyMode: boolean;
    healthySamples: number;
    badSamples: number;
    requiredHealthySamples?: number;
    requiredBadSamples?: number;
};
export type QosProbeResult = 'successful' | 'failed' | 'inconclusive';
export type PeerQosTrackState = {
    trackId: string;
    source: QosSource;
    kind: QosTrackKind;
    state: QosState;
    quality: QosQuality;
    level: number;
    inAudioOnlyMode?: boolean;
};
export type PeerQosDecision = {
    peerQuality: QosQuality;
    prioritizedTrackIds: string[];
    sacrificialTrackIds: string[];
    keepAudioAlive: boolean;
    preferScreenShare: boolean;
    allowVideoRecovery: boolean;
};
//# sourceMappingURL=types.d.ts.map
