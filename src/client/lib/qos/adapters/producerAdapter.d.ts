import type { Producer } from '../../Producer';
import type { QosEncodingParameters, QosSource, RawSenderSnapshot } from '../types';
export type ProducerAdapterActionResult = {
    applied: boolean;
    reason?: string;
};
export type ProducerAdapter = {
    getSnapshotBase(): Omit<RawSenderSnapshot, 'timestampMs'>;
    getStatsReport(): Promise<RTCStatsReport>;
    setEncodingParameters(parameters: QosEncodingParameters): Promise<ProducerAdapterActionResult>;
    setMaxSpatialLayer(spatialLayer: number): Promise<ProducerAdapterActionResult>;
    pauseUpstream(): Promise<ProducerAdapterActionResult>;
    resumeUpstream(): Promise<ProducerAdapterActionResult>;
};
export declare class MediasoupProducerAdapter implements ProducerAdapter {
    private readonly producer;
    private readonly source;
    constructor(producer: Producer, source: QosSource);
    getSnapshotBase(): Omit<RawSenderSnapshot, 'timestampMs'>;
    getStatsReport(): Promise<RTCStatsReport>;
    setEncodingParameters(parameters: QosEncodingParameters): Promise<ProducerAdapterActionResult>;
    setMaxSpatialLayer(spatialLayer: number): Promise<ProducerAdapterActionResult>;
    pauseUpstream(): Promise<ProducerAdapterActionResult>;
    resumeUpstream(): Promise<ProducerAdapterActionResult>;
}
//# sourceMappingURL=producerAdapter.d.ts.map