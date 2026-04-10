export interface QosClock {
	nowMs(): number;
	setInterval(cb: () => void, intervalMs: number): unknown;
	clearInterval(id: unknown): void;
}

export class SystemQosClock implements QosClock {
	nowMs(): number {
		return Date.now();
	}

	setInterval(
		cb: () => void,
		intervalMs: number
	): ReturnType<typeof setInterval> {
		return setInterval(cb, intervalMs);
	}

	clearInterval(id: unknown): void {
		clearInterval(id as ReturnType<typeof setInterval>);
	}
}

type ManualTimer = {
	id: number;
	cb: () => void;
	intervalMs: number;
	nextRunAtMs: number;
};

/**
 * Deterministic fake clock for unit tests and trace replays.
 */
export class ManualQosClock implements QosClock {
	private _nowMs = 0;
	private _nextId = 1;
	private readonly _timers: Map<number, ManualTimer> = new Map();

	nowMs(): number {
		return this._nowMs;
	}

	setInterval(cb: () => void, intervalMs: number): number {
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

	clearInterval(id: unknown): void {
		if (typeof id !== 'number') {
			return;
		}

		this._timers.delete(id);
	}

	/**
	 * Advances clock time and executes due interval callbacks.
	 */
	advanceBy(deltaMs: number): void {
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
	advanceTo(targetMs: number): void {
		if (targetMs <= this._nowMs) {
			return;
		}

		this._nowMs = Math.floor(targetMs);
		this.runDueTimers();
	}

	reset(nowMs = 0): void {
		this._nowMs = Math.max(0, Math.floor(nowMs));
		this._timers.clear();
	}

	private runDueTimers(): void {
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
