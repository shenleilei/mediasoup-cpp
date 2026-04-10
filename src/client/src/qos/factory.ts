import type { Producer } from '../Producer';
import { SystemQosClock, type QosClock } from './clock';
import { PeerQosCoordinator } from './coordinator';
import { PublisherQosController } from './controller';
import {
	QosActionExecutor,
	type QosActionExecutorOptions,
	type QosActionSink,
} from './executor';
import { resolveProfile } from './profiles';
import type { QosProfile, QosSource } from './types';
import {
	MediasoupProducerAdapter,
	type ProducerAdapter,
	type ProducerAdapterActionResult,
} from './adapters/producerAdapter';
import {
	QosSignalChannel,
	type QosSignalSender,
} from './adapters/signalChannel';
import { ProducerSenderStatsProvider } from './adapters/statsProvider';

export type MediasoupProducerQosActionSink = QosActionSink;

export type CreateMediasoupProducerQosControllerOptions = {
	producer: Producer;
	source: QosSource;
	sendRequest: QosSignalSender;
	profile?: Partial<QosProfile>;
	clock?: QosClock;
	traceCapacity?: number;
	sampleIntervalMs?: number;
	snapshotIntervalMs?: number;
	allowAudioOnly?: boolean;
	allowVideoPause?: boolean;
	executorOptions?: QosActionExecutorOptions;
};

export type MediasoupProducerQosBundle = {
	controller: PublisherQosController;
	signalChannel: QosSignalChannel;
	actionExecutor: QosActionExecutor;
	statsProvider: ProducerSenderStatsProvider;
	producerAdapter: ProducerAdapter;
	handleNotification: (message: { method: string; data?: unknown }) => void;
};

export type CreateMediasoupPeerQosSessionProducer = {
	producer: Producer;
	source: QosSource;
	profile?: Partial<QosProfile>;
};

export type CreateMediasoupPeerQosSessionOptions = {
	producers: CreateMediasoupPeerQosSessionProducer[];
	sendRequest: QosSignalSender;
	clock?: QosClock;
	traceCapacity?: number;
	sampleIntervalMs?: number;
	snapshotIntervalMs?: number;
	allowAudioOnly?: boolean;
	allowVideoPause?: boolean;
	executorOptions?: QosActionExecutorOptions;
};

export type MediasoupPeerQosSession = {
	bundles: MediasoupProducerQosBundle[];
	coordinator: PeerQosCoordinator;
	startAll: () => void;
	stopAll: () => void;
	sampleAllNow: () => Promise<void>;
	handleNotification: (message: { method: string; data?: unknown }) => void;
	getDecision: () => ReturnType<PeerQosCoordinator['getDecision']>;
};

function applyPeerCoordination(
	bundles: MediasoupProducerQosBundle[],
	decision: ReturnType<PeerQosCoordinator['getDecision']>
): void {
	for (const bundle of bundles) {
		const track = bundle.controller.getTrackState();
		const isSacrificial = decision.sacrificialTrackIds.includes(track.trackId);
		const override: Parameters<
			typeof bundle.controller.setCoordinationOverride
		>[0] = {};

		if (track.source !== 'audio' && !decision.allowVideoRecovery) {
			override.disableRecovery = true;
		}

		if (
			track.source === 'camera' &&
			isSacrificial &&
			(decision.peerQuality === 'poor' || decision.peerQuality === 'lost')
		) {
			override.forceAudioOnly = true;
		}

		if (
			track.source === 'camera' &&
			isSacrificial &&
			decision.preferScreenShare
		) {
			override.disableRecovery = true;
		}

		bundle.controller.setCoordinationOverride(
			Object.keys(override).length > 0 ? override : undefined
		);
	}
}

