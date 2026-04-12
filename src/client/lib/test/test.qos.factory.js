"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const fake_mediastreamtrack_1 = require("fake-mediastreamtrack");
const clock_1 = require("../qos/clock");
const factory_1 = require("../qos/factory");
function makeStatsReport() {
    return new Map(Object.entries({
        out1: {
            id: 'out1',
            type: 'outbound-rtp',
            timestamp: 2000,
            bytesSent: 10000,
            packetsSent: 100,
            targetBitrate: 500000,
            frameWidth: 640,
            frameHeight: 360,
            framesPerSecond: 24,
            qualityLimitationReason: 'none',
        },
    }));
}
function makeProducer() {
    const track = new fake_mediastreamtrack_1.FakeMediaStreamTrack({ kind: 'video' });
    return {
        id: 'producer-1',
        kind: 'video',
        track,
        closed: false,
        rtpParameters: {
            encodings: [{ maxBitrate: 900000 }],
        },
        getStats: jest.fn(async () => makeStatsReport()),
        setRtpEncodingParameters: jest.fn(async () => undefined),
        setMaxSpatialLayer: jest.fn(async () => undefined),
        pause: jest.fn(),
        resume: jest.fn(),
    };
}
test('createMediasoupProducerQosController wires a producer into a runnable qos bundle', async () => {
    const producer = makeProducer();
    const sendRequest = jest.fn(async () => undefined);
    const clock = new clock_1.ManualQosClock();
    const bundle = (0, factory_1.createMediasoupProducerQosController)({
        producer,
        source: 'camera',
        sendRequest,
        clock,
    });
    expect(bundle.controller).toBeDefined();
    expect(bundle.signalChannel).toBeDefined();
    expect(bundle.actionExecutor).toBeDefined();
    expect(bundle.statsProvider).toBeDefined();
    expect(bundle.producerAdapter).toBeDefined();
    bundle.handleNotification({
        method: 'qosPolicy',
        data: {
            schema: 'mediasoup.qos.policy.v1',
            sampleIntervalMs: 1000,
            snapshotIntervalMs: 2000,
            allowAudioOnly: true,
            allowVideoPause: true,
            profiles: {
                camera: 'default',
                screenShare: 'clarity-first',
                audio: 'speech-first',
            },
        },
    });
    await bundle.controller.sampleNow();
    expect(producer.getStats).toHaveBeenCalledTimes(1);
    expect(sendRequest).toHaveBeenCalledTimes(1);
    const firstCall = sendRequest.mock.calls[0];
    expect(firstCall[0]).toBe('clientStats');
    expect(firstCall[1]?.schema).toBe('mediasoup.qos.client.v1');
});
