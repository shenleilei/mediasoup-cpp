"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.DownlinkSampler = void 0;

/**
 * Collects receiver-side WebRTC stats for all active consumers on a
 * recv transport and merges them with UI-level hints (visible, pinned, etc.).
 *
 * Usage:
 *   const sampler = new DownlinkSampler(recvTransport);
 *   sampler.setHints(consumerId, { visible: true, pinned: false, ... });
 *   const snapshot = await sampler.sample(subscriberPeerId);
 */
class DownlinkSampler {
    constructor(recvTransport) {
        this._transport = recvTransport;
        /** @type {Map<string, object>} consumerId -> hint */
        this._hints = new Map();
        this._prevBytesReceived = new Map();
        this._prevTimestamp = new Map();
    }

    /**
     * Set UI-level hints for a consumer.
     * @param {string} consumerId
     * @param {object} hint  { producerId, visible, pinned, activeSpeaker, isScreenShare, targetWidth, targetHeight }
     */
    setHints(consumerId, hint) {
        this._hints.set(consumerId, hint);
    }

    removeHints(consumerId) {
        this._hints.delete(consumerId);
        this._prevBytesReceived.delete(consumerId);
        this._prevTimestamp.delete(consumerId);
    }

    /**
     * Sample current recv transport stats and return a subscriptions array
     * plus transport-level metrics.
     * @param {string} subscriberPeerId
     * @returns {Promise<{transport: object, subscriptions: Array}>}
     */
    async sample(subscriberPeerId) {
        const pc = this._transport?._handler?._pc;
        if (!pc) {
            return { transport: { availableIncomingBitrate: 0, currentRoundTripTime: 0 }, subscriptions: [] };
        }

        const rawStats = await pc.getStats();
        const inboundBySSRC = new Map();
        let availableIncomingBitrate = 0;
        let currentRoundTripTime = 0;

        for (const report of rawStats.values()) {
            if (report.type === 'inbound-rtp' && report.kind === 'video') {
                inboundBySSRC.set(report.ssrc, report);
            }
            if (report.type === 'candidate-pair' && report.nominated) {
                availableIncomingBitrate = report.availableIncomingBitrate || 0;
                currentRoundTripTime = report.currentRoundTripTime || 0;
            }
        }

        const subscriptions = [];
        const matchedReportIds = new Set();
        for (const [consumerId, hint] of this._hints) {
            let stats = null;
            const hintSsrc = Number(hint?.ssrc);
            if (Number.isFinite(hintSsrc) && inboundBySSRC.has(hintSsrc)) {
                const direct = inboundBySSRC.get(hintSsrc);
                if (direct?.id) matchedReportIds.add(direct.id);
                stats = direct;
            }

            if (!stats && hint?.trackId) {
                for (const report of inboundBySSRC.values()) {
                    if (report.trackIdentifier === hint.trackId) {
                        if (report?.id) matchedReportIds.add(report.id);
                        stats = report;
                        break;
                    }
                }
            }

            if (!stats && hint?.mid) {
                for (const report of inboundBySSRC.values()) {
                    if (String(report.mid ?? '') === String(hint.mid)) {
                        if (report?.id) matchedReportIds.add(report.id);
                        stats = report;
                        break;
                    }
                }
            }

            if (!stats) {
                for (const report of inboundBySSRC.values()) {
                    if (report?.id && matchedReportIds.has(report.id)) continue;
                    if (report?.id) matchedReportIds.add(report.id);
                    stats = report;
                    break;
                }
            }

            subscriptions.push({
                consumerId,
                producerId: hint.producerId || '',
                visible: !!hint.visible,
                pinned: !!hint.pinned,
                activeSpeaker: !!hint.activeSpeaker,
                isScreenShare: !!hint.isScreenShare,
                targetWidth: hint.targetWidth || 0,
                targetHeight: hint.targetHeight || 0,
                packetsLost: stats?.packetsLost || 0,
                jitter: stats?.jitter || 0,
                framesPerSecond: stats?.framesPerSecond || 0,
                frameWidth: stats?.frameWidth || 0,
                frameHeight: stats?.frameHeight || 0,
                freezeRate: computeFreezeRate(stats),
            });
        }

        return {
            transport: { availableIncomingBitrate, currentRoundTripTime },
            subscriptions,
        };
    }
}

function computeFreezeRate(stats) {
    if (!stats) return 0;
    const totalFrames = stats.framesDecoded || 0;
    const freezeCount = stats.freezeCount || 0;
    if (totalFrames === 0) return 0;
    // freezeCount / framesDecoded is a rough proxy; totalFreezesDuration / totalPlaybackDuration
    // is more accurate when available.
    if (typeof stats.totalFreezesDuration === 'number' && typeof stats.totalPlaybackDuration === 'number' && stats.totalPlaybackDuration > 0) {
        return stats.totalFreezesDuration / stats.totalPlaybackDuration;
    }
    return freezeCount > 0 ? freezeCount / totalFrames : 0;
}

exports.DownlinkSampler = DownlinkSampler;
