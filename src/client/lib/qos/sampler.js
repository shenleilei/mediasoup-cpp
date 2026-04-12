"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.IntervalQosSampler = void 0;
function toError(error) {
    if (error instanceof Error) {
        return error;
    }
    return new Error(String(error));
}
/**
 * Small clock-driven sampler used by later controller wiring.
 * This stays intentionally detached from Producer/Transport objects.
 */
class IntervalQosSampler {
    constructor(_clock, _provider, intervalMs) {
        this._clock = _clock;
        this._provider = _provider;
        this._running = false;
        this._sampling = false;
        if (!Number.isFinite(intervalMs) || intervalMs <= 0) {
            throw new TypeError('intervalMs must be a positive finite number');
        }
        this._intervalMs = Math.floor(intervalMs);
    }
    get running() {
        return this._running;
    }
    get intervalMs() {
        return this._intervalMs;
    }
    updateIntervalMs(intervalMs) {
        if (!Number.isFinite(intervalMs) || intervalMs <= 0) {
            throw new TypeError('intervalMs must be a positive finite number');
        }
        const nextIntervalMs = Math.floor(intervalMs);
        if (nextIntervalMs === this._intervalMs) {
            return;
        }
        this._intervalMs = nextIntervalMs;
        if (this._running) {
            this.stop();
            if (this._onSample) {
                this.start(this._onSample, this._onError);
            }
        }
    }
    async sampleOnce() {
        return this._provider.getSnapshot();
    }
    start(onSample, onError) {
        if (this._running) {
            return;
        }
        this._onSample = onSample;
        this._onError = onError;
        this._running = true;
        this._intervalId = this._clock.setInterval(() => {
            void this.runTick(onSample, onError);
        }, this._intervalMs);
    }
    stop() {
        if (!this._running) {
            return;
        }
        this._running = false;
        if (this._intervalId !== undefined) {
            this._clock.clearInterval(this._intervalId);
            this._intervalId = undefined;
        }
    }
    async runTick(onSample, onError) {
        if (this._sampling) {
            return;
        }
        this._sampling = true;
        try {
            const snapshot = await this._provider.getSnapshot();
            await onSample(snapshot);
        }
        catch (error) {
            onError?.(toError(error));
        }
        finally {
            this._sampling = false;
        }
    }
}
exports.IntervalQosSampler = IntervalQosSampler;
