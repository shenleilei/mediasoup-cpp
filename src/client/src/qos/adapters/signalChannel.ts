import {
	parseQosOverride,
	parseQosPolicy,
	serializeClientQosSnapshot,
} from '../protocol';
import type {
	ClientQosSnapshot,
	QosConnectionQualityUpdate,
	QosOverride,
	QosPolicy,
	QosRoomState,
} from '../types';

export type QosSignalSender = (
	method: string,
	data: unknown
) => Promise<void> | void;

export type QosPolicyListener = (policy: QosPolicy) => void;
export type QosOverrideListener = (override: QosOverride) => void;
export type QosConnectionQualityListener = (
	update: QosConnectionQualityUpdate
) => void;
export type QosRoomStateListener = (state: QosRoomState) => void;

export type QosSignalMessage = {
	method: string;
	data?: unknown;
};

/**
 * Lightweight bridge between QoS module and app signaling layer.
 * It does not own transport; caller wires send/receive callbacks.
 */
export class QosSignalChannel {
	private readonly policyListeners: Set<QosPolicyListener> = new Set();
	private readonly overrideListeners: Set<QosOverrideListener> = new Set();
	private readonly connectionQualityListeners: Set<QosConnectionQualityListener> =
		new Set();
	private readonly roomStateListeners: Set<QosRoomStateListener> = new Set();

	constructor(
		private readonly send: QosSignalSender,
		private readonly publishMethod = 'clientStats'
	) {}

	async publishSnapshot(snapshot: ClientQosSnapshot): Promise<void> {
		const data = serializeClientQosSnapshot(snapshot);

		await this.send(this.publishMethod, data);
	}

	onPolicy(listener: QosPolicyListener): () => void {
		this.policyListeners.add(listener);

		return () => {
			this.policyListeners.delete(listener);
		};
	}

	onOverride(listener: QosOverrideListener): () => void {
		this.overrideListeners.add(listener);

		return () => {
			this.overrideListeners.delete(listener);
		};
	}

	onConnectionQuality(listener: QosConnectionQualityListener): () => void {
		this.connectionQualityListeners.add(listener);

		return () => {
			this.connectionQualityListeners.delete(listener);
		};
	}

	onRoomState(listener: QosRoomStateListener): () => void {
		this.roomStateListeners.add(listener);

		return () => {
			this.roomStateListeners.delete(listener);
		};
	}

	/**
	 * Ingests app-level signaling messages and dispatches qos policy/override.
	 * Invalid payloads are ignored intentionally (fail-soft).
	 */
	handleMessage(message: QosSignalMessage): void {
		if (!message || typeof message.method !== 'string') {
			return;
		}

		if (message.method === 'qosPolicy') {
			try {
				const policy = parseQosPolicy(message.data);

				for (const listener of this.policyListeners) {
					listener(policy);
				}
			} catch {
				// Fail-soft: drop malformed policy payload.
			}

			return;
		}

		if (message.method === 'qosOverride') {
			try {
				const override = parseQosOverride(message.data);

				for (const listener of this.overrideListeners) {
					listener(override);
				}
			} catch {
				// Fail-soft: drop malformed override payload.
			}
		}

		if (message.method === 'qosConnectionQuality') {
			const data = message.data as QosConnectionQualityUpdate | undefined;

			if (!data || typeof data.quality !== 'string') {
				return;
			}
			for (const listener of this.connectionQualityListeners) {
				listener(data);
			}

			return;
		}

		if (message.method === 'qosRoomState') {
			const data = message.data as QosRoomState | undefined;

			if (!data || typeof data.quality !== 'string') {
				return;
			}
			for (const listener of this.roomStateListeners) {
				listener(data);
			}
		}
	}
}
