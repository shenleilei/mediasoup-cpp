"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.QosActionExecutor = void 0;
exports.defaultQosActionKeyResolver = defaultQosActionKeyResolver;
function toError(error) {
    if (error instanceof Error) {
        return error;
    }
    return new Error(String(error));
}
function normalizeNumber(value) {
    if (typeof value !== 'number' || !Number.isFinite(value)) {
        return '';
    }
    return String(value);
}
function defaultQosActionKeyResolver(action) {
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
class QosActionExecutor {
    constructor(_sink, options = {}) {
        this._sink = _sink;
        this._enableIdempotence = options.enableIdempotence ?? true;
        this._stopOnError = options.stopOnError ?? false;
        this._keyResolver = options.keyResolver ?? defaultQosActionKeyResolver;
        this._onError = options.onError;
    }
    get lastAppliedActionKey() {
        return this._lastAppliedActionKey;
    }
    resetIdempotenceState() {
        this._lastAppliedActionKey = undefined;
    }
    async execute(action) {
        const actionKey = this._keyResolver(action);
        if (action.type === 'noop') {
            return {
                status: 'skipped_noop',
                action,
                actionKey,
            };
        }
        if (this._enableIdempotence &&
            this._lastAppliedActionKey !== undefined &&
            actionKey === this._lastAppliedActionKey) {
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
        }
        catch (error) {
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
    async executeMany(actions) {
        const results = [];
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
exports.QosActionExecutor = QosActionExecutor;
