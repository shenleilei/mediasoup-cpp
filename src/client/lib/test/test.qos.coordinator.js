"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const coordinator_1 = require("../qos/coordinator");
function track(overrides = {}) {
    return {
        trackId: 'track',
        source: 'camera',
        kind: 'video',
        state: 'stable',
        quality: 'excellent',
        level: 0,
        ...overrides,
    };
}
test('peer qos coordinator prioritizes audio then screenshare then camera', () => {
    const decision = (0, coordinator_1.buildPeerQosDecision)([
        track({ trackId: 'camera', source: 'camera' }),
        track({ trackId: 'audio', source: 'audio', kind: 'audio' }),
        track({ trackId: 'screen', source: 'screenShare' }),
    ]);
    expect(decision.prioritizedTrackIds).toEqual(['audio', 'screen', 'camera']);
    expect(decision.keepAudioAlive).toBe(true);
    expect(decision.preferScreenShare).toBe(true);
});
test('peer qos coordinator sacrifices camera first when screenshare exists', () => {
    const decision = (0, coordinator_1.buildPeerQosDecision)([
        track({ trackId: 'camera', source: 'camera' }),
        track({ trackId: 'screen', source: 'screenShare' }),
        track({ trackId: 'audio', source: 'audio', kind: 'audio' }),
    ]);
    expect(decision.sacrificialTrackIds).toEqual(['camera']);
});
test('peer qos coordinator derives peer quality from worst track', () => {
    const decision = (0, coordinator_1.buildPeerQosDecision)([
        track({ trackId: 'camera', quality: 'excellent' }),
        track({
            trackId: 'audio',
            source: 'audio',
            kind: 'audio',
            quality: 'poor',
        }),
    ]);
    expect(decision.peerQuality).toBe('poor');
    expect(decision.allowVideoRecovery).toBe(false);
});
test('peer qos coordinator handles empty set', () => {
    const decision = (0, coordinator_1.buildPeerQosDecision)([]);
    expect(decision.peerQuality).toBe('lost');
    expect(decision.prioritizedTrackIds).toEqual([]);
    expect(decision.sacrificialTrackIds).toEqual([]);
});
test('peer qos coordinator class tracks upsert/remove and decision', () => {
    const coordinator = new coordinator_1.PeerQosCoordinator();
    coordinator.upsertTrack(track({ trackId: 'audio', source: 'audio', kind: 'audio' }));
    coordinator.upsertTrack(track({ trackId: 'camera', source: 'camera' }));
    expect(coordinator.getDecision().prioritizedTrackIds).toEqual([
        'audio',
        'camera',
    ]);
    coordinator.removeTrack('camera');
    expect(coordinator.getDecision().prioritizedTrackIds).toEqual(['audio']);
    coordinator.clear();
    expect(coordinator.getDecision().prioritizedTrackIds).toEqual([]);
});
