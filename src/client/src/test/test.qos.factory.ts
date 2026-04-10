import { FakeMediaStreamTrack } from 'fake-mediastreamtrack';

import { ManualQosClock } from '../qos/clock';
import { createMediasoupProducerQosController } from '../qos/factory';

function makeStatsReport(): RTCStatsReport {
	return new Map(
		Object.entries({
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
		})
	) as unknown as RTCStatsReport;
}

function makeProducer() {
	const track = new FakeMediaStreamTrack({ kind: 'video' });

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
	} as any;
}

test('createMediasoupProducerQosController wires a producer into a runnable qos bundle', async () => {
	const producer = makeProducer();
	const sendRequest = jest.fn(async () => undefined);
	const clock = new ManualQosClock();

	const bundle = createMediasoupProducerQosController({
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

	const firstCall = sendRequest.mock.calls[0] as unknown as [
		string,
		{ schema?: string },
	];

	expect(firstCall[0]).toBe('clientStats');
	expect(firstCall[1]?.schema).toBe('mediasoup.qos.client.v1');
});
