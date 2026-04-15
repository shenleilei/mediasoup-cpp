"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.DownlinkReporter = void 0;
const downlinkProtocol_1 = require("./downlinkProtocol");

/**
 * Thin orchestrator that periodically samples recv-transport stats,
 * merges UI hints, and sends a downlinkClientStats snapshot to the server.
 *
 * Usage:
 *   const reporter = new DownlinkReporter({ sampler, hints, send, peerId });
 *   reporter.start();   // begins periodic reporting
 *   reporter.stop();
 *   reporter.reportNow(); // force an immediate report
 */
class DownlinkReporter {
    /**
     * @param {object} opts
     * @param {import('./downlinkSampler').DownlinkSampler} opts.sampler
     * @param {import('./downlinkHints').DownlinkHints} opts.hints
     * @param {(method: string, data: object) => Promise<any>} opts.send  signaling send function
     * @param {string} opts.peerId  subscriber peer id
     * @param {number} [opts.intervalMs=2000]
     */
    constructor({ sampler, hints, send, peerId, intervalMs = 2000, logger = console }) {
        this._sampler = sampler;
        this._hints = hints;
        this._send = send;
        this._peerId = peerId;
        this._intervalMs = intervalMs;
        this._seq = 0;
        this._timerId = null;
        this._reporting = false;
        this._logger = logger;
        this._consecutiveErrors = 0;
    }

    get running() { return this._timerId !== null; }

    start() {
        if (this._timerId !== null) return;
        this._timerId = setInterval(() => { void this._tick(); }, this._intervalMs);
    }

    stop() {
        if (this._timerId !== null) {
            clearInterval(this._timerId);
            this._timerId = null;
        }
    }

    async reportNow() {
        return this._tick();
    }

    async _tick() {
        if (this._reporting) return;
        this._reporting = true;
        try {
            // Sync hints into sampler
            for (const [cid, hint] of this._hints.getAll()) {
                this._sampler.setHints(cid, hint);
            }
            const nextSeq = this._seq >= downlinkProtocol_1.DOWNLINK_MAX_SEQ
                ? 0
                : this._seq + 1;
            this._seq = nextSeq;
            const { transport, subscriptions } = await this._sampler.sample(this._peerId);
            const snapshot = (0, downlinkProtocol_1.serializeDownlinkSnapshot)({
                seq: nextSeq,
                subscriberPeerId: this._peerId,
                transport,
                subscriptions,
            });
            await this._send('downlinkClientStats', snapshot);
            this._consecutiveErrors = 0;
        } catch (e) {
            this._consecutiveErrors++;
            if (this._consecutiveErrors <= 3 || this._consecutiveErrors % 10 === 0) {
                this._logger.warn(
                    `[DownlinkReporter] tick failed (count=${this._consecutiveErrors}): ${e.message || e}`
                );
            }
        } finally {
            this._reporting = false;
        }
    }
}

exports.DownlinkReporter = DownlinkReporter;
