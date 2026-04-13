"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.DOWNLINK_SCHEMA_V1 = 'mediasoup.qos.downlink.client.v1';
exports.DOWNLINK_SCHEMA_V1_LEGACY = 'mediasoup.downlink.v1';
exports.DOWNLINK_MAX_SUBSCRIPTIONS = 64;
exports.serializeDownlinkSnapshot = serializeDownlinkSnapshot;
exports.parseDownlinkSnapshot = parseDownlinkSnapshot;

/**
 * Build a wire-format downlink snapshot from hints + stats.
 * @param {object} opts
 * @param {number} opts.seq
 * @param {string} opts.subscriberPeerId
 * @param {{availableIncomingBitrate:number, currentRoundTripTime:number}} opts.transport
 * @param {Array} opts.subscriptions
 * @returns {object}
 */
function serializeDownlinkSnapshot({ seq, subscriberPeerId, transport, subscriptions }) {
    // Enforce client-side cap to avoid wasting bandwidth on oversized payloads
    // that the server would reject anyway.
    const capped = subscriptions.length > exports.DOWNLINK_MAX_SUBSCRIPTIONS
        ? subscriptions.slice(0, exports.DOWNLINK_MAX_SUBSCRIPTIONS)
        : subscriptions;
    if (capped !== subscriptions) {
        console.warn(
            `[downlinkProtocol] Truncated ${subscriptions.length} subscriptions to ${exports.DOWNLINK_MAX_SUBSCRIPTIONS}`
        );
    }
    return {
        schema: exports.DOWNLINK_SCHEMA_V1,
        seq,
        tsMs: Date.now(),
        subscriberPeerId,
        transport: {
            availableIncomingBitrate: transport.availableIncomingBitrate || 0,
            currentRoundTripTime: transport.currentRoundTripTime || 0,
        },
        subscriptions: capped.map(s => ({
            consumerId: s.consumerId,
            producerId: s.producerId,
            visible: !!s.visible,
            pinned: !!s.pinned,
            activeSpeaker: !!s.activeSpeaker,
            isScreenShare: !!s.isScreenShare,
            targetWidth: s.targetWidth || 0,
            targetHeight: s.targetHeight || 0,
            packetsLost: s.packetsLost || 0,
            jitter: s.jitter || 0,
            framesPerSecond: s.framesPerSecond || 0,
            frameWidth: s.frameWidth || 0,
            frameHeight: s.frameHeight || 0,
            freezeRate: s.freezeRate || 0,
        })),
    };
}

/**
 * Validate an incoming downlink snapshot (fail-fast).
 * @param {object} payload
 * @returns {object}
 */
function parseDownlinkSnapshot(payload) {
    if (!payload || typeof payload !== 'object') {
        throw new TypeError('downlink snapshot must be an object');
    }
    if (payload.schema !== exports.DOWNLINK_SCHEMA_V1 &&
        payload.schema !== exports.DOWNLINK_SCHEMA_V1_LEGACY) {
        throw new TypeError(`unsupported schema: ${payload.schema}`);
    }
    if (typeof payload.seq !== 'number' || payload.seq < 0) {
        throw new TypeError('seq must be a non-negative number');
    }
    if (payload.schema === exports.DOWNLINK_SCHEMA_V1_LEGACY) {
        payload.schema = exports.DOWNLINK_SCHEMA_V1;
    }
    return payload;
}
