import type { QosClock } from './clock';
import type { RawSenderSnapshot } from './types';

export interface QosStatsProvider {
	getSnapshot(): Promise<RawSenderSnapshot>;
}

export type QosSampleHandler = (
	snapshot: RawSenderSnapshot
) => void | Promise<void>;

export type QosSampleErrorHandler = (error: Error) => void;

function toError(error: unknown): Error {
	if (error instanceof Error) {
		return error;
	}

	return new Error(String(error));
}

/**
 * Small clock-driven sampler used by later controller wiring.
 * This stays intentionally detached from Producer/Transport objects.
 */
export class IntervalQosSampler {
	private _intervalId: unknown;
	private _running = false;
	private _sampling = false;
	private _intervalMs: number;
	private _onSample?: QosSampleHandler;
	private _onError?: QosSampleErrorHandler;

	constructor(
		private readonly _clock: QosClock,
		private readonly _provider: QosStatsProvider,
		intervalMs: number
	) {
		if (!Number.isFinite(intervalMs) || intervalMs <= 0) {
			throw new TypeError('intervalMs must be a positive finite number');
		}

		this._intervalMs = Math.floor(intervalMs);
	}

	get running(): boolean {
		return this._running;
	}

	get intervalMs(): number {
		return this._intervalMs;
	}

	updateIntervalMs(intervalMs: number): void {
		if (!Number.isFinite(intervalMs) || intervalMs <= 0) {
			throw new TypeError('intervalMs must be a positive finite number');
		}

		const nextIntervalMs = Math.floor(intervalMs);

		if (nextIntervalMs === this._intervalMs) {
			return;
		}

		this._intervalMs = nextIntervalMs;

		if (this._running) {
			this.stop();

			if (this._onSample) {
				this.start(this._onSample, this._onError);
			}
		}
	}

	async sampleOnce(): Promise<RawSenderSnapshot> {
		return this._provider.getSnapshot();
	}

	start(onSample: QosSampleHandler, onError?: QosSampleErrorHandler): void {
		if (this._running) {
			return;
		}

		this._onSample = onSample;
		this._onError = onError;
		this._running = true;
		this._intervalId = this._clock.setInterval(() => {
			void this.runTick(onSample, onError);
		}, this._intervalMs);
	}

	stop(): void {
		if (!this._running) {
			return;
		}

		this._running = false;

		if (this._intervalId !== undefined) {
			this._clock.clearInterval(this._intervalId);
			this._intervalId = undefined;
		}
	}

	private async runTick(
		onSample: QosSampleHandler,
		onError?: QosSampleErrorHandler
	): Promise<void> {
		if (this._sampling) {
			return;
		}

		this._sampling = true;

		try {
			const snapshot = await this._provider.getSnapshot();

			await onSample(snapshot);
		} catch (error) {
			onError?.(toError(error));
		} finally {
			this._sampling = false;
		}
	}
}
