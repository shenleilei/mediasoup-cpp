import type { DerivedQosSignals, QosReason, RawSenderSnapshot } from './types';
export type ReasonClassifierContext = {
    activeOverride?: boolean;
    manual?: boolean;
    cpuSampleCount?: number;
};
export type DeriveSignalsOptions = {
    ewmaAlpha?: number;
    reasonContext?: ReasonClassifierContext;
};
/**
 * Returns non-negative counter delta.
 * Missing/invalid values, clock skew or counter reset all return 0.
 */
export declare function computeCounterDelta(currentValue: number | undefined, previousValue: number | undefined): number;
/**
 * Computes sender bitrate in bps from byte counters and timestamps.
 */
export declare function computeBitrateBps(currentBytesSent: number | undefined, previousBytesSent: number | undefined, currentTimestampMs: number, previousTimestampMs: number | undefined): number;
/**
 * Uses media-level loss ratio = lost / (sent + lost).
 */
export declare function computeLossRate(packetsSentDelta: number, packetsLostDelta: number): number;
export declare function computeEwma(currentValue: number, previousEwma: number | undefined, alpha?: number): number;
export declare function computeBitrateUtilization(sendBitrateBps: number, targetBitrateBps: number, configuredBitrateBps?: number): number;
export declare function classifyReason(signals: Pick<DerivedQosSignals, 'bandwidthLimited' | 'cpuLimited' | 'bitrateUtilization' | 'lossEwma' | 'rttEwma'>, context?: ReasonClassifierContext): QosReason;
export declare function deriveSignals(current: RawSenderSnapshot, previous?: RawSenderSnapshot, previousSignals?: DerivedQosSignals, options?: DeriveSignalsOptions): DerivedQosSignals;
//# sourceMappingURL=signals.d.ts.map
