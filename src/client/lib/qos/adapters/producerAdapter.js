"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.MediasoupProducerAdapter = void 0;
function mapEncodingParameters(parameters) {
    const mapped = {};
    if (typeof parameters.maxBitrateBps === 'number') {
        mapped.maxBitrate = parameters.maxBitrateBps;
    }
    if (typeof parameters.maxFramerate === 'number') {
        mapped.maxFramerate = parameters.maxFramerate;
    }
    if (typeof parameters.scaleResolutionDownBy === 'number') {
        mapped.scaleResolutionDownBy = parameters.scaleResolutionDownBy;
    }
    if (typeof parameters.adaptivePtime === 'boolean') {
        mapped.adaptivePtime = parameters.adaptivePtime;
    }
    return mapped;
}
class MediasoupProducerAdapter {
    constructor(producer, source) {
        this.producer = producer;
        this.source = source;
    }
    getSnapshotBase() {
        const kind = this.producer.kind;
        let configuredBitrateBps;
        const encodings = this.producer.rtpParameters?.encodings;
        if (Array.isArray(encodings) && encodings.length > 0) {
            const sum = encodings.reduce((acc, encoding) => {
                const bitrate = typeof encoding?.maxBitrate === 'number' &&
                    Number.isFinite(encoding.maxBitrate) &&
                    encoding.maxBitrate > 0
                    ? encoding.maxBitrate
                    : 0;
                return acc + bitrate;
            }, 0);
            if (sum > 0) {
                configuredBitrateBps = sum;
            }
        }
        return {
            source: this.source,
            kind,
            producerId: this.producer.id,
            trackId: this.producer.track?.id,
            configuredBitrateBps,
        };
    }
    async getStatsReport() {
        return this.producer.getStats();
    }
    async setEncodingParameters(parameters) {
        if (this.producer.closed) {
            return { applied: false, reason: 'producer-closed' };
        }
        try {
            await this.producer.setRtpEncodingParameters(mapEncodingParameters(parameters));
            return { applied: true };
        }
        catch (error) {
            return { applied: false, reason: error.message };
        }
    }
    async setMaxSpatialLayer(spatialLayer) {
        if (this.producer.closed) {
            return { applied: false, reason: 'producer-closed' };
        }
        if (this.producer.kind !== 'video') {
            return { applied: false, reason: 'not-video-producer' };
        }
        try {
            await this.producer.setMaxSpatialLayer(spatialLayer);
            return { applied: true };
        }
        catch (error) {
            return { applied: false, reason: error.message };
        }
    }
    async pauseUpstream() {
        if (this.producer.closed) {
            return { applied: false, reason: 'producer-closed' };
        }
        try {
            this.producer.pause();
            return { applied: true };
        }
        catch (error) {
            return { applied: false, reason: error.message };
        }
    }
    async resumeUpstream() {
        if (this.producer.closed) {
            return { applied: false, reason: 'producer-closed' };
        }
        try {
            this.producer.resume();
            return { applied: true };
        }
        catch (error) {
            return { applied: false, reason: error.message };
        }
    }
}
exports.MediasoupProducerAdapter = MediasoupProducerAdapter;
