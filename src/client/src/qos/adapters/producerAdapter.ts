import type { Producer } from '../../Producer';
import type {
	QosEncodingParameters,
	QosSource,
	QosTrackKind,
	RawSenderSnapshot,
} from '../types';

export type ProducerAdapterActionResult = {
	applied: boolean;
	reason?: string;
};

export type ProducerAdapter = {
	getSnapshotBase(): Omit<RawSenderSnapshot, 'timestampMs'>;
	getStatsReport(): Promise<RTCStatsReport>;
	setEncodingParameters(
		parameters: QosEncodingParameters
	): Promise<ProducerAdapterActionResult>;
	setMaxSpatialLayer(
		spatialLayer: number
	): Promise<ProducerAdapterActionResult>;
	pauseUpstream(): Promise<ProducerAdapterActionResult>;
	resumeUpstream(): Promise<ProducerAdapterActionResult>;
};

function mapEncodingParameters(
	parameters: QosEncodingParameters
): RTCRtpEncodingParameters & { adaptivePtime?: boolean } {
	const mapped: RTCRtpEncodingParameters & { adaptivePtime?: boolean } = {};

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

export class MediasoupProducerAdapter implements ProducerAdapter {
	constructor(
		private readonly producer: Producer,
		private readonly source: QosSource
	) {}

	getSnapshotBase(): Omit<RawSenderSnapshot, 'timestampMs'> {
		const kind = this.producer.kind as QosTrackKind;

		let configuredBitrateBps: number | undefined;
		const encodings = this.producer.rtpParameters?.encodings;

		if (Array.isArray(encodings) && encodings.length > 0) {
			const sum = encodings.reduce((acc, encoding) => {
				const bitrate =
					typeof encoding?.maxBitrate === 'number' &&
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

	async getStatsReport(): Promise<RTCStatsReport> {
		return this.producer.getStats();
	}

	async setEncodingParameters(
		parameters: QosEncodingParameters
	): Promise<ProducerAdapterActionResult> {
		if (this.producer.closed) {
			return { applied: false, reason: 'producer-closed' };
		}

		try {
			await this.producer.setRtpEncodingParameters(
				mapEncodingParameters(parameters)
			);

			return { applied: true };
		} catch (error) {
			return { applied: false, reason: (error as Error).message };
		}
	}

	async setMaxSpatialLayer(
		spatialLayer: number
	): Promise<ProducerAdapterActionResult> {
		if (this.producer.closed) {
			return { applied: false, reason: 'producer-closed' };
		}

		if (this.producer.kind !== 'video') {
			return { applied: false, reason: 'not-video-producer' };
		}

		try {
			await this.producer.setMaxSpatialLayer(spatialLayer);

			return { applied: true };
		} catch (error) {
			return { applied: false, reason: (error as Error).message };
		}
	}

	async pauseUpstream(): Promise<ProducerAdapterActionResult> {
		if (this.producer.closed) {
			return { applied: false, reason: 'producer-closed' };
		}

		try {
			this.producer.pause();

			return { applied: true };
		} catch (error) {
			return { applied: false, reason: (error as Error).message };
		}
	}

	async resumeUpstream(): Promise<ProducerAdapterActionResult> {
		if (this.producer.closed) {
			return { applied: false, reason: 'producer-closed' };
		}

		try {
			this.producer.resume();

			return { applied: true };
		} catch (error) {
			return { applied: false, reason: (error as Error).message };
		}
	}
}
