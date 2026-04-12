"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const fake_mediastreamtrack_1 = require("fake-mediastreamtrack");
const clock_1 = require("../qos/clock");
const factory_1 = require("../qos/factory");
function makeStatsReport(kind) {
    return new Map(Object.entries({
        out1: {
            id: 'out1',
            type: 'outbound-rtp',
            timestamp: 2000,
            bytesSent: kind === 'audio' ? 4000 : 10000,
            packetsSent: kind === 'audio' ? 80 : 100,
            targetBitrate: kind === 'audio' ? 32000 : 500000,
            frameWidth: kind === 'video' ? 640 : undefined,
            frameHeight: kind === 'video' ? 360 : undefined,
            framesPerSecond: kind === 'video' ? 24 : undefined,
            qualityLimitationReason: 'none',
        },
    }));
}
function makeProducer(kind, id) {
    const track = new fake_mediastreamtrack_1.FakeMediaStreamTrack({ kind });
    return {
        id,
        kind,
        track,
        closed: false,
        rtpParameters: {
            encodings: [{ maxBitrate: kind === 'audio' ? 32000 : 900000 }],
        },
        getStats: jest.fn(async () => makeStatsReport(kind)),
        setRtpEncodingParameters: jest.fn(async () => undefined),
        setMaxSpatialLayer: jest.fn(async () => undefined),
        pause: jest.fn(),
        resume: jest.fn(),
    };
}
test('createMediasoupPeerQosSession samples multiple producers and builds coordinator decision', async () => {
    const clock = new clock_1.ManualQosClock();
    const sendRequest = jest.fn(async () => undefined);
    const audioProducer = makeProducer('audio', 'audio-1');
    const videoProducer = makeProducer('video', 'video-1');
    const session = (0, factory_1.createMediasoupPeerQosSession)({
        producers: [
            { producer: audioProducer, source: 'audio' },
            { producer: videoProducer, source: 'camera' },
        ],
        sendRequest,
        clock,
    });
    await session.sampleAllNow();
    expect(audioProducer.getStats).toHaveBeenCalledTimes(1);
    expect(videoProducer.getStats).toHaveBeenCalledTimes(1);
    const decision = session.getDecision();
    expect(decision.keepAudioAlive).toBe(true);
    expect(decision.prioritizedTrackIds).toHaveLength(2);
    expect(decision.prioritizedTrackIds[0]).toBe(audioProducer.track.id);
});
test('peer qos session applies coordination override to sacrificial camera when audio quality is poor', async () => {
    const clock = new clock_1.ManualQosClock();
    const sendRequest = jest.fn(async () => undefined);
    const audioProducer = makeProducer('audio', 'audio-1');
    const videoProducer = makeProducer('video', 'video-1');
    audioProducer.getStats = jest.fn(async () => new Map(Object.entries({
        out1: {
            id: 'out1',
            type: 'outbound-rtp',
            timestamp: 2000,
            bytesSent: 1000,
            packetsSent: 100,
            packetsLost: 30,
            targetBitrate: 32000,
            qualityLimitationReason: 'bandwidth',
        },
    })));
    const session = (0, factory_1.createMediasoupPeerQosSession)({
        producers: [
            { producer: audioProducer, source: 'audio' },
            { producer: videoProducer, source: 'camera' },
        ],
        sendRequest,
        clock,
    });
    await session.sampleAllNow();
    await session.sampleAllNow();
    await session.sampleAllNow();
    await session.sampleAllNow();
    const cameraBundle = session.bundles.find(bundle => bundle.controller.getTrackState().source === 'camera');
    expect(cameraBundle).toBeDefined();
    expect(cameraBundle?.controller.getCoordinationOverride()).toEqual({
        disableRecovery: true,
        forceAudioOnly: true,
    });
});
