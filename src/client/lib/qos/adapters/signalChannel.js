"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.QosSignalChannel = void 0;
const protocol_1 = require("../protocol");
/**
 * Lightweight bridge between QoS module and app signaling layer.
 * It does not own transport; caller wires send/receive callbacks.
 */
class QosSignalChannel {
    constructor(send, publishMethod = 'clientStats') {
        this.send = send;
        this.publishMethod = publishMethod;
        this.policyListeners = new Set();
        this.overrideListeners = new Set();
        this.connectionQualityListeners = new Set();
        this.roomStateListeners = new Set();
    }
    async publishSnapshot(snapshot) {
        const data = (0, protocol_1.serializeClientQosSnapshot)(snapshot);
        await this.send(this.publishMethod, data);
    }
    onPolicy(listener) {
        this.policyListeners.add(listener);
        return () => {
            this.policyListeners.delete(listener);
        };
    }
    onOverride(listener) {
        this.overrideListeners.add(listener);
        return () => {
            this.overrideListeners.delete(listener);
        };
    }
    onConnectionQuality(listener) {
        this.connectionQualityListeners.add(listener);
        return () => {
            this.connectionQualityListeners.delete(listener);
        };
    }
    onRoomState(listener) {
        this.roomStateListeners.add(listener);
        return () => {
            this.roomStateListeners.delete(listener);
        };
    }
    /**
     * Ingests app-level signaling messages and dispatches qos policy/override.
     * Invalid payloads are ignored intentionally (fail-soft).
     */
    handleMessage(message) {
        if (!message || typeof message.method !== 'string') {
            return;
        }
        if (message.method === 'qosPolicy') {
            try {
                const policy = (0, protocol_1.parseQosPolicy)(message.data);
                for (const listener of this.policyListeners) {
                    listener(policy);
                }
            }
            catch {
                // Fail-soft: drop malformed policy payload.
            }
            return;
        }
        if (message.method === 'qosOverride') {
            try {
                const override = (0, protocol_1.parseQosOverride)(message.data);
                for (const listener of this.overrideListeners) {
                    listener(override);
                }
            }
            catch {
                // Fail-soft: drop malformed override payload.
            }
        }
        if (message.method === 'qosConnectionQuality') {
            const data = message.data;
            if (!data || typeof data.quality !== 'string') {
                return;
            }
            for (const listener of this.connectionQualityListeners) {
                listener(data);
            }
            return;
        }
        if (message.method === 'qosRoomState') {
            const data = message.data;
            if (!data || typeof data.quality !== 'string') {
                return;
            }
            for (const listener of this.roomStateListeners) {
                listener(data);
            }
        }
    }
}
exports.QosSignalChannel = QosSignalChannel;
