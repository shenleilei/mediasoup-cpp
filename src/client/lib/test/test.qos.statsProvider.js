"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const statsProvider_1 = require("../qos/adapters/statsProvider");
function makeAdapter(report, base) {
    return {
        getSnapshotBase: () => base ?? {
            source: 'camera',
            kind: 'video',
            producerId: 'p1',
            trackId: 't1',
            configuredBitrateBps: 900000,
        },
        getStatsReport: async () => report,
        setEncodingParameters: async () => ({ applied: true }),
        setMaxSpatialLayer: async () => ({ applied: true }),
        pauseUpstream: async () => ({ applied: true }),
        resumeUpstream: async () => ({ applied: true }),
    };
}
test('stats provider returns base snapshot when no outbound-rtp exists', async () => {
    const report = new Map();
    const provider = new statsProvider_1.ProducerSenderStatsProvider(makeAdapter(report));
    const snapshot = await provider.getSnapshot();
    expect(snapshot.source).toBe('camera');
    expect(snapshot.kind).toBe('video');
    expect(snapshot.producerId).toBe('p1');
    expect(snapshot.trackId).toBe('t1');
    expect(typeof snapshot.timestampMs).toBe('number');
    expect(snapshot.bytesSent).toBeUndefined();
    expect(snapshot.packetsSent).toBeUndefined();
});
test('stats provider parses outbound + remote-inbound metrics', async () => {
    const report = new Map();
    report.set('out1', {
        id: 'out1',
        type: 'outbound-rtp',
        timestamp: 2000,
        bytesSent: 10000,
        packetsSent: 150,
        retransmittedPacketsSent: 5,
        targetBitrate: 550000,
        frameWidth: 640,
        frameHeight: 360,
        framesPerSecond: 24,
        qualityLimitationReason: 'bandwidth',
        qualityLimitationDurations: { bandwidth: 2.5 },
        remoteId: 'r1',
    });
    report.set('r1', {
        id: 'r1',
        type: 'remote-inbound-rtp',
        timestamp: 2000,
        packetsLost: 9,
        roundTripTime: 0.2,
        jitter: 0.01,
    });
    const provider = new statsProvider_1.ProducerSenderStatsProvider(makeAdapter(report));
    const snapshot = await provider.getSnapshot();
    expect(snapshot.timestampMs).toBe(2000);
    expect(snapshot.bytesSent).toBe(10000);
    expect(snapshot.packetsSent).toBe(150);
    expect(snapshot.packetsLost).toBe(9);
    expect(snapshot.retransmittedPacketsSent).toBe(5);
    expect(snapshot.targetBitrateBps).toBe(550000);
    expect(snapshot.frameWidth).toBe(640);
    expect(snapshot.frameHeight).toBe(360);
    expect(snapshot.framesPerSecond).toBe(24);
    expect(snapshot.qualityLimitationReason).toBe('bandwidth');
    expect(snapshot.qualityLimitationDurationsSec).toEqual({ bandwidth: 2.5 });
    expect(snapshot.roundTripTimeMs).toBe(200);
    expect(snapshot.jitterMs).toBe(10);
});
test('stats provider prefers selected candidate-pair RTT over remote-inbound RTT', async () => {
    const report = new Map();
    report.set('out1', {
        id: 'out1',
        type: 'outbound-rtp',
        timestamp: 3000,
        bytesSent: 10000,
        packetsSent: 150,
        targetBitrate: 550000,
        transportId: 'transport-1',
        remoteId: 'r1',
    });
    report.set('r1', {
        id: 'r1',
        type: 'remote-inbound-rtp',
        timestamp: 3000,
        packetsLost: 9,
        roundTripTime: 0.9,
        jitter: 0.01,
    });
    report.set('transport-1', {
        id: 'transport-1',
        type: 'transport',
        selectedCandidatePairId: 'cp-1',
    });
    report.set('cp-1', {
        id: 'cp-1',
        type: 'candidate-pair',
        state: 'succeeded',
        selected: true,
        currentRoundTripTime: 0.05,
    });
    const provider = new statsProvider_1.ProducerSenderStatsProvider(makeAdapter(report));
    const snapshot = await provider.getSnapshot();
    expect(snapshot.roundTripTimeMs).toBe(50);
    expect(snapshot.jitterMs).toBe(10);
});
test('stats provider ignores stale remote-inbound RTT/jitter when timestamp does not advance', async () => {
    let report = new Map();
    const adapter = makeAdapter(new Map());
    adapter.getStatsReport = async () => report;
    const provider = new statsProvider_1.ProducerSenderStatsProvider(adapter);
    report.set('out1', {
        id: 'out1',
        type: 'outbound-rtp',
        timestamp: 2000,
        bytesSent: 10000,
        packetsSent: 150,
        targetBitrate: 550000,
        remoteId: 'r1',
    });
    report.set('r1', {
        id: 'r1',
        type: 'remote-inbound-rtp',
        timestamp: 2000,
        packetsLost: 9,
        roundTripTime: 0.12,
        jitter: 0.01,
    });
    let snapshot = await provider.getSnapshot();
    expect(snapshot.roundTripTimeMs).toBe(120);
    expect(snapshot.jitterMs).toBe(10);
    report = new Map();
    report.set('out1', {
        id: 'out1',
        type: 'outbound-rtp',
        timestamp: 2500,
        bytesSent: 12000,
        packetsSent: 180,
        targetBitrate: 550000,
        remoteId: 'r1',
    });
    report.set('r1', {
        id: 'r1',
        type: 'remote-inbound-rtp',
        timestamp: 2000,
        packetsLost: 9,
        roundTripTime: 0.9,
        jitter: 0.2,
    });
    snapshot = await provider.getSnapshot();
    expect(snapshot.roundTripTimeMs).toBeUndefined();
    expect(snapshot.jitterMs).toBeUndefined();
});
test('stats provider aggregates multi-outbound rows', async () => {
    const report = new Map();
    report.set('outA', {
        id: 'outA',
        type: 'outbound-rtp',
        timestamp: 1000,
        bytesSent: 5000,
        packetsSent: 50,
        targetBitrate: 200000,
        frameWidth: 320,
        frameHeight: 180,
        framesPerSecond: 15,
    });
    report.set('outB', {
        id: 'outB',
        type: 'outbound-rtp',
        timestamp: 1500,
        bytesSent: 7000,
        packetsSent: 70,
        targetBitrate: 300000,
        frameWidth: 640,
        frameHeight: 360,
        framesPerSecond: 30,
    });
    const provider = new statsProvider_1.ProducerSenderStatsProvider(makeAdapter(report));
    const snapshot = await provider.getSnapshot();
    expect(snapshot.timestampMs).toBe(1500);
    expect(snapshot.bytesSent).toBe(12000);
    expect(snapshot.packetsSent).toBe(120);
    expect(snapshot.targetBitrateBps).toBe(500000);
    expect(snapshot.frameWidth).toBe(640);
    expect(snapshot.frameHeight).toBe(360);
    expect(snapshot.framesPerSecond).toBe(30);
});
