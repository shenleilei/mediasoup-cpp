import type { DerivedQosSignals, QosProfile, QosQuality, QosState, QosStateMachineContext, QosStateTransitionResult } from './types';
export declare function createInitialQosStateMachineContext(nowMs: number): QosStateMachineContext;
export declare function mapStateToQuality(state: QosState, signals: DerivedQosSignals): QosQuality;
export declare function evaluateStateTransition(context: QosStateMachineContext, signals: DerivedQosSignals, profile: QosProfile, nowMs: number): QosStateTransitionResult;
//# sourceMappingURL=stateMachine.d.ts.map