import type { QosClock } from './clock';
import type { RawSenderSnapshot } from './types';
export interface QosStatsProvider {
    getSnapshot(): Promise<RawSenderSnapshot>;
}
export type QosSampleHandler = (snapshot: RawSenderSnapshot) => void | Promise<void>;
export type QosSampleErrorHandler = (error: Error) => void;
/**
 * Small clock-driven sampler used by later controller wiring.
 * This stays intentionally detached from Producer/Transport objects.
 */
export declare class IntervalQosSampler {
    private readonly _clock;
    private readonly _provider;
    private _intervalId;
    private _running;
    private _sampling;
    private _intervalMs;
    private _onSample?;
    private _onError?;
    constructor(_clock: QosClock, _provider: QosStatsProvider, intervalMs: number);
    get running(): boolean;
    get intervalMs(): number;
    updateIntervalMs(intervalMs: number): void;
    sampleOnce(): Promise<RawSenderSnapshot>;
    start(onSample: QosSampleHandler, onError?: QosSampleErrorHandler): void;
    stop(): void;
    private runTick;
}
//# sourceMappingURL=sampler.d.ts.map