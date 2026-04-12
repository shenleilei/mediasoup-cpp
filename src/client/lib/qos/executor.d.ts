import type { PlannedQosAction } from './types';
export type QosActionExecutionStatus = 'applied' | 'skipped_noop' | 'skipped_idempotent' | 'failed';
export type QosActionExecutionResult = {
    status: QosActionExecutionStatus;
    action: PlannedQosAction;
    actionKey: string;
    error?: Error;
};
export type QosActionKeyResolver = (action: PlannedQosAction) => string;
export interface QosActionSink {
    execute(action: PlannedQosAction): Promise<void> | void;
}
export type QosActionExecutorOptions = {
    enableIdempotence?: boolean;
    stopOnError?: boolean;
    keyResolver?: QosActionKeyResolver;
    onError?: (error: Error, action: PlannedQosAction, actionKey: string) => void;
};
export declare function defaultQosActionKeyResolver(action: PlannedQosAction): string;
/**
 * Executes planner actions against an injected sink.
 * The executor itself is generic and has no dependency on Producer/Transport.
 */
export declare class QosActionExecutor {
    private readonly _sink;
    private _lastAppliedActionKey?;
    private readonly _enableIdempotence;
    private readonly _stopOnError;
    private readonly _keyResolver;
    private readonly _onError?;
    constructor(_sink: QosActionSink, options?: QosActionExecutorOptions);
    get lastAppliedActionKey(): string | undefined;
    resetIdempotenceState(): void;
    execute(action: PlannedQosAction): Promise<QosActionExecutionResult>;
    executeMany(actions: PlannedQosAction[]): Promise<QosActionExecutionResult[]>;
}
//# sourceMappingURL=executor.d.ts.map