function createProducerActionSink(
	producerAdapter: ProducerAdapter
): MediasoupProducerQosActionSink {
	return {
		async execute(action) {
			let result: ProducerAdapterActionResult = { applied: true };

			switch (action.type) {
				case 'setEncodingParameters': {
					result = await producerAdapter.setEncodingParameters(
						action.encodingParameters
					);
					break;
				}

				case 'setMaxSpatialLayer': {
					result = await producerAdapter.setMaxSpatialLayer(
						action.spatialLayer
					);
					break;
				}

				case 'enterAudioOnly': {
					result = await producerAdapter.pauseUpstream();
					break;
				}

				case 'pauseUpstream': {
					result = await producerAdapter.pauseUpstream();
					break;
				}

				case 'exitAudioOnly': {
					result = await producerAdapter.resumeUpstream();
					break;
				}

				case 'resumeUpstream': {
					result = await producerAdapter.resumeUpstream();
					break;
				}

				case 'noop': {
					result = { applied: true };
					break;
				}
			}

			if (!result.applied) {
				throw new Error(result.reason ?? 'producer adapter action not applied');
			}
		},
	};
}

export function createMediasoupProducerQosController(
	options: CreateMediasoupProducerQosControllerOptions
): MediasoupProducerQosBundle {
	const producerAdapter = new MediasoupProducerAdapter(
		options.producer,
		options.source
	);
	const statsProvider = new ProducerSenderStatsProvider(producerAdapter);
	const signalChannel = new QosSignalChannel(options.sendRequest);
	const actionExecutor = new QosActionExecutor(
		createProducerActionSink(producerAdapter),
		options.executorOptions
	);
	const profile = resolveProfile(options.source, options.profile);
	const clock = options.clock ?? new SystemQosClock();

	const controller = new PublisherQosController({
		clock,
		profile,
		statsProvider,
		actionExecutor,
		signalChannel,
		trackId: options.producer.track?.id ?? options.producer.id,
		kind: options.producer.kind,
		producerId: options.producer.id,
		traceCapacity: options.traceCapacity,
		sampleIntervalMs: options.sampleIntervalMs,
		snapshotIntervalMs: options.snapshotIntervalMs,
		allowAudioOnly: options.allowAudioOnly,
		allowVideoPause: options.allowVideoPause,
	});

	return {
		controller,
		signalChannel,
		actionExecutor,
		statsProvider,
		producerAdapter,
		handleNotification(message) {
			signalChannel.handleMessage(message);
		},
	};
}

export function createMediasoupPeerQosSession(
	options: CreateMediasoupPeerQosSessionOptions
): MediasoupPeerQosSession {
	const coordinator = new PeerQosCoordinator();
	const bundles = options.producers.map(entry =>
		createMediasoupProducerQosController({
			producer: entry.producer,
			source: entry.source,
			profile: entry.profile,
			sendRequest: options.sendRequest,
			clock: options.clock,
			traceCapacity: options.traceCapacity,
			sampleIntervalMs: options.sampleIntervalMs,
			snapshotIntervalMs: options.snapshotIntervalMs,
			allowAudioOnly: options.allowAudioOnly,
			allowVideoPause: options.allowVideoPause,
			executorOptions: options.executorOptions,
		})
	);

	return {
		bundles,
		coordinator,
		startAll() {
			for (const bundle of bundles) {
				bundle.controller.start();
			}
		},
		stopAll() {
			for (const bundle of bundles) {
				bundle.controller.stop();
			}
		},
		async sampleAllNow() {
			for (const bundle of bundles) {
				await bundle.controller.sampleNow();
				coordinator.upsertTrack(bundle.controller.getTrackState());
			}

			applyPeerCoordination(bundles, coordinator.getDecision());
		},
		handleNotification(message) {
			for (const bundle of bundles) {
				bundle.handleNotification(message);
			}
		},
		getDecision() {
			for (const bundle of bundles) {
				coordinator.upsertTrack(bundle.controller.getTrackState());
			}

			const decision = coordinator.getDecision();

			applyPeerCoordination(bundles, decision);

			return decision;
		},
	};
}
