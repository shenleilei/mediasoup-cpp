import type { PlannedQosAction } from './types';

export type QosActionExecutionStatus =
	| 'applied'
	| 'skipped_noop'
	| 'skipped_idempotent'
	| 'failed';

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

function toError(error: unknown): Error {
	if (error instanceof Error) {
		return error;
	}

	return new Error(String(error));
}

function normalizeNumber(value: number | undefined): string {
	if (typeof value !== 'number' || !Number.isFinite(value)) {
		return '';
	}

	return String(value);
}

export function defaultQosActionKeyResolver(action: PlannedQosAction): string {
	switch (action.type) {
		case 'setEncodingParameters': {
			const params = action.encodingParameters;

			return [
				action.type,
				action.level,
				normalizeNumber(params.maxBitrateBps),
				normalizeNumber(params.maxFramerate),
				normalizeNumber(params.scaleResolutionDownBy),
				String(params.adaptivePtime ?? ''),
			].join(':');
		}

		case 'setMaxSpatialLayer': {
			return `${action.type}:${action.level}:${action.spatialLayer}`;
		}

		case 'enterAudioOnly':
		case 'pauseUpstream':
		case 'resumeUpstream':
		case 'exitAudioOnly':
		case 'noop': {
			return `${action.type}:${action.level}`;
		}
	}
}

/**
 * Executes planner actions against an injected sink.
 * The executor itself is generic and has no dependency on Producer/Transport.
 */
export class QosActionExecutor {
	private _lastAppliedActionKey?: string;
	private readonly _enableIdempotence: boolean;
	private readonly _stopOnError: boolean;
	private readonly _keyResolver: QosActionKeyResolver;
	private readonly _onError?: (
		error: Error,
		action: PlannedQosAction,
		actionKey: string
	) => void;

	constructor(
		private readonly _sink: QosActionSink,
		options: QosActionExecutorOptions = {}
	) {
		this._enableIdempotence = options.enableIdempotence ?? true;
		this._stopOnError = options.stopOnError ?? false;
		this._keyResolver = options.keyResolver ?? defaultQosActionKeyResolver;
		this._onError = options.onError;
	}

	get lastAppliedActionKey(): string | undefined {
		return this._lastAppliedActionKey;
	}

	resetIdempotenceState(): void {
		this._lastAppliedActionKey = undefined;
	}

	async execute(action: PlannedQosAction): Promise<QosActionExecutionResult> {
		const actionKey = this._keyResolver(action);

		if (action.type === 'noop') {
			return {
				status: 'skipped_noop',
				action,
				actionKey,
			};
		}

		if (
			this._enableIdempotence &&
			this._lastAppliedActionKey !== undefined &&
			actionKey === this._lastAppliedActionKey
		) {
			return {
				status: 'skipped_idempotent',
				action,
				actionKey,
			};
		}

		try {
			await this._sink.execute(action);
			this._lastAppliedActionKey = actionKey;

			return {
				status: 'applied',
				action,
				actionKey,
			};
		} catch (error) {
			const normalized = toError(error);

			this._onError?.(normalized, action, actionKey);

			return {
				status: 'failed',
				action,
				actionKey,
				error: normalized,
			};
		}
	}

	async executeMany(
		actions: PlannedQosAction[]
	): Promise<QosActionExecutionResult[]> {
		const results: QosActionExecutionResult[] = [];

		for (const action of actions) {
			const result = await this.execute(action);

			results.push(result);

			if (this._stopOnError && result.status === 'failed') {
				break;
			}
		}

		return results;
	}
}
