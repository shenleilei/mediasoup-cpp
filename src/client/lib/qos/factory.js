"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.createMediasoupProducerQosController = createMediasoupProducerQosController;
exports.createMediasoupPeerQosSession = createMediasoupPeerQosSession;
const clock_1 = require("./clock");
const coordinator_1 = require("./coordinator");
const controller_1 = require("./controller");
const executor_1 = require("./executor");
const profiles_1 = require("./profiles");
const producerAdapter_1 = require("./adapters/producerAdapter");
const signalChannel_1 = require("./adapters/signalChannel");
const statsProvider_1 = require("./adapters/statsProvider");
function applyPeerCoordination(bundles, decision) {
    for (const bundle of bundles) {
        const track = bundle.controller.getTrackState();
        const isSacrificial = decision.sacrificialTrackIds.includes(track.trackId);
        const override = {};
        if (track.source !== 'audio' && !decision.allowVideoRecovery) {
            override.disableRecovery = true;
        }
        if (track.source === 'camera' &&
            isSacrificial &&
            (decision.peerQuality === 'poor' || decision.peerQuality === 'lost')) {
            override.forceAudioOnly = true;
        }
        if (track.source === 'camera' &&
            isSacrificial &&
            decision.preferScreenShare) {
            override.disableRecovery = true;
        }
        bundle.controller.setCoordinationOverride(Object.keys(override).length > 0 ? override : undefined);
    }
}
function createProducerActionSink(producerAdapter) {
    return {
        async execute(action) {
            let result = { applied: true };
            switch (action.type) {
                case 'setEncodingParameters': {
                    result = await producerAdapter.setEncodingParameters(action.encodingParameters);
                    break;
                }
                case 'setMaxSpatialLayer': {
                    result = await producerAdapter.setMaxSpatialLayer(action.spatialLayer);
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
function createMediasoupProducerQosController(options) {
    const producerAdapter = new producerAdapter_1.MediasoupProducerAdapter(options.producer, options.source);
    const statsProvider = new statsProvider_1.ProducerSenderStatsProvider(producerAdapter);
    const signalChannel = new signalChannel_1.QosSignalChannel(options.sendRequest);
    const actionExecutor = new executor_1.QosActionExecutor(createProducerActionSink(producerAdapter), options.executorOptions);
    const profile = (0, profiles_1.resolveProfile)(options.source, options.profile);
    const clock = options.clock ?? new clock_1.SystemQosClock();
    const controller = new controller_1.PublisherQosController({
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
function createMediasoupPeerQosSession(options) {
    const coordinator = new coordinator_1.PeerQosCoordinator();
    const bundles = options.producers.map(entry => createMediasoupProducerQosController({
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
    }));
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
