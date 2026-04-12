import type { DerivedQosSignals, QosProbeContext, QosProbeResult, QosProfile } from './types';
export type QosProbeEvaluation = {
    context?: QosProbeContext;
    result: QosProbeResult;
};
export declare function beginProbe(previousLevel: number, targetLevel: number, startedAtMs: number, previousAudioOnlyMode: boolean, targetAudioOnlyMode: boolean): QosProbeContext;
export declare function evaluateProbe(context: QosProbeContext | undefined, signals: DerivedQosSignals, profile: QosProfile): QosProbeEvaluation;
//# sourceMappingURL=probe.d.ts.map