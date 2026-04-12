"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.ProducerSenderStatsProvider = void 0;
function isObject(value) {
    return typeof value === 'object' && value !== null;
}
function readNumber(obj, key) {
    const value = obj[key];
    if (typeof value !== 'number' || !Number.isFinite(value)) {
        return undefined;
    }
    return value;
}
function normalizeTimeToMs(value) {
    if (typeof value !== 'number' || !Number.isFinite(value)) {
        return undefined;
    }
    // RTC stats often expose seconds for RTT/jitter in remote-inbound/outbound.
    if (value >= 0 && value <= 10) {
        return value * 1000;
    }
    return value;
}
function readQualityLimitationReason(value) {
    if (typeof value !== 'string') {
        return undefined;
    }
    if (value === 'bandwidth' ||
        value === 'cpu' ||
        value === 'other' ||
        value === 'none') {
        return value;
    }
    return 'unknown';
}
function appendQualityDurations(acc, value) {
    if (!isObject(value)) {
        return;
    }
    for (const [key, raw] of Object.entries(value)) {
        if (typeof raw !== 'number' || !Number.isFinite(raw)) {
            continue;
        }
        acc[key] = (acc[key] ?? 0) + raw;
    }
}
function snapshotFromBase(base) {
    return {
        ...base,
        timestampMs: Date.now(),
    };
}
class ProducerSenderStatsProvider {
    constructor(adapter) {
        this.adapter = adapter;
    }
    async getSnapshot() {
        const base = this.adapter.getSnapshotBase();
        const report = await this.adapter.getStatsReport();
        const remoteById = new Map();
        const outbounds = [];
        report.forEach(entry => {
            if (!isObject(entry)) {
                return;
            }
            const type = typeof entry.type === 'string' ? entry.type : '';
            if (type === 'remote-inbound-rtp') {
                const id = typeof entry.id === 'string' ? entry.id : undefined;
                if (id) {
                    remoteById.set(id, entry);
                }
                return;
            }
            if (type !== 'outbound-rtp') {
                return;
            }
            // Ignore RTCP stats rows.
            if (entry.isRemote === true) {
                return;
            }
            outbounds.push(entry);
        });
        if (outbounds.length === 0) {
            return snapshotFromBase(base);
        }
        let timestampMs = 0;
        let bytesSent = 0;
        let packetsSent = 0;
        let packetsLost = 0;
        let retransmittedPacketsSent = 0;
        let targetBitrateBps = 0;
        let frameWidth = 0;
        let frameHeight = 0;
        let framesPerSecond = 0;
        let qualityLimitationReason;
        const qualityLimitationDurationsSec = {};
        let rttSumMs = 0;
        let rttCount = 0;
        let jitterSumMs = 0;
        let jitterCount = 0;
        for (const outbound of outbounds) {
            const statTimestamp = readNumber(outbound, 'timestamp');
            if (typeof statTimestamp === 'number' &&
                Number.isFinite(statTimestamp) &&
                statTimestamp > timestampMs) {
                timestampMs = statTimestamp;
            }
            bytesSent += readNumber(outbound, 'bytesSent') ?? 0;
            packetsSent += readNumber(outbound, 'packetsSent') ?? 0;
            retransmittedPacketsSent +=
                readNumber(outbound, 'retransmittedPacketsSent') ?? 0;
            targetBitrateBps += readNumber(outbound, 'targetBitrate') ?? 0;
            frameWidth = Math.max(frameWidth, readNumber(outbound, 'frameWidth') ?? 0);
            frameHeight = Math.max(frameHeight, readNumber(outbound, 'frameHeight') ?? 0);
            framesPerSecond = Math.max(framesPerSecond, readNumber(outbound, 'framesPerSecond') ?? 0);
            if (!qualityLimitationReason) {
                qualityLimitationReason = readQualityLimitationReason(outbound.qualityLimitationReason);
            }
            appendQualityDurations(qualityLimitationDurationsSec, outbound.qualityLimitationDurations);
            const remoteId = typeof outbound.remoteId === 'string' ? outbound.remoteId : undefined;
            const remote = remoteId ? remoteById.get(remoteId) : undefined;
            const lossFromRemote = remote
                ? readNumber(remote, 'packetsLost')
                : readNumber(outbound, 'packetsLost');
            packetsLost += lossFromRemote ?? 0;
            const rttMs = normalizeTimeToMs((remote && readNumber(remote, 'roundTripTime')) ??
                readNumber(outbound, 'roundTripTime'));
            if (typeof rttMs === 'number') {
                rttSumMs += rttMs;
                rttCount++;
            }
            const jitterMs = normalizeTimeToMs((remote && readNumber(remote, 'jitter')) ??
                readNumber(outbound, 'jitter'));
            if (typeof jitterMs === 'number') {
                jitterSumMs += jitterMs;
                jitterCount++;
            }
        }
        if (timestampMs <= 0) {
            timestampMs = Date.now();
        }
        return {
            ...base,
            timestampMs,
            bytesSent: bytesSent > 0 ? bytesSent : undefined,
            packetsSent: packetsSent > 0 ? packetsSent : undefined,
            packetsLost: packetsLost > 0 ? packetsLost : undefined,
            retransmittedPacketsSent: retransmittedPacketsSent > 0 ? retransmittedPacketsSent : undefined,
            targetBitrateBps: targetBitrateBps > 0 ? targetBitrateBps : undefined,
            roundTripTimeMs: rttCount > 0
                ? Math.round((rttSumMs / rttCount) * 1000) / 1000
                : undefined,
            jitterMs: jitterCount > 0
                ? Math.round((jitterSumMs / jitterCount) * 1000) / 1000
                : undefined,
            frameWidth: frameWidth > 0 ? frameWidth : undefined,
            frameHeight: frameHeight > 0 ? frameHeight : undefined,
            framesPerSecond: framesPerSecond > 0 ? framesPerSecond : undefined,
            qualityLimitationReason: qualityLimitationReason ?? 'unknown',
            qualityLimitationDurationsSec: Object.keys(qualityLimitationDurationsSec).length > 0
                ? qualityLimitationDurationsSec
                : undefined,
        };
    }
}
exports.ProducerSenderStatsProvider = ProducerSenderStatsProvider;
