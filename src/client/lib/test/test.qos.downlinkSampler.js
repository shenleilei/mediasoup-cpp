"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const downlinkSampler_1 = require("../qos/downlinkSampler");
function makeStatsReport(...reports) {
    return new Map(reports.map((report, index) => [report.id || `r${index}`, report]));
}
test('downlink sampler keeps audio and video reports separated by kind', async () => {
    const sampler = new downlinkSampler_1.DownlinkSampler(null, {
        statsProvider: async () => makeStatsReport({
            id: 'video-1',
            type: 'inbound-rtp',
            kind: 'video',
            mid: 'v0',
            packetsLost: 4,
            packetsReceived: 96,
            jitter: 0.01,
            freezeCount: 2,
            framesDecoded: 100,
            framesDropped: 3,
            jitterBufferDelay: 1.2,
            jitterBufferEmittedCount: 6,
        }, {
            id: 'audio-1',
            type: 'inbound-rtp',
            kind: 'audio',
            mid: 'a0',
            packetsLost: 2,
            packetsReceived: 98,
            jitter: 0.004,
            concealedSamples: 40,
            totalSamplesReceived: 4000,
        }, {
            id: 'cp-1',
            type: 'candidate-pair',
            nominated: true,
            availableIncomingBitrate: 555000,
            currentRoundTripTime: 0.08,
        }),
    });
    sampler.setHints('consumer-video', {
        producerId: 'producer-video',
        kind: 'video',
        mid: 'v0',
        visible: true,
        pinned: false,
        activeSpeaker: false,
        isScreenShare: false,
        targetWidth: 640,
        targetHeight: 360,
    });
    sampler.setHints('consumer-audio', {
        producerId: 'producer-audio',
        kind: 'audio',
        mid: 'a0',
        visible: true,
        pinned: false,
        activeSpeaker: false,
        isScreenShare: false,
        targetWidth: 0,
        targetHeight: 0,
    });
    const first = await sampler.sample('alice');
    expect(first.transport.availableIncomingBitrate).toBe(555000);
    expect(first.transport.currentRoundTripTime).toBe(0.08);
    const video = first.subscriptions.find(item => item.consumerId === 'consumer-video');
    const audio = first.subscriptions.find(item => item.consumerId === 'consumer-audio');
    expect(video.kind).toBe('video');
    expect(video.freezeRate).toBeCloseTo(0.02, 6);
    expect(video.freezeCount).toBe(2);
    expect(video.framesDropped).toBe(3);
    expect(video.jitterBufferDelayMs).toBeCloseTo(200, 6);
    expect(audio.kind).toBe('audio');
    expect(audio.concealedSamples).toBe(40);
    expect(audio.totalSamplesReceived).toBe(4000);
});
test('downlink sampler reports interval loss percent instead of cumulative counters', async () => {
    const reports = [
        makeStatsReport({
            id: 'video-1',
            type: 'inbound-rtp',
            kind: 'video',
            mid: 'v0',
            packetsLost: 10,
            packetsReceived: 90,
            jitter: 0.01,
            framesDecoded: 100,
        }),
        makeStatsReport({
            id: 'video-1',
            type: 'inbound-rtp',
            kind: 'video',
            mid: 'v0',
            packetsLost: 15,
            packetsReceived: 135,
            jitter: 0.02,
            framesDecoded: 140,
        }),
    ];
    let index = 0;
    const sampler = new downlinkSampler_1.DownlinkSampler(null, {
        statsProvider: async () => reports[index++],
    });
    sampler.setHints('consumer-video', {
        producerId: 'producer-video',
        kind: 'video',
        mid: 'v0',
        visible: true,
        pinned: false,
        activeSpeaker: false,
        isScreenShare: false,
        targetWidth: 640,
        targetHeight: 360,
    });
    const first = await sampler.sample('alice');
    expect(first.subscriptions[0].packetsLost).toBe(0);
    const second = await sampler.sample('alice');
    expect(second.subscriptions[0].packetsLost).toBe(10);
});
test('downlink sampler fallback keeps kind isolation without explicit ids', async () => {
    const sampler = new downlinkSampler_1.DownlinkSampler(null, {
        statsProvider: async () => makeStatsReport({
            id: 'audio-1',
            type: 'inbound-rtp',
            kind: 'audio',
            packetsLost: 1,
            packetsReceived: 9,
            jitter: 0.004,
        }, {
            id: 'video-1',
            type: 'inbound-rtp',
            kind: 'video',
            packetsLost: 2,
            packetsReceived: 18,
            jitter: 0.01,
            framesDecoded: 20,
        }),
    });
    sampler.setHints('consumer-video', {
        producerId: 'producer-video',
        kind: 'video',
        visible: true,
        pinned: false,
        activeSpeaker: false,
        isScreenShare: false,
        targetWidth: 640,
        targetHeight: 360,
    });
    const snapshot = await sampler.sample('alice');
    expect(snapshot.subscriptions[0].kind).toBe('video');
    expect(snapshot.subscriptions[0].jitter).toBe(0.01);
});
