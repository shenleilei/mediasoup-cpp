import type { ClientQosSnapshot, QosConnectionQualityUpdate, QosOverride, QosPolicy, QosRoomState } from '../types';
export type QosSignalSender = (method: string, data: unknown) => Promise<void> | void;
export type QosPolicyListener = (policy: QosPolicy) => void;
export type QosOverrideListener = (override: QosOverride) => void;
export type QosConnectionQualityListener = (update: QosConnectionQualityUpdate) => void;
export type QosRoomStateListener = (state: QosRoomState) => void;
export type QosSignalMessage = {
    method: string;
    data?: unknown;
};
/**
 * Lightweight bridge between QoS module and app signaling layer.
 * It does not own transport; caller wires send/receive callbacks.
 */
export declare class QosSignalChannel {
    private readonly send;
    private readonly publishMethod;
    private readonly policyListeners;
    private readonly overrideListeners;
    private readonly connectionQualityListeners;
    private readonly roomStateListeners;
    constructor(send: QosSignalSender, publishMethod?: string);
    publishSnapshot(snapshot: ClientQosSnapshot): Promise<void>;
    onPolicy(listener: QosPolicyListener): () => void;
    onOverride(listener: QosOverrideListener): () => void;
    onConnectionQuality(listener: QosConnectionQualityListener): () => void;
    onRoomState(listener: QosRoomStateListener): () => void;
    /**
     * Ingests app-level signaling messages and dispatches qos policy/override.
     * Invalid payloads are ignored intentionally (fail-soft).
     */
    handleMessage(message: QosSignalMessage): void;
}
//# sourceMappingURL=signalChannel.d.ts.map