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
function readString(obj, key) {
    const value = obj[key];
    if (typeof value !== 'string') {
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
function roundMetric(sum, count) {
    return count > 0
        ? Math.round((sum / count) * 1000) / 1000
        : undefined;
}
function isSelectedCandidatePair(entry) {
    const state = readString(entry, 'state');
    return (entry.selected === true ||
        (entry.nominated === true &&
            (state === undefined || state === 'succeeded')));
}
function resolveCandidatePair(outbound, transportsById, candidatePairsById, selectedCandidatePairs) {
    const transportId = readString(outbound, 'transportId');
    if (transportId) {
        const transport = transportsById.get(transportId);
        const selectedCandidatePairId = readString(transport ?? {}, 'selectedCandidatePairId');
        if (selectedCandidatePairId) {
            const candidatePair = candidatePairsById.get(selectedCandidatePairId);
            if (candidatePair) {
                return candidatePair;
            }
        }
    }
    if (selectedCandidatePairs.length === 1) {
        return selectedCandidatePairs[0];
    }
    return undefined;
}
class ProducerSenderStatsProvider {
    constructor(adapter) {
        this.adapter = adapter;
        this.lastRemoteTimestampsMs = new Map();
    }
    async getSnapshot() {
        const base = this.adapter.getSnapshotBase();
        const report = await this.adapter.getStatsReport();
        const remoteById = new Map();
        const candidatePairsById = new Map();
        const selectedCandidatePairs = [];
        const transportsById = new Map();
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
            if (type === 'candidate-pair') {
                const id = readString(entry, 'id');
                if (id) {
                    candidatePairsById.set(id, entry);
                }
                if (isSelectedCandidatePair(entry)) {
                    selectedCandidatePairs.push(entry);
                }
                return;
            }
            if (type === 'transport') {
                const id = readString(entry, 'id');
                if (id) {
                    transportsById.set(id, entry);
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
        let transportRttSumMs = 0;
        let transportRttCount = 0;
        let fallbackRttSumMs = 0;
        let fallbackRttCount = 0;
        let jitterSumMs = 0;
        let jitterCount = 0;
        const nextRemoteTimestampsMs = new Map(this.lastRemoteTimestampsMs);
        const seenCandidatePairIds = new Set();
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
            const candidatePair = resolveCandidatePair(outbound, transportsById, candidatePairsById, selectedCandidatePairs);
            const candidatePairId = readString(candidatePair ?? {}, 'id');
            if (!candidatePairId || !seenCandidatePairIds.has(candidatePairId)) {
                const transportRttMs = normalizeTimeToMs(readNumber(candidatePair ?? {}, 'currentRoundTripTime'));
                if (typeof transportRttMs === 'number') {
                    transportRttSumMs += transportRttMs;
                    transportRttCount++;
                }
                if (candidatePairId) {
                    seenCandidatePairIds.add(candidatePairId);
                }
            }
            const remoteId = readString(outbound, 'remoteId');
            const remote = remoteId ? remoteById.get(remoteId) : undefined;
            const lossFromRemote = remote
                ? readNumber(remote, 'packetsLost')
                : readNumber(outbound, 'packetsLost');
            packetsLost += lossFromRemote ?? 0;
            const remoteTimestampMs = readNumber(remote ?? {}, 'timestamp');
            const previousRemoteTimestampMs = remoteId
                ? this.lastRemoteTimestampsMs.get(remoteId)
                : undefined;
            const remoteMetricsFresh = Boolean(remote) &&
                (typeof remoteTimestampMs !== 'number' ||
                    previousRemoteTimestampMs === undefined ||
                    remoteTimestampMs > previousRemoteTimestampMs);
            if (remoteId && typeof remoteTimestampMs === 'number' && remoteMetricsFresh) {
                nextRemoteTimestampsMs.set(remoteId, remoteTimestampMs);
            }
            const fallbackRttMs = remoteMetricsFresh
                ? normalizeTimeToMs(readNumber(remote ?? {}, 'roundTripTime'))
                : remote
                    ? undefined
                    : normalizeTimeToMs(readNumber(outbound, 'roundTripTime'));
            if (typeof fallbackRttMs === 'number') {
                fallbackRttSumMs += fallbackRttMs;
                fallbackRttCount++;
            }
            const jitterMs = remoteMetricsFresh
                ? normalizeTimeToMs(readNumber(remote ?? {}, 'jitter'))
                : remote
                    ? undefined
                    : normalizeTimeToMs(readNumber(outbound, 'jitter'));
            if (typeof jitterMs === 'number') {
                jitterSumMs += jitterMs;
                jitterCount++;
            }
        }
        this.lastRemoteTimestampsMs = nextRemoteTimestampsMs;
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
            roundTripTimeMs: roundMetric(transportRttCount > 0 ? transportRttSumMs : fallbackRttSumMs, transportRttCount > 0 ? transportRttCount : fallbackRttCount),
            jitterMs: roundMetric(jitterSumMs, jitterCount),
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
