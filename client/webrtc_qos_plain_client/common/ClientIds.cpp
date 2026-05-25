#include "common/ClientIds.h"

#include <algorithm>

namespace webrtc_qos_plain {

uint32_t StableId(const std::string& value, uint32_t salt)
{
	uint32_t hash = salt ? salt : 2166136261u;
	for (unsigned char c : value) {
		hash ^= c;
		hash *= 16777619u;
	}
	return hash == 0 ? 1u : hash;
}

webrtc_qos::SessionConfig MakeSingleVideoSessionConfig(const SingleVideoSessionParams& params)
{
	VideoTrackSessionParams track;
	track.trackId = params.trackId;
	track.senderSsrc = params.senderSsrc;
	track.payloadType = params.payloadType;
	track.transportCcExtId = params.transportCcExtId;
	track.weight = 100;
	track.baseTrack = true;

	VideoSessionParams multi;
	multi.roomId = params.roomId;
	multi.transportId = params.transportId;
	multi.sourceId = params.sourceId;
	multi.receiverId = params.receiverId;
	multi.receiverIdOverride = params.receiverIdOverride;
	multi.startBitrateBps = params.startBitrateBps;
	multi.minBitrateBps = params.minBitrateBps;
	multi.maxBitrateBps = params.maxBitrateBps;
	multi.debugName = params.debugName;
	multi.tracks.push_back(track);
	return MakeVideoSessionConfig(multi);
}

webrtc_qos::SessionConfig MakeVideoSessionConfig(const VideoSessionParams& params)
{
	webrtc_qos::SessionConfig session;
	session.ids.session_id = StableId(params.roomId, 0x811c9dc5u);
	session.ids.stream_id = StableId(params.roomId + ":" + params.sourceId, 0x9e3779b9u);
	session.ids.transport_id = StableId(params.transportId, 0x85ebca6bu);
	session.ids.source_id = StableId(params.sourceId, 0xc2b2ae35u);
	session.ids.receiver_id = params.receiverIdOverride != 0
		? params.receiverIdOverride
		: (params.receiverId.empty() ? 0u : StableId(params.receiverId, 0x27d4eb2fu));
	session.start_bitrate_bps = params.startBitrateBps;
	session.min_bitrate_bps = params.minBitrateBps;
	session.max_bitrate_bps = params.maxBitrateBps;
	session.debug_name = params.debugName;
	session.h264.profile_level_id = 0x42e01fu;
	session.h264.packetization_mode_1 = true;
	session.h264.max_rtp_payload_bytes = 1200;
	session.rtcp.sr_rr_interval_ms = 1000;

	for (size_t index = 0; index < params.tracks.size(); ++index) {
		const auto& input = params.tracks[index];
		webrtc_qos::VideoTrackConfig track;
		track.ids = session.ids;
		track.ids.track_id = input.trackId != 0
			? input.trackId
			: StableId(params.roomId + ":" + params.sourceId + ":" + input.trackIdString + ":" + std::to_string(index + 1), 0x165667b1u);
		track.ids.sender_ssrc = input.senderSsrc;
		track.h264 = session.h264;
		track.h264.payload_type = input.payloadType;
		track.weight = input.weight == 0 ? 100 : input.weight;
		track.enabled = true;
		track.base_track = input.baseTrack || index == 0;
		session.video_tracks.push_back(track);
		if (index == 0) {
			session.ids.sender_ssrc = input.senderSsrc;
			session.ids.track_id = track.ids.track_id;
			session.h264.payload_type = input.payloadType;
			session.twcc.extension_id = input.transportCcExtId;
		}
	}
	return session;
}

} // namespace webrtc_qos_plain
