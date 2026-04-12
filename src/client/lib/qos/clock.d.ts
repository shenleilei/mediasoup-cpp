export interface QosClock {
    nowMs(): number;
    setInterval(cb: () => void, intervalMs: number): unknown;
    clearInterval(id: unknown): void;
}
export declare class SystemQosClock implements QosClock {
    nowMs(): number;
    setInterval(cb: () => void, intervalMs: number): ReturnType<typeof setInterval>;
    clearInterval(id: unknown): void;
}
/**
 * Deterministic fake clock for unit tests and trace replays.
 */
export declare class ManualQosClock implements QosClock {
    private _nowMs;
    private _nextId;
    private readonly _timers;
    nowMs(): number;
    setInterval(cb: () => void, intervalMs: number): number;
    clearInterval(id: unknown): void;
    /**
     * Advances clock time and executes due interval callbacks.
     */
    advanceBy(deltaMs: number): void;
    /**
     * Moves clock directly to target timestamp (monotonic only).
     */
    advanceTo(targetMs: number): void;
    reset(nowMs?: number): void;
    private runDueTimers;
}
//# sourceMappingURL=clock.d.ts.map