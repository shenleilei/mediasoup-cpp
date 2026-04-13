export interface DownlinkSubscriptionHint {
    consumerId: string;
    producerId: string;
    ssrc?: number;
    trackId?: string;
    mid?: string;
    visible: boolean;
    pinned: boolean;
    activeSpeaker: boolean;
    isScreenShare: boolean;
    targetWidth: number;
    targetHeight: number;
}

export interface DownlinkTransportStats {
    availableIncomingBitrate: number;
    currentRoundTripTime: number;
}

export interface DownlinkSubscriptionStats {
    consumerId: string;
    producerId: string;
    packetsLost: number;
    jitter: number;
    framesPerSecond: number;
    frameWidth: number;
    frameHeight: number;
    freezeRate: number;
}

export interface DownlinkSnapshot {
    schema: string;
    seq: number;
    tsMs: number;
    subscriberPeerId: string;
    transport: DownlinkTransportStats;
    subscriptions: Array<DownlinkSubscriptionHint & DownlinkSubscriptionStats>;
}
