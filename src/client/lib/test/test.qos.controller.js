"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const clock_1 = require("../qos/clock");
const controller_1 = require("../qos/controller");
const profiles_1 = require("../qos/profiles");
function createSnapshot(timestampMs, overrides = {}) {
    return {
        timestampMs,
        source: 'camera',
        kind: 'video',
        bytesSent: 1000 + timestampMs,
        packetsSent: Math.floor(timestampMs / 10),
        packetsLost: 0,
        retransmittedPacketsSent: 0,
        targetBitrateBps: 900000,
        configuredBitrateBps: 900000,
        roundTripTimeMs: 120,
        jitterMs: 8,
        frameWidth: 1280,
        frameHeight: 720,
        framesPerSecond: 30,
        qualityLimitationReason: 'none',
        ...overrides,
    };
}
async function flushAsyncTicks() {
    await Promise.resolve();
    await Promise.resolve();
}
test('controller start/stop is idempotent and drives sample loop', async () => {
    const clock = new clock_1.ManualQosClock();
    const snapshots = [createSnapshot(1000), createSnapshot(2000)];
    const statsProvider = {
        getSnapshot: jest
            .fn()
            .mockResolvedValueOnce(snapshots[0])
            .mockResolvedValueOnce(snapshots[1]),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.start();
    controller.start();
    expect(controller.isRunning).toBe(true);
    clock.advanceBy(1000);
    await flushAsyncTicks();
    clock.advanceBy(1000);
    await flushAsyncTicks();
    expect(statsProvider.getSnapshot.mock.calls.length).toBeGreaterThanOrEqual(1);
    await controller.sampleNow();
    expect(statsProvider.getSnapshot.mock.calls.length).toBeGreaterThanOrEqual(2);
    controller.stop();
    controller.stop();
    expect(controller.isRunning).toBe(false);
    clock.advanceBy(3000);
    await flushAsyncTicks();
    expect(statsProvider.getSnapshot).toHaveBeenCalledTimes(2);
});
test('controller executes planned degradation action after warning transition', async () => {
    const clock = new clock_1.ManualQosClock();
    const statsProvider = {
        getSnapshot: jest
            .fn()
            .mockResolvedValueOnce(createSnapshot(1000, {
            bytesSent: 1000,
            targetBitrateBps: 900000,
            qualityLimitationReason: 'bandwidth',
        }))
            .mockResolvedValueOnce(createSnapshot(2000, {
            bytesSent: 2000,
            targetBitrateBps: 900000,
            qualityLimitationReason: 'bandwidth',
        })),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 10000,
    });
    await controller.sampleNow();
    await controller.sampleNow();
    const nonNoopCalls = actionExecutor.execute.mock.calls.filter(call => call[0].type !== 'noop');
    expect(controller.state).toBe('early_warning');
    expect(nonNoopCalls.length).toBeGreaterThan(0);
    expect(nonNoopCalls[0]?.[0].type).toBe('setEncodingParameters');
    expect(nonNoopCalls[0]?.[0].level).toBe(1);
});
test('controller does not degrade on content-limited low send bitrate alone', async () => {
    const clock = new clock_1.ManualQosClock();
    const statsProvider = {
        getSnapshot: jest
            .fn()
            .mockResolvedValueOnce(createSnapshot(1000, {
            bytesSent: 1000,
            targetBitrateBps: 900000,
            roundTripTimeMs: 80,
            jitterMs: 6,
            qualityLimitationReason: 'none',
        }))
            .mockResolvedValueOnce(createSnapshot(2000, {
            bytesSent: 2000,
            targetBitrateBps: 900000,
            roundTripTimeMs: 80,
            jitterMs: 6,
            qualityLimitationReason: 'none',
        }))
            .mockResolvedValueOnce(createSnapshot(3000, {
            bytesSent: 3000,
            targetBitrateBps: 900000,
            roundTripTimeMs: 80,
            jitterMs: 6,
            qualityLimitationReason: 'none',
        }))
            .mockResolvedValueOnce(createSnapshot(4000, {
            bytesSent: 4000,
            targetBitrateBps: 900000,
            roundTripTimeMs: 80,
            jitterMs: 6,
            qualityLimitationReason: 'none',
        })),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    await controller.sampleNow();
    await controller.sampleNow();
    await controller.sampleNow();
    await controller.sampleNow();
    expect(controller.state).toBe('stable');
    expect(controller.level).toBe(0);
    expect(actionExecutor.execute).not.toHaveBeenCalled();
});
test('controller publishes snapshots on interval and immediate state transition', async () => {
    const clock = new clock_1.ManualQosClock();
    const statsProvider = {
        getSnapshot: jest
            .fn()
            .mockResolvedValueOnce(createSnapshot(1000, { bytesSent: 20000 }))
            .mockResolvedValueOnce(createSnapshot(2000, { bytesSent: 40000 }))
            .mockResolvedValueOnce(createSnapshot(3000, {
            bytesSent: 41000,
            qualityLimitationReason: 'bandwidth',
        }))
            .mockResolvedValueOnce(createSnapshot(4000, {
            bytesSent: 42000,
            qualityLimitationReason: 'bandwidth',
        })),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 5000,
    });
    clock.advanceBy(1000);
    await controller.sampleNow();
    clock.advanceBy(1000);
    await controller.sampleNow();
    clock.advanceBy(1000);
    await controller.sampleNow();
    clock.advanceBy(1000);
    await controller.sampleNow();
    expect(signalChannel.publishSnapshot).toHaveBeenCalledTimes(2);
    const secondPayload = signalChannel.publishSnapshot.mock
        .calls[1]?.[0];
    expect(secondPayload?.tracks?.[0]?.state).toBe('early_warning');
});
test('controller applies override and forces audio-only action execution', async () => {
    const clock = new clock_1.ManualQosClock();
    const statsProvider = {
        getSnapshot: jest.fn(async () => createSnapshot(1000)),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    let onOverrideHandler;
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
        onOverride: cb => {
            onOverrideHandler = cb;
        },
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    onOverrideHandler?.({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        ttlMs: 10000,
        reason: 'room-protection',
        forceAudioOnly: true,
    });
    controller.start();
    clock.advanceBy(1000);
    await flushAsyncTicks();
    expect(actionExecutor.execute).toHaveBeenCalled();
    expect(actionExecutor.execute.mock.calls.some(call => call[0].type === 'enterAudioOnly')).toBe(true);
});
test('controller ignores forceAudioOnly override when policy disables video pause', async () => {
    const clock = new clock_1.ManualQosClock();
    const statsProvider = {
        getSnapshot: jest.fn(async () => createSnapshot(1000)),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    let onOverrideHandler;
    let onPolicyHandler;
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
        onOverride: cb => {
            onOverrideHandler = cb;
        },
        onPolicy: cb => {
            onPolicyHandler = cb;
        },
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    onPolicyHandler?.({
        schema: 'mediasoup.qos.policy.v1',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
        allowAudioOnly: false,
        allowVideoPause: false,
        profiles: {
            camera: 'conservative',
            screenShare: 'clarity-first',
            audio: 'speech-first',
        },
    });
    onOverrideHandler?.({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        ttlMs: 10000,
        reason: 'room-protection',
        forceAudioOnly: true,
    });
    await controller.sampleNow();
    expect(actionExecutor.execute).not.toHaveBeenCalled();
    expect(actionExecutor.execute.mock.calls.some(call => call[0].type === 'enterAudioOnly')).toBe(false);
    expect(controller.getTrackState().inAudioOnlyMode).toBe(false);
});
test('controller sampleNow triggers a single processing cycle', async () => {
    const clock = new clock_1.ManualQosClock();
    const statsProvider = {
        getSnapshot: jest.fn(async () => createSnapshot(1000)),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    await controller.sampleNow();
    expect(statsProvider.getSnapshot).toHaveBeenCalledTimes(1);
    expect(signalChannel.publishSnapshot).toHaveBeenCalledTimes(1);
});
test('controller can begin recovery from audio-only after network health returns', async () => {
    const clock = new clock_1.ManualQosClock();
    const statsProvider = {
        getSnapshot: jest.fn(async () => createSnapshot(10000, {
            bytesSent: 1000,
            packetsSent: 100,
            packetsLost: 0,
            targetBitrateBps: 300000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 24,
            qualityLimitationReason: 'none',
        })),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.currentLevel = 4;
    controller.inAudioOnlyMode = true;
    controller.stateContext = {
        state: 'congested',
        enteredAtMs: 1000,
        lastCongestedAtMs: 1000,
        consecutiveHealthySamples: 4,
        consecutiveRecoverySamples: 4,
        consecutiveWarningSamples: 0,
        consecutiveCongestedSamples: 0,
    };
    clock.advanceBy(10000);
    await controller.sampleNow();
    expect(controller.state).toBe('recovering');
    expect(controller.level).toBe(3);
    expect(controller.getTrackState().inAudioOnlyMode).toBe(false);
    expect(actionExecutor.execute.mock.calls.some(call => call[0].type === 'exitAudioOnly')).toBe(true);
});
test('controller holds recovery level steady while probe is in progress', async () => {
    const clock = new clock_1.ManualQosClock();
    const healthySamples = [
        createSnapshot(1000, {
            bytesSent: 120000,
            packetsSent: 120,
            packetsLost: 0,
            targetBitrateBps: 300000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(2000, {
            bytesSent: 240000,
            packetsSent: 240,
            packetsLost: 0,
            targetBitrateBps: 300000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(3000, {
            bytesSent: 360000,
            packetsSent: 360,
            packetsLost: 0,
            targetBitrateBps: 300000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(4000, {
            bytesSent: 480000,
            packetsSent: 480,
            packetsLost: 0,
            targetBitrateBps: 300000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
    ];
    const statsProvider = {
        getSnapshot: jest
            .fn()
            .mockResolvedValueOnce(healthySamples[0])
            .mockResolvedValueOnce(healthySamples[1])
            .mockResolvedValueOnce(healthySamples[2])
            .mockResolvedValueOnce(healthySamples[3]),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.previousSnapshot = createSnapshot(0, {
        bytesSent: 0,
        packetsSent: 0,
        packetsLost: 0,
        targetBitrateBps: 300000,
        configuredBitrateBps: 900000,
        roundTripTimeMs: 90,
        jitterMs: 8,
        qualityLimitationReason: 'none',
    });
    controller.currentLevel = 4;
    controller.inAudioOnlyMode = true;
    controller.stateContext = {
        state: 'recovering',
        enteredAtMs: 0,
        lastCongestedAtMs: 0,
        lastRecoveryAtMs: 0,
        consecutiveHealthySamples: 0,
        consecutiveRecoverySamples: 0,
        consecutiveWarningSamples: 0,
        consecutiveCongestedSamples: 0,
    };
    clock.advanceBy(1000);
    await controller.sampleNow();
    expect(controller.level).toBe(3);
    expect(controller.getTrackState().inAudioOnlyMode).toBe(false);
    expect(controller.getRuntimeSettings().probeActive).toBe(true);
    expect(actionExecutor.execute.mock.calls.map(call => call[0].type)).toEqual([
        'exitAudioOnly',
        'setEncodingParameters',
        'setMaxSpatialLayer',
    ]);
    clock.advanceBy(1000);
    await controller.sampleNow();
    expect(controller.level).toBe(3);
    expect(controller.getRuntimeSettings().probeActive).toBe(true);
    expect(actionExecutor.execute).toHaveBeenCalledTimes(3);
    clock.advanceBy(1000);
    await controller.sampleNow();
    expect(controller.level).toBe(3);
    expect(controller.getRuntimeSettings().probeActive).toBe(true);
    expect(actionExecutor.execute).toHaveBeenCalledTimes(3);
    clock.advanceBy(1000);
    await controller.sampleNow();
    expect(controller.level).toBe(2);
    expect(controller.getTrackState().inAudioOnlyMode).toBe(false);
    expect(actionExecutor.execute).toHaveBeenCalledTimes(4);
    expect(actionExecutor.execute.mock.calls[3]?.[0]).toMatchObject({
        type: 'setEncodingParameters',
        level: 2,
    });
});
test('controller accelerates the next recovery probe after a strong probe success', async () => {
    const clock = new clock_1.ManualQosClock();
    const healthySamples = [
        createSnapshot(1000, {
            bytesSent: 120000,
            packetsSent: 120,
            packetsLost: 0,
            targetBitrateBps: 300000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(2000, {
            bytesSent: 240000,
            packetsSent: 240,
            packetsLost: 0,
            targetBitrateBps: 300000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(3000, {
            bytesSent: 360000,
            packetsSent: 360,
            packetsLost: 0,
            targetBitrateBps: 300000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(4000, {
            bytesSent: 480000,
            packetsSent: 480,
            packetsLost: 0,
            targetBitrateBps: 300000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(5000, {
            bytesSent: 720000,
            packetsSent: 600,
            packetsLost: 0,
            targetBitrateBps: 500000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(6000, {
            bytesSent: 960000,
            packetsSent: 720,
            packetsLost: 0,
            targetBitrateBps: 500000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
    ];
    const statsProvider = {
        getSnapshot: jest
            .fn()
            .mockResolvedValueOnce(healthySamples[0])
            .mockResolvedValueOnce(healthySamples[1])
            .mockResolvedValueOnce(healthySamples[2])
            .mockResolvedValueOnce(healthySamples[3])
            .mockResolvedValueOnce(healthySamples[4])
            .mockResolvedValueOnce(healthySamples[5]),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.previousSnapshot = createSnapshot(0, {
        bytesSent: 0,
        packetsSent: 0,
        packetsLost: 0,
        targetBitrateBps: 300000,
        configuredBitrateBps: 900000,
        roundTripTimeMs: 90,
        jitterMs: 8,
        qualityLimitationReason: 'none',
    });
    controller.currentLevel = 4;
    controller.inAudioOnlyMode = true;
    controller.stateContext = {
        state: 'recovering',
        enteredAtMs: 0,
        lastCongestedAtMs: 0,
        lastRecoveryAtMs: 0,
        consecutiveHealthySamples: 0,
        consecutiveRecoverySamples: 0,
        consecutiveWarningSamples: 0,
        consecutiveCongestedSamples: 0,
    };
    for (let i = 0; i < healthySamples.length; i += 1) {
        clock.advanceBy(1000);
        await controller.sampleNow();
    }
    expect(controller.level).toBe(1);
    expect(actionExecutor.execute.mock.calls.some(call => call[0].type === 'exitAudioOnly')).toBe(true);
    expect(actionExecutor.execute.mock.calls.filter(call => call[0].type === 'setEncodingParameters').map(call => call[0].level)).toEqual([3, 2, 1]);
});
test('controller probes stable upgrades before applying the next recovery step', async () => {
    const clock = new clock_1.ManualQosClock();
    const healthySamples = [
        createSnapshot(1000, {
            bytesSent: 120000,
            packetsSent: 120,
            packetsLost: 0,
            targetBitrateBps: 900000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(2000, {
            bytesSent: 240000,
            packetsSent: 240,
            packetsLost: 0,
            targetBitrateBps: 900000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(3000, {
            bytesSent: 360000,
            packetsSent: 360,
            packetsLost: 0,
            targetBitrateBps: 900000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(4000, {
            bytesSent: 480000,
            packetsSent: 480,
            packetsLost: 0,
            targetBitrateBps: 900000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
    ];
    const statsProvider = {
        getSnapshot: jest
            .fn()
            .mockResolvedValueOnce(healthySamples[0])
            .mockResolvedValueOnce(healthySamples[1])
            .mockResolvedValueOnce(healthySamples[2])
            .mockResolvedValueOnce(healthySamples[3]),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.previousSnapshot = createSnapshot(0, {
        bytesSent: 0,
        packetsSent: 0,
        packetsLost: 0,
        targetBitrateBps: 900000,
        configuredBitrateBps: 900000,
        roundTripTimeMs: 90,
        jitterMs: 8,
        qualityLimitationReason: 'none',
    });
    controller.currentLevel = 2;
    controller.stateContext = {
        state: 'stable',
        enteredAtMs: 0,
        consecutiveHealthySamples: 0,
        consecutiveRecoverySamples: 0,
        consecutiveWarningSamples: 0,
        consecutiveCongestedSamples: 0,
    };
    clock.advanceBy(1000);
    await controller.sampleNow();
    expect(controller.level).toBe(1);
    expect(controller.getRuntimeSettings().probeActive).toBe(true);
    expect(actionExecutor.execute.mock.calls[0]?.[0]).toMatchObject({
        type: 'setEncodingParameters',
        level: 1,
    });
    clock.advanceBy(1000);
    await controller.sampleNow();
    expect(controller.level).toBe(1);
    expect(controller.getRuntimeSettings().probeActive).toBe(true);
    expect(actionExecutor.execute).toHaveBeenCalledTimes(1);
    clock.advanceBy(1000);
    await controller.sampleNow();
    expect(controller.level).toBe(1);
    expect(controller.getRuntimeSettings().probeActive).toBe(true);
    expect(actionExecutor.execute).toHaveBeenCalledTimes(1);
    clock.advanceBy(1000);
    await controller.sampleNow();
    expect(controller.level).toBe(0);
    expect(controller.getRuntimeSettings().probeActive).toBe(true);
    expect(actionExecutor.execute).toHaveBeenCalledTimes(2);
    expect(actionExecutor.execute.mock.calls[1]?.[0]).toMatchObject({
        type: 'setEncodingParameters',
        level: 0,
    });
});
test('controller ignores jitter-only probe spikes until the upgrade probe completes', async () => {
    const clock = new clock_1.ManualQosClock();
    const samples = [
        createSnapshot(1000, {
            bytesSent: 120000,
            packetsSent: 120,
            packetsLost: 0,
            targetBitrateBps: 900000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(2000, {
            bytesSent: 240000,
            packetsSent: 240,
            packetsLost: 0,
            targetBitrateBps: 900000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 160,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(3000, {
            bytesSent: 360000,
            packetsSent: 360,
            packetsLost: 0,
            targetBitrateBps: 900000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 140,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(4000, {
            bytesSent: 480000,
            packetsSent: 480,
            packetsLost: 0,
            targetBitrateBps: 900000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
    ];
    const statsProvider = {
        getSnapshot: jest
            .fn()
            .mockResolvedValueOnce(samples[0])
            .mockResolvedValueOnce(samples[1])
            .mockResolvedValueOnce(samples[2])
            .mockResolvedValueOnce(samples[3]),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.previousSnapshot = createSnapshot(0, {
        bytesSent: 0,
        packetsSent: 0,
        packetsLost: 0,
        targetBitrateBps: 900000,
        configuredBitrateBps: 900000,
        roundTripTimeMs: 90,
        jitterMs: 8,
        qualityLimitationReason: 'none',
    });
    controller.currentLevel = 2;
    controller.stateContext = {
        state: 'stable',
        enteredAtMs: 0,
        consecutiveHealthySamples: 0,
        consecutiveRecoverySamples: 0,
        consecutiveWarningSamples: 0,
        consecutiveCongestedSamples: 0,
    };
    clock.advanceBy(1000);
    await controller.sampleNow();
    expect(controller.level).toBe(1);
    expect(controller.state).toBe('stable');
    expect(controller.getRuntimeSettings().probeActive).toBe(true);
    clock.advanceBy(1000);
    await controller.sampleNow();
    expect(controller.level).toBe(1);
    expect(controller.state).toBe('stable');
    expect(controller.getRuntimeSettings().probeActive).toBe(true);
    clock.advanceBy(1000);
    await controller.sampleNow();
    expect(controller.level).toBe(1);
    expect(controller.state).toBe('stable');
    expect(controller.getRuntimeSettings().probeActive).toBe(true);
    clock.advanceBy(1000);
    await controller.sampleNow();
    expect(controller.level).toBe(0);
    expect(controller.state).toBe('stable');
    expect(controller.getRuntimeSettings().probeActive).toBe(true);
    expect(actionExecutor.execute.mock.calls.map(call => call[0].level)).toEqual([1, 0]);
});
test('controller ignores jitter-only tail spikes during recovery probe grace window', async () => {
    const clock = new clock_1.ManualQosClock();
    const samples = [
        createSnapshot(1000, {
            bytesSent: 120000,
            packetsSent: 120,
            packetsLost: 0,
            targetBitrateBps: 900000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 160,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(2000, {
            bytesSent: 240000,
            packetsSent: 240,
            packetsLost: 0,
            targetBitrateBps: 900000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 140,
            qualityLimitationReason: 'none',
        }),
        createSnapshot(3000, {
            bytesSent: 360000,
            packetsSent: 360,
            packetsLost: 0,
            targetBitrateBps: 900000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 90,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        }),
    ];
    const statsProvider = {
        getSnapshot: jest
            .fn()
            .mockResolvedValueOnce(samples[0])
            .mockResolvedValueOnce(samples[1])
            .mockResolvedValueOnce(samples[2]),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.previousSnapshot = createSnapshot(0, {
        bytesSent: 0,
        packetsSent: 0,
        packetsLost: 0,
        targetBitrateBps: 900000,
        configuredBitrateBps: 900000,
        roundTripTimeMs: 90,
        jitterMs: 8,
        qualityLimitationReason: 'none',
    });
    controller.currentLevel = 0;
    controller.stateContext = {
        state: 'stable',
        enteredAtMs: 0,
        consecutiveHealthySamples: 0,
        consecutiveRecoverySamples: 0,
        consecutiveWarningSamples: 0,
        consecutiveCongestedSamples: 0,
    };
    controller.recoveryProbeSuccessStreak = 1;
    controller.recoveryProbeGraceUntilMs = 3000;
    for (let i = 0; i < samples.length; i += 1) {
        clock.advanceBy(1000);
        await controller.sampleNow();
    }
    expect(controller.level).toBe(0);
    expect(controller.state).toBe('stable');
    expect(controller.getRuntimeSettings().probeActive).toBe(false);
    expect(actionExecutor.execute).not.toHaveBeenCalled();
});
test('controller applies qosPolicy updates to runtime settings', async () => {
    const clock = new clock_1.ManualQosClock();
    const statsProvider = {
        getSnapshot: jest.fn(async () => createSnapshot(1000)),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    let onPolicyHandler;
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
        onPolicy: cb => {
            onPolicyHandler = cb;
        },
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    onPolicyHandler?.({
        schema: 'mediasoup.qos.policy.v1',
        sampleIntervalMs: 1500,
        snapshotIntervalMs: 4000,
        allowAudioOnly: false,
        allowVideoPause: false,
        profiles: {
            camera: 'conservative',
            screenShare: 'clarity-first',
            audio: 'speech-first',
        },
    });
    expect(controller.getRuntimeSettings()).toEqual({
        sampleIntervalMs: 1500,
        snapshotIntervalMs: 4000,
        allowAudioOnly: false,
        allowVideoPause: false,
        probeActive: false,
    });
});
test('override clear removes matching overrides by reason prefix', async () => {
    const clock = new clock_1.ManualQosClock();
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider: { getSnapshot: jest.fn(async () => createSnapshot(1000)) },
        actionExecutor: { execute: jest.fn().mockResolvedValue(true) },
        signalChannel: { publishSnapshot: jest.fn(), onPolicy: () => {} },
        trackId: 'track-1',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    // Apply two server overrides with different reasons.
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        reason: 'server_auto_poor',
        ttlMs: 10000,
        maxLevelClamp: 2,
    });
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        reason: 'server_room_pressure',
        ttlMs: 8000,
        maxLevelClamp: 1,
    });
    // Both should be active — merged clamp should be min(2,1) = 1.
    let active = controller.getActiveOverride(clock.nowMs());
    expect(active).toBeDefined();
    expect(active.maxLevelClamp).toBe(1);
    // Send server_auto_clear — should remove server_auto_poor but keep room_pressure.
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        reason: 'server_auto_clear',
        ttlMs: 0,
    });
    active = controller.getActiveOverride(clock.nowMs());
    expect(active).toBeDefined();
    expect(active.maxLevelClamp).toBe(1);
    expect(active.reason).toBe('server_room_pressure');
    // Send server_room_pressure_clear — should remove the last override.
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        reason: 'server_room_pressure_clear',
        ttlMs: 0,
    });
    active = controller.getActiveOverride(clock.nowMs());
    expect(active).toBeUndefined();
});
test('server_ttl_expired clears all active overrides', async () => {
    const clock = new clock_1.ManualQosClock();
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider: { getSnapshot: jest.fn(async () => createSnapshot(1000)) },
        actionExecutor: { execute: jest.fn().mockResolvedValue(true) },
        signalChannel: { publishSnapshot: jest.fn(), onPolicy: () => {} },
        trackId: 'track-1',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        reason: 'server_auto_lost',
        ttlMs: 15000,
        forceAudioOnly: true,
    });
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        reason: 'server_room_pressure',
        ttlMs: 8000,
        maxLevelClamp: 1,
    });
    expect(controller.getActiveOverride(clock.nowMs())).toBeDefined();
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        reason: 'server_ttl_expired',
        ttlMs: 0,
    });
    expect(controller.getActiveOverride(clock.nowMs())).toBeUndefined();
});
test('manual clear removes manual overrides without clearing server overrides', async () => {
    const clock = new clock_1.ManualQosClock();
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider: { getSnapshot: jest.fn(async () => createSnapshot(1000)) },
        actionExecutor: { execute: jest.fn().mockResolvedValue(true) },
        signalChannel: { publishSnapshot: jest.fn(), onPolicy: () => {} },
        trackId: 'track-1',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        reason: 'manual_protection',
        ttlMs: 10000,
        forceAudioOnly: true,
    });
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        reason: 'server_room_pressure',
        ttlMs: 8000,
        maxLevelClamp: 1,
    });
    let active = controller.getActiveOverride(clock.nowMs());
    expect(active).toBeDefined();
    expect(active.forceAudioOnly).toBe(true);
    expect(active.maxLevelClamp).toBe(1);
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        reason: 'manual_clear',
        ttlMs: 0,
    });
    active = controller.getActiveOverride(clock.nowMs());
    expect(active).toBeDefined();
    expect(active.forceAudioOnly).not.toBe(true);
    expect(active.maxLevelClamp).toBe(1);
    expect(active.reason).toBe('server_room_pressure');
});
test('manual clear exits override-driven audio-only and restores local control', async () => {
    const clock = new clock_1.ManualQosClock();
    const statsProvider = {
        getSnapshot: jest.fn(async () => createSnapshot(1000, {
            bytesSent: 1000,
            packetsSent: 100,
            packetsLost: 0,
            targetBitrateBps: 900000,
            configuredBitrateBps: 900000,
            roundTripTimeMs: 120,
            jitterMs: 8,
            qualityLimitationReason: 'none',
        })),
    };
    const actionExecutor = {
        execute: jest
            .fn()
            .mockResolvedValue(true),
    };
    const signalChannel = {
        publishSnapshot: jest.fn(async () => undefined),
    };
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider,
        actionExecutor,
        signalChannel,
        trackId: 'track-camera-main',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        reason: 'manual_protection',
        ttlMs: 10000,
        forceAudioOnly: true,
    });
    await controller.sampleNow();
    expect(actionExecutor.execute.mock.calls.some(call => call[0].type === 'enterAudioOnly')).toBe(true);
    expect(controller.getTrackState().inAudioOnlyMode).toBe(true);
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        reason: 'manual_clear',
        ttlMs: 0,
    });
    await controller.sampleNow();
    expect(actionExecutor.execute.mock.calls.some(call => call[0].type === 'exitAudioOnly')).toBe(true);
    expect(controller.getTrackState().inAudioOnlyMode).toBe(false);
});
test('track-scoped override ignores other track id', () => {
    const clock = new clock_1.ManualQosClock();
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider: { getSnapshot: jest.fn(async () => createSnapshot(1000)) },
        actionExecutor: { execute: jest.fn().mockResolvedValue(true) },
        signalChannel: { publishSnapshot: jest.fn(), onPolicy: () => {} },
        trackId: 'track-1',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'track',
        trackId: 'track-2',
        reason: 'downlink_v2_room_demand',
        ttlMs: 5000,
        maxLevelClamp: 1,
    });
    expect(controller.getActiveOverride(clock.nowMs())).toBeUndefined();
});
test('track-scoped override applies on matching track id', () => {
    const clock = new clock_1.ManualQosClock();
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider: { getSnapshot: jest.fn(async () => createSnapshot(1000)) },
        actionExecutor: { execute: jest.fn().mockResolvedValue(true) },
        signalChannel: { publishSnapshot: jest.fn(), onPolicy: () => {} },
        trackId: 'track-1',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'track',
        trackId: 'track-1',
        reason: 'downlink_v2_room_demand',
        ttlMs: 5000,
        maxLevelClamp: 1,
    });
    const active = controller.getActiveOverride(clock.nowMs());
    expect(active).toBeDefined();
    expect(active.maxLevelClamp).toBe(1);
    expect(active.reason).toBe('downlink_v2_room_demand');
});
test('track-scoped clear clears matching track clamp', () => {
    const clock = new clock_1.ManualQosClock();
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider: { getSnapshot: jest.fn(async () => createSnapshot(1000)) },
        actionExecutor: { execute: jest.fn().mockResolvedValue(true) },
        signalChannel: { publishSnapshot: jest.fn(), onPolicy: () => {} },
        trackId: 'track-1',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'track',
        trackId: 'track-1',
        reason: 'downlink_v2_room_demand',
        ttlMs: 5000,
        maxLevelClamp: 1,
    });
    expect(controller.getActiveOverride(clock.nowMs())).toBeDefined();
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'track',
        trackId: 'track-1',
        reason: 'downlink_v2_demand_restored',
        ttlMs: 0,
    });
    expect(controller.getActiveOverride(clock.nowMs())).toBeUndefined();
});
test('track-scoped clear preserves unrelated manual override', () => {
    const clock = new clock_1.ManualQosClock();
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider: { getSnapshot: jest.fn(async () => createSnapshot(1000)) },
        actionExecutor: { execute: jest.fn().mockResolvedValue(true) },
        signalChannel: { publishSnapshot: jest.fn(), onPolicy: () => {} },
        trackId: 'track-1',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'peer',
        reason: 'manual_protection',
        ttlMs: 10000,
        forceAudioOnly: true,
    });
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'track',
        trackId: 'track-1',
        reason: 'downlink_v2_room_demand',
        ttlMs: 5000,
        maxLevelClamp: 1,
    });
    let active = controller.getActiveOverride(clock.nowMs());
    expect(active).toBeDefined();
    expect(active.forceAudioOnly).toBe(true);
    expect(active.maxLevelClamp).toBe(1);
    controller.handleOverride({
        schema: 'mediasoup.qos.override.v1',
        scope: 'track',
        trackId: 'track-1',
        reason: 'downlink_v2_demand_restored',
        ttlMs: 0,
    });
    active = controller.getActiveOverride(clock.nowMs());
    expect(active).toBeDefined();
    expect(active.forceAudioOnly).toBe(true);
    expect(active.maxLevelClamp).toBeUndefined();
    expect(active.reason).toBe('manual_protection');
});
test('handlePolicy with profiles switches actual profile parameters', () => {
    const clock = new clock_1.ManualQosClock();
    let onPolicyHandler;
    const controller = new controller_1.PublisherQosController({
        clock,
        profile: (0, profiles_1.getDefaultCameraProfile)(),
        statsProvider: { getSnapshot: jest.fn(async () => createSnapshot(1000)) },
        actionExecutor: { execute: jest.fn().mockResolvedValue(true) },
        signalChannel: {
            publishSnapshot: jest.fn(),
            onPolicy: cb => { onPolicyHandler = cb; },
        },
        trackId: 'track-1',
        kind: 'video',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 1000,
    });
    const defaultProfile = (0, profiles_1.getDefaultCameraProfile)();
    // Verify starting state is default.
    expect(controller.profile.name).toBe('default-camera');
    expect(controller.profile.recoveryCooldownMs).toBe(defaultProfile.recoveryCooldownMs);
    // Switch to conservative via policy.
    onPolicyHandler?.({
        schema: 'mediasoup.qos.policy.v1',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 2000,
        allowAudioOnly: true,
        allowVideoPause: true,
        profiles: {
            camera: 'conservative',
            screenShare: 'clarity-first',
            audio: 'speech-first',
        },
    });
    // Profile should have actually changed — not just the name.
    expect(controller.profile.name).toBe('conservative');
    expect(controller.profile.recoveryCooldownMs).toBe(12000);
    expect(controller.profile.thresholds.congestedLossRate).toBe(0.06);
    // Sending same profile name again should not re-resolve.
    const profileRef = controller.profile;
    onPolicyHandler?.({
        schema: 'mediasoup.qos.policy.v1',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 2000,
        allowAudioOnly: true,
        allowVideoPause: true,
        profiles: { camera: 'conservative', screenShare: 'clarity-first', audio: 'speech-first' },
    });
    expect(controller.profile).toBe(profileRef);
});
