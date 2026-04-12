import type { QosTraceEntry } from './types';
/**
 * Fixed-capacity trace buffer for QoS diagnostics.
 */
export declare class QosTraceBuffer<TEntry = QosTraceEntry> {
    private readonly _capacity;
    private readonly _entries;
    constructor(capacity?: number);
    get capacity(): number;
    get size(): number;
    append(entry: TEntry): void;
    getEntries(): TEntry[];
    clear(): void;
    appendTraceEntry(entry: TEntry): void;
    getTraceEntries(): TEntry[];
    clearTrace(): void;
}
export declare function createQosTraceBuffer<TEntry = QosTraceEntry>(capacity?: number): QosTraceBuffer<TEntry>;
//# sourceMappingURL=trace.d.ts.map