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
    constructor(recvTransport, options = {}) {
        this._transport = recvTransport;
        /** @type {Map<string, object>} consumerId -> hint */
        this._hints = new Map();
        this._prevPacketsLost = new Map();
        this._prevPacketsReceived = new Map();
        this._statsProvider =
            typeof options.statsProvider === 'function'
                ? options.statsProvider
                : async () => this._transport?.getStats?.();
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
        this._prevPacketsLost.delete(consumerId);
        this._prevPacketsReceived.delete(consumerId);
    }

    /**
     * Sample current recv transport stats and return a subscriptions array
     * plus transport-level metrics.
     * @param {string} subscriberPeerId
     * @returns {Promise<{transport: object, subscriptions: Array}>}
     */
    async sample(subscriberPeerId) {
        const rawStats = await this._statsProvider();
        if (!rawStats || typeof rawStats.values !== 'function') {
            return { transport: { availableIncomingBitrate: 0, currentRoundTripTime: 0 }, subscriptions: [] };
        }
        const inboundReports = [];
        let availableIncomingBitrate = 0;
        let currentRoundTripTime = 0;

        for (const report of rawStats.values()) {
            if (report.type === 'inbound-rtp' && report.kind === 'video') {
                inboundReports.push(report);
            }
            if (report.type === 'candidate-pair' && report.nominated) {
                availableIncomingBitrate = report.availableIncomingBitrate || 0;
                currentRoundTripTime = report.currentRoundTripTime || 0;
            }
        }

        const subscriptions = [];
        const matchedReports = new Set();
        for (const [consumerId, hint] of this._hints) {
            const stats = selectInboundReport(inboundReports, hint, matchedReports);
            const lossPercent = computeLossPercent(
                stats,
                this._prevPacketsLost.get(consumerId),
                this._prevPacketsReceived.get(consumerId),
            );
            if (stats) {
                this._prevPacketsLost.set(consumerId, Number(stats.packetsLost) || 0);
                this._prevPacketsReceived.set(consumerId, Number(stats.packetsReceived) || 0);
            }

            subscriptions.push({
                consumerId,
                producerId: hint.producerId || '',
                kind: hint.kind === 'audio' ? 'audio' : 'video',
                visible: !!hint.visible,
                pinned: !!hint.pinned,
                activeSpeaker: !!hint.activeSpeaker,
                isScreenShare: !!hint.isScreenShare,
                targetWidth: hint.targetWidth || 0,
                targetHeight: hint.targetHeight || 0,
                packetsLost: lossPercent,
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

function selectInboundReport(reports, hint, matchedReports) {
    const explicitMatches = reports.filter(report => matchesHint(report, hint));
    for (const report of explicitMatches) {
        if (!matchedReports.has(report)) {
            matchedReports.add(report);
            return report;
        }
    }
    for (const report of reports) {
        if (!matchedReports.has(report)) {
            matchedReports.add(report);
            return report;
        }
    }
    return null;
}

function matchesHint(report, hint = {}) {
    if (!report || !hint) return false;
    if (Number.isFinite(hint.ssrc) && Number(report.ssrc) === Number(hint.ssrc)) {
        return true;
    }
    if (typeof hint.mid === 'string' && hint.mid && report.mid === hint.mid) {
        return true;
    }
    if (typeof hint.trackIdentifier === 'string' &&
        hint.trackIdentifier &&
        report.trackIdentifier === hint.trackIdentifier) {
        return true;
    }
    return false;
}

function computeLossPercent(stats, previousLost, previousReceived) {
    if (!stats) return 0;
    const currentLost = Number(stats.packetsLost);
    const currentReceived = Number(stats.packetsReceived);
    if (!Number.isFinite(currentLost) || !Number.isFinite(currentReceived)) {
        return 0;
    }
    if (!Number.isFinite(previousLost) || !Number.isFinite(previousReceived)) {
        return 0;
    }
    const deltaLost = currentLost - previousLost;
    const deltaReceived = currentReceived - previousReceived;
    if (deltaLost < 0 || deltaReceived < 0) {
        return 0;
    }
    const total = deltaLost + deltaReceived;
    if (total <= 0) {
        return 0;
    }
    return Math.max(0, Math.min(100, Math.round((deltaLost / total) * 100)));
}

exports.DownlinkSampler = DownlinkSampler;
