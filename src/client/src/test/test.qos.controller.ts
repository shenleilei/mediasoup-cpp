import { ManualQosClock } from '../qos/clock';
import {
	PublisherQosController,
	type PublisherQosControllerSignalChannel,
} from '../qos/controller';
import { getDefaultCameraProfile } from '../qos/profiles';
import type {
	PlannedQosAction,
	QosOverride,
	RawSenderSnapshot,
} from '../qos/types';

function createSnapshot(
	timestampMs: number,
	overrides: Partial<RawSenderSnapshot> = {}
): RawSenderSnapshot {
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

async function flushAsyncTicks(): Promise<void> {
	await Promise.resolve();
	await Promise.resolve();
}

test('controller start/stop is idempotent and drives sample loop', async () => {
	const clock = new ManualQosClock();
	const snapshots = [createSnapshot(1000), createSnapshot(2000)];
	const statsProvider = {
		getSnapshot: jest
			.fn<Promise<RawSenderSnapshot>, []>()
			.mockResolvedValueOnce(snapshots[0])
			.mockResolvedValueOnce(snapshots[1]),
	};
	const actionExecutor = {
		execute: jest
			.fn<Promise<boolean>, [PlannedQosAction]>()
			.mockResolvedValue(true),
	};
	const signalChannel: PublisherQosControllerSignalChannel = {
		publishSnapshot: jest.fn(async () => undefined),
	};

	const controller = new PublisherQosController({
		clock,
		profile: getDefaultCameraProfile(),
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
	const clock = new ManualQosClock();
	const statsProvider = {
		getSnapshot: jest
			.fn<Promise<RawSenderSnapshot>, []>()
			.mockResolvedValueOnce(
				createSnapshot(1000, {
					bytesSent: 1000,
					targetBitrateBps: 900000,
					qualityLimitationReason: 'bandwidth',
				})
			)
			.mockResolvedValueOnce(
				createSnapshot(2000, {
					bytesSent: 2000,
					targetBitrateBps: 900000,
					qualityLimitationReason: 'bandwidth',
				})
			),
	};
	const actionExecutor = {
		execute: jest
			.fn<Promise<boolean>, [PlannedQosAction]>()
			.mockResolvedValue(true),
	};
	const signalChannel: PublisherQosControllerSignalChannel = {
		publishSnapshot: jest.fn(async () => undefined),
	};

	const controller = new PublisherQosController({
		clock,
		profile: getDefaultCameraProfile(),
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

	const nonNoopCalls = actionExecutor.execute.mock.calls.filter(
		call => call[0].type !== 'noop'
	);

	expect(controller.state).toBe('early_warning');
	expect(nonNoopCalls.length).toBeGreaterThan(0);
	expect(nonNoopCalls[0]?.[0].type).toBe('setEncodingParameters');
	expect(nonNoopCalls[0]?.[0].level).toBe(1);
});

test('controller publishes snapshots on interval and immediate state transition', async () => {
	const clock = new ManualQosClock();
	const statsProvider = {
		getSnapshot: jest
			.fn<Promise<RawSenderSnapshot>, []>()
			.mockResolvedValueOnce(createSnapshot(1000, { bytesSent: 20000 }))
			.mockResolvedValueOnce(createSnapshot(2000, { bytesSent: 40000 }))
			.mockResolvedValueOnce(
				createSnapshot(3000, {
					bytesSent: 41000,
					qualityLimitationReason: 'bandwidth',
				})
			)
			.mockResolvedValueOnce(
				createSnapshot(4000, {
					bytesSent: 42000,
					qualityLimitationReason: 'bandwidth',
				})
			),
	};
	const actionExecutor = {
		execute: jest
			.fn<Promise<boolean>, [PlannedQosAction]>()
			.mockResolvedValue(true),
	};
	const signalChannel: PublisherQosControllerSignalChannel = {
		publishSnapshot: jest.fn(async () => undefined),
	};

	const controller = new PublisherQosController({
		clock,
		profile: getDefaultCameraProfile(),
		statsProvider,
		actionExecutor,
		signalChannel,
		trackId: 'track-camera-main',
		kind: 'video',
		sampleIntervalMs: 1000,
		snapshotIntervalMs: 5000,
	});

	controller.start();

	clock.advanceBy(1000);
	await flushAsyncTicks();
	clock.advanceBy(1000);
	await flushAsyncTicks();
	clock.advanceBy(1000);
	await flushAsyncTicks();
	clock.advanceBy(1000);
	await flushAsyncTicks();

	expect(signalChannel.publishSnapshot).toHaveBeenCalledTimes(2);

	const secondPayload = (signalChannel.publishSnapshot as jest.Mock).mock
		.calls[1]?.[0];

	expect(secondPayload?.tracks?.[0]?.state).toBe('early_warning');
});

test('controller applies override and forces audio-only action execution', async () => {
	const clock = new ManualQosClock();
	const statsProvider = {
		getSnapshot: jest.fn(async () => createSnapshot(1000)),
	};
	const actionExecutor = {
		execute: jest
			.fn<Promise<boolean>, [PlannedQosAction]>()
			.mockResolvedValue(true),
	};

	let onOverrideHandler: ((override: QosOverride) => void) | undefined;

	const signalChannel: PublisherQosControllerSignalChannel = {
		publishSnapshot: jest.fn(async () => undefined),
		onOverride: cb => {
			onOverrideHandler = cb;
		},
	};

	const controller = new PublisherQosController({
		clock,
		profile: getDefaultCameraProfile(),
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
	expect(
		actionExecutor.execute.mock.calls.some(
			call => call[0].type === 'enterAudioOnly'
		)
	).toBe(true);
});

test('controller sampleNow triggers a single processing cycle', async () => {
	const clock = new ManualQosClock();
	const statsProvider = {
		getSnapshot: jest.fn(async () => createSnapshot(1000)),
	};
	const actionExecutor = {
		execute: jest
			.fn<Promise<boolean>, [PlannedQosAction]>()
			.mockResolvedValue(true),
	};
	const signalChannel: PublisherQosControllerSignalChannel = {
		publishSnapshot: jest.fn(async () => undefined),
	};

	const controller = new PublisherQosController({
		clock,
		profile: getDefaultCameraProfile(),
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

test('controller applies qosPolicy updates to runtime settings', async () => {
	const clock = new ManualQosClock();
	const statsProvider = {
		getSnapshot: jest.fn(async () => createSnapshot(1000)),
	};
	const actionExecutor = {
		execute: jest
			.fn<Promise<boolean>, [PlannedQosAction]>()
			.mockResolvedValue(true),
	};

	let onPolicyHandler: ((policy: any) => void) | undefined;
	const signalChannel: PublisherQosControllerSignalChannel = {
		publishSnapshot: jest.fn(async () => undefined),
		onPolicy: cb => {
			onPolicyHandler = cb;
		},
	};

	const controller = new PublisherQosController({
		clock,
		profile: getDefaultCameraProfile(),
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
