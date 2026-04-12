import type { PeerQosDecision, PeerQosTrackState } from './types';
export declare function buildPeerQosDecision(tracks: PeerQosTrackState[]): PeerQosDecision;
export declare class PeerQosCoordinator {
    private readonly tracks;
    upsertTrack(track: PeerQosTrackState): void;
    removeTrack(trackId: string): void;
    clear(): void;
    getTracks(): PeerQosTrackState[];
    getDecision(): PeerQosDecision;
}
//# sourceMappingURL=coordinator.d.ts.map