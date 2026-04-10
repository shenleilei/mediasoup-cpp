import { DEFAULT_TRACE_BUFFER_SIZE } from './constants';
import type { QosTraceEntry } from './types';

/**
 * Fixed-capacity trace buffer for QoS diagnostics.
 */
export class QosTraceBuffer<TEntry = QosTraceEntry> {
	private readonly _capacity: number;
	private readonly _entries: TEntry[] = [];

	constructor(capacity = DEFAULT_TRACE_BUFFER_SIZE) {
		this._capacity = Math.max(1, Math.floor(capacity));
	}

	get capacity(): number {
		return this._capacity;
	}

	get size(): number {
		return this._entries.length;
	}

	append(entry: TEntry): void {
		this._entries.push(entry);

		if (this._entries.length > this._capacity) {
			this._entries.splice(0, this._entries.length - this._capacity);
		}
	}

	getEntries(): TEntry[] {
		return this._entries.slice();
	}

	clear(): void {
		this._entries.length = 0;
	}

	appendTraceEntry(entry: TEntry): void {
		this.append(entry);
	}

	getTraceEntries(): TEntry[] {
		return this.getEntries();
	}

	clearTrace(): void {
		this.clear();
	}
}

export function createQosTraceBuffer<TEntry = QosTraceEntry>(
	capacity?: number
): QosTraceBuffer<TEntry> {
	return new QosTraceBuffer<TEntry>(capacity);
}
