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
	webrtc_qos::SessionConfig session;
	session.ids.session_id = StableId(params.roomId, 0x811c9dc5u);
	session.ids.stream_id = StableId(params.roomId + ":" + params.sourceId, 0x9e3779b9u);
	session.ids.transport_id = StableId(params.transportId, 0x85ebca6bu);
	session.ids.source_id = StableId(params.sourceId, 0xc2b2ae35u);
	session.ids.receiver_id = params.receiverIdOverride != 0
		? params.receiverIdOverride
		: (params.receiverId.empty() ? 0u : StableId(params.receiverId, 0x27d4eb2fu));
	session.ids.sender_ssrc = params.senderSsrc;
	session.ids.track_id = params.trackId;
	session.start_bitrate_bps = params.startBitrateBps;
	session.min_bitrate_bps = params.minBitrateBps;
	session.max_bitrate_bps = params.maxBitrateBps;
	session.debug_name = params.debugName;
	session.h264.payload_type = params.payloadType;
	session.h264.profile_level_id = 0x42e01fu;
	session.h264.packetization_mode_1 = true;
	session.h264.max_rtp_payload_bytes = 1200;
	session.twcc.extension_id = params.transportCcExtId;
	session.rtcp.sr_rr_interval_ms = 1000;

	webrtc_qos::VideoTrackConfig track;
	track.ids = session.ids;
	track.ids.track_id = params.trackId;
	track.ids.sender_ssrc = params.senderSsrc;
	track.h264 = session.h264;
	track.weight = 100;
	track.enabled = true;
	track.base_track = true;
	session.video_tracks.push_back(track);
	return session;
}

} // namespace webrtc_qos_plain
