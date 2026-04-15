"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.DownlinkHints = void 0;

/**
 * Manages per-consumer UI hints (visible, pinned, activeSpeaker, etc.)
 * and provides a simple API for the application layer to update them.
 *
 * Usage:
 *   const hints = new DownlinkHints();
 *   hints.set(consumerId, { producerId, visible: true, pinned: false, ... });
 *   hints.setVisible(consumerId, false);
 *   const all = hints.getAll();
 */
class DownlinkHints {
    constructor() {
        /** @type {Map<string, object>} */
        this._hints = new Map();
    }

    /**
     * Set or replace full hint for a consumer.
     * @param {string} consumerId
     * @param {object} hint
     */
    set(consumerId, hint) {
        this._hints.set(consumerId, {
            producerId: hint.producerId || '',
            kind: hint.kind === 'audio' ? 'audio' : 'video',
            ssrc: Number.isFinite(hint.ssrc) ? hint.ssrc : undefined,
            mid: typeof hint.mid === 'string' ? hint.mid : undefined,
            trackIdentifier: typeof hint.trackIdentifier === 'string' ? hint.trackIdentifier : undefined,
            visible: hint.visible !== false,
            pinned: !!hint.pinned,
            activeSpeaker: !!hint.activeSpeaker,
            isScreenShare: !!hint.isScreenShare,
            targetWidth: hint.targetWidth || 0,
            targetHeight: hint.targetHeight || 0,
        });
    }

    remove(consumerId) {
        this._hints.delete(consumerId);
    }

    setVisible(consumerId, visible) {
        const h = this._hints.get(consumerId);
        if (h) h.visible = !!visible;
        else console.warn(`[DownlinkHints] setVisible ignored for unknown consumerId=${consumerId}`);
    }

    setPinned(consumerId, pinned) {
        const h = this._hints.get(consumerId);
        if (h) h.pinned = !!pinned;
        else console.warn(`[DownlinkHints] setPinned ignored for unknown consumerId=${consumerId}`);
    }

    setActiveSpeaker(consumerId, active) {
        const h = this._hints.get(consumerId);
        if (h) h.activeSpeaker = !!active;
        else console.warn(`[DownlinkHints] setActiveSpeaker ignored for unknown consumerId=${consumerId}`);
    }

    setTargetSize(consumerId, width, height) {
        const h = this._hints.get(consumerId);
        if (h) {
            h.targetWidth = width || 0;
            h.targetHeight = height || 0;
        }
        else console.warn(`[DownlinkHints] setTargetSize ignored for unknown consumerId=${consumerId}`);
    }

    get(consumerId) {
        return this._hints.get(consumerId) || null;
    }

    getAll() {
        return this._hints;
    }

    clear() {
        this._hints.clear();
    }
}

exports.DownlinkHints = DownlinkHints;
