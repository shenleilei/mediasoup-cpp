import { FakeMediaStreamTrack } from 'fake-mediastreamtrack';

import { ManualQosClock } from '../qos/clock';
import { createMediasoupPeerQosSession } from '../qos/factory';

function makeStatsReport(kind: 'audio' | 'video'): RTCStatsReport {
	return new Map(
		Object.entries({
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
		})
	) as unknown as RTCStatsReport;
}

function makeProducer(kind: 'audio' | 'video', id: string) {
	const track = new FakeMediaStreamTrack({ kind });

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
	} as any;
}

test('createMediasoupPeerQosSession samples multiple producers and builds coordinator decision', async () => {
	const clock = new ManualQosClock();
	const sendRequest = jest.fn(async () => undefined);
	const audioProducer = makeProducer('audio', 'audio-1');
	const videoProducer = makeProducer('video', 'video-1');

	const session = createMediasoupPeerQosSession({
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
	const clock = new ManualQosClock();
	const sendRequest = jest.fn(async () => undefined);
	const audioProducer = makeProducer('audio', 'audio-1');
	const videoProducer = makeProducer('video', 'video-1');

	audioProducer.getStats = jest.fn(
		async () =>
			new Map(
				Object.entries({
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
				})
			) as unknown as RTCStatsReport
	);

	const session = createMediasoupPeerQosSession({
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

	const cameraBundle = session.bundles.find(
		bundle => bundle.controller.getTrackState().source === 'camera'
	);

	expect(cameraBundle).toBeDefined();
	expect(cameraBundle?.controller.getCoordinationOverride()).toEqual({
		disableRecovery: true,
		forceAudioOnly: true,
	});
});
