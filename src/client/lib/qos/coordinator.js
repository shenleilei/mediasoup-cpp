"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.PeerQosCoordinator = void 0;
exports.buildPeerQosDecision = buildPeerQosDecision;
const qualityRank = {
    excellent: 3,
    good: 2,
    poor: 1,
    lost: 0,
};
function sourcePriority(source) {
    switch (source) {
        case 'audio': {
            return 0;
        }
        case 'screenShare': {
            return 1;
        }
        case 'camera': {
            return 2;
        }
    }
}
function minQuality(a, b) {
    return qualityRank[a] <= qualityRank[b] ? a : b;
}
function prioritizeTracks(tracks) {
    return [...tracks].sort((left, right) => {
        const leftPriority = sourcePriority(left.source);
        const rightPriority = sourcePriority(right.source);
        if (leftPriority !== rightPriority) {
            return leftPriority - rightPriority;
        }
        return left.trackId.localeCompare(right.trackId);
    });
}
function buildPeerQosDecision(tracks) {
    if (tracks.length === 0) {
        return {
            peerQuality: 'lost',
            prioritizedTrackIds: [],
            sacrificialTrackIds: [],
            keepAudioAlive: false,
            preferScreenShare: false,
            allowVideoRecovery: false,
        };
    }
    const prioritizedTracks = prioritizeTracks(tracks);
    const hasAudio = prioritizedTracks.some(track => track.source === 'audio');
    const hasScreenShare = prioritizedTracks.some(track => track.source === 'screenShare');
    let peerQuality = prioritizedTracks[0].quality;
    for (const track of prioritizedTracks.slice(1)) {
        peerQuality = minQuality(peerQuality, track.quality);
    }
    const sacrificialTracks = prioritizedTracks
        .filter(track => {
        if (track.source === 'audio') {
            return false;
        }
        if (hasScreenShare) {
            return track.source === 'camera';
        }
        return hasAudio || track.source === 'camera';
    })
        .sort((left, right) => sourcePriority(right.source) - sourcePriority(left.source));
    return {
        peerQuality,
        prioritizedTrackIds: prioritizedTracks.map(track => track.trackId),
        sacrificialTrackIds: sacrificialTracks.map(track => track.trackId),
        keepAudioAlive: hasAudio,
        preferScreenShare: hasScreenShare,
        allowVideoRecovery: peerQuality !== 'poor' &&
            peerQuality !== 'lost' &&
            !prioritizedTracks.some(track => track.source === 'audio' && track.quality === 'poor'),
    };
}
class PeerQosCoordinator {
    constructor() {
        this.tracks = new Map();
    }
    upsertTrack(track) {
        this.tracks.set(track.trackId, track);
    }
    removeTrack(trackId) {
        this.tracks.delete(trackId);
    }
    clear() {
        this.tracks.clear();
    }
    getTracks() {
        return Array.from(this.tracks.values());
    }
    getDecision() {
        return buildPeerQosDecision(this.getTracks());
    }
}
exports.PeerQosCoordinator = PeerQosCoordinator;
