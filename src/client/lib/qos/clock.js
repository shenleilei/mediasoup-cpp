"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.ManualQosClock = exports.SystemQosClock = void 0;
class SystemQosClock {
    nowMs() {
        return Date.now();
    }
    setInterval(cb, intervalMs) {
        return setInterval(cb, intervalMs);
    }
    clearInterval(id) {
        clearInterval(id);
    }
}
exports.SystemQosClock = SystemQosClock;
/**
 * Deterministic fake clock for unit tests and trace replays.
 */
class ManualQosClock {
    constructor() {
        this._nowMs = 0;
        this._nextId = 1;
        this._timers = new Map();
    }
    nowMs() {
        return this._nowMs;
    }
    setInterval(cb, intervalMs) {
        const safeIntervalMs = Math.max(1, Math.floor(intervalMs));
        const id = this._nextId++;
        this._timers.set(id, {
            id,
            cb,
            intervalMs: safeIntervalMs,
            nextRunAtMs: this._nowMs + safeIntervalMs,
        });
        return id;
    }
    clearInterval(id) {
        if (typeof id !== 'number') {
            return;
        }
        this._timers.delete(id);
    }
    /**
     * Advances clock time and executes due interval callbacks.
     */
    advanceBy(deltaMs) {
        const safeDeltaMs = Math.max(0, Math.floor(deltaMs));
        if (safeDeltaMs === 0) {
            return;
        }
        this._nowMs += safeDeltaMs;
        this.runDueTimers();
    }
    /**
     * Moves clock directly to target timestamp (monotonic only).
     */
    advanceTo(targetMs) {
        if (targetMs <= this._nowMs) {
            return;
        }
        this._nowMs = Math.floor(targetMs);
        this.runDueTimers();
    }
    reset(nowMs = 0) {
        this._nowMs = Math.max(0, Math.floor(nowMs));
        this._timers.clear();
    }
    runDueTimers() {
        let hasWork = true;
        // Keep ticking while there are timers scheduled in the past.
        while (hasWork) {
            hasWork = false;
            for (const timer of this._timers.values()) {
                if (timer.nextRunAtMs > this._nowMs) {
                    continue;
                }
                hasWork = true;
                timer.nextRunAtMs += timer.intervalMs;
                timer.cb();
            }
        }
    }
}
exports.ManualQosClock = ManualQosClock;
