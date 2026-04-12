import type { Producer } from '../Producer';
import { type QosClock } from './clock';
import { PeerQosCoordinator } from './coordinator';
import { PublisherQosController } from './controller';
import { QosActionExecutor, type QosActionExecutorOptions, type QosActionSink } from './executor';
import type { QosProfile, QosSource } from './types';
import { type ProducerAdapter } from './adapters/producerAdapter';
import { QosSignalChannel, type QosSignalSender } from './adapters/signalChannel';
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
    handleNotification: (message: {
        method: string;
        data?: unknown;
    }) => void;
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
    handleNotification: (message: {
        method: string;
        data?: unknown;
    }) => void;
    getDecision: () => ReturnType<PeerQosCoordinator['getDecision']>;
};
export declare function createMediasoupProducerQosController(options: CreateMediasoupProducerQosControllerOptions): MediasoupProducerQosBundle;
export declare function createMediasoupPeerQosSession(options: CreateMediasoupPeerQosSessionOptions): MediasoupPeerQosSession;
//# sourceMappingURL=factory.d.ts.map