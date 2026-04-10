import type {
	PeerQosDecision,
	PeerQosTrackState,
	QosQuality,
	QosSource,
} from './types';

const qualityRank: Record<QosQuality, number> = {
	excellent: 3,
	good: 2,
	poor: 1,
	lost: 0,
};

function sourcePriority(source: QosSource): number {
	switch (source) {
		case 'audio': {
			return 0;
		}

		case 'screenShare': {
			return 1;
		}

		case 'camera': {
			return 2;
		}
	}
}

function minQuality(a: QosQuality, b: QosQuality): QosQuality {
	return qualityRank[a] <= qualityRank[b] ? a : b;
}

function prioritizeTracks(tracks: PeerQosTrackState[]): PeerQosTrackState[] {
	return [...tracks].sort((left, right) => {
		const leftPriority = sourcePriority(left.source);
		const rightPriority = sourcePriority(right.source);

		if (leftPriority !== rightPriority) {
			return leftPriority - rightPriority;
		}

		return left.trackId.localeCompare(right.trackId);
	});
}

export function buildPeerQosDecision(
	tracks: PeerQosTrackState[]
): PeerQosDecision {
	if (tracks.length === 0) {
		return {
			peerQuality: 'lost',
			prioritizedTrackIds: [],
			sacrificialTrackIds: [],
			keepAudioAlive: false,
			preferScreenShare: false,
			allowVideoRecovery: false,
		};
	}

	const prioritizedTracks = prioritizeTracks(tracks);
	const hasAudio = prioritizedTracks.some(track => track.source === 'audio');
	const hasScreenShare = prioritizedTracks.some(
		track => track.source === 'screenShare'
	);

	let peerQuality: QosQuality = prioritizedTracks[0].quality;

	for (const track of prioritizedTracks.slice(1)) {
		peerQuality = minQuality(peerQuality, track.quality);
	}

	const sacrificialTracks = prioritizedTracks
		.filter(track => {
			if (track.source === 'audio') {
				return false;
			}

			if (hasScreenShare) {
				return track.source === 'camera';
			}

			return hasAudio || track.source === 'camera';
		})
		.sort(
			(left, right) =>
				sourcePriority(right.source) - sourcePriority(left.source)
		);

	return {
		peerQuality,
		prioritizedTrackIds: prioritizedTracks.map(track => track.trackId),
		sacrificialTrackIds: sacrificialTracks.map(track => track.trackId),
		keepAudioAlive: hasAudio,
		preferScreenShare: hasScreenShare,
		allowVideoRecovery:
			peerQuality !== 'poor' &&
			peerQuality !== 'lost' &&
			!prioritizedTracks.some(
				track => track.source === 'audio' && track.quality === 'poor'
			),
	};
}

export class PeerQosCoordinator {
	private readonly tracks: Map<string, PeerQosTrackState> = new Map();

	upsertTrack(track: PeerQosTrackState): void {
		this.tracks.set(track.trackId, track);
	}

	removeTrack(trackId: string): void {
		this.tracks.delete(trackId);
	}

	clear(): void {
		this.tracks.clear();
	}

	getTracks(): PeerQosTrackState[] {
		return Array.from(this.tracks.values());
	}

	getDecision(): PeerQosDecision {
		return buildPeerQosDecision(this.getTracks());
	}
}
