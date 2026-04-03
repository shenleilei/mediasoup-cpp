#pragma once
#include "RtpTypes.h"

namespace mediasoup {

// mediasoup supported RTP capabilities (matches supportedRtpCapabilities.ts)
inline RtpCapabilities getSupportedRtpCapabilities() {
	RtpCapabilities caps;

	// Audio codecs
	caps.codecs.push_back({"audio", "audio/opus", 100, 48000, 2,
		{{"minptime", 10}, {"useinbandfec", 1}},
		{{"nack", ""}, {"transport-cc", ""}}});

	caps.codecs.push_back({"audio", "audio/PCMU", 0, 8000, 0, {}, {}});
	caps.codecs.push_back({"audio", "audio/PCMA", 8, 8000, 0, {}, {}});
	caps.codecs.push_back({"audio", "audio/ISAC", 0, 32000, 0, {}, {}});
	caps.codecs.push_back({"audio", "audio/ISAC", 0, 16000, 0, {}, {}});
	caps.codecs.push_back({"audio", "audio/G722", 9, 8000, 0, {}, {}});
	caps.codecs.push_back({"audio", "audio/multiopus", 0, 48000, 6,
		{{"channel_mapping", "0,4,1,2,3,5"}, {"num_streams", 4}, {"coupled_streams", 2}},
		{{"nack", ""}, {"transport-cc", ""}}});

	// Video codecs
	caps.codecs.push_back({"video", "video/VP8", 101, 90000, 0, {},
		{{"nack", ""}, {"nack", "pli"}, {"ccm", "fir"}, {"goog-remb", ""}, {"transport-cc", ""}}});
	caps.codecs.push_back({"video", "video/rtx", 102, 90000, 0, {{"apt", 101}}, {}});

	caps.codecs.push_back({"video", "video/VP9", 103, 90000, 0, {{"profile-id", 0}},
		{{"nack", ""}, {"nack", "pli"}, {"ccm", "fir"}, {"goog-remb", ""}, {"transport-cc", ""}}});
	caps.codecs.push_back({"video", "video/rtx", 104, 90000, 0, {{"apt", 103}}, {}});

	caps.codecs.push_back({"video", "video/VP9", 105, 90000, 0, {{"profile-id", 2}},
		{{"nack", ""}, {"nack", "pli"}, {"ccm", "fir"}, {"goog-remb", ""}, {"transport-cc", ""}}});
	caps.codecs.push_back({"video", "video/rtx", 106, 90000, 0, {{"apt", 105}}, {}});

	caps.codecs.push_back({"video", "video/H264", 107, 90000, 0,
		{{"level-asymmetry-allowed", 1}, {"packetization-mode", 1}, {"profile-level-id", "4d0032"}},
		{{"nack", ""}, {"nack", "pli"}, {"ccm", "fir"}, {"goog-remb", ""}, {"transport-cc", ""}}});
	caps.codecs.push_back({"video", "video/rtx", 108, 90000, 0, {{"apt", 107}}, {}});

	caps.codecs.push_back({"video", "video/H264", 109, 90000, 0,
		{{"level-asymmetry-allowed", 1}, {"packetization-mode", 0}, {"profile-level-id", "4d0032"}},
		{{"nack", ""}, {"nack", "pli"}, {"ccm", "fir"}, {"goog-remb", ""}, {"transport-cc", ""}}});
	caps.codecs.push_back({"video", "video/rtx", 110, 90000, 0, {{"apt", 109}}, {}});

	caps.codecs.push_back({"video", "video/H264", 127, 90000, 0,
		{{"level-asymmetry-allowed", 1}, {"packetization-mode", 1}, {"profile-level-id", "42e01f"}},
		{{"nack", ""}, {"nack", "pli"}, {"ccm", "fir"}, {"goog-remb", ""}, {"transport-cc", ""}}});
	caps.codecs.push_back({"video", "video/rtx", 125, 90000, 0, {{"apt", 127}}, {}});

	caps.codecs.push_back({"video", "video/H264", 108, 90000, 0,
		{{"level-asymmetry-allowed", 1}, {"packetization-mode", 0}, {"profile-level-id", "42e01f"}},
		{{"nack", ""}, {"nack", "pli"}, {"ccm", "fir"}, {"goog-remb", ""}, {"transport-cc", ""}}});
	caps.codecs.push_back({"video", "video/rtx", 124, 90000, 0, {{"apt", 108}}, {}});

	caps.codecs.push_back({"video", "video/H265", 111, 90000, 0,
		{{"level-id", 93}, {"profile-id", 1}, {"tier-flag", 0}},
		{{"nack", ""}, {"nack", "pli"}, {"ccm", "fir"}, {"goog-remb", ""}, {"transport-cc", ""}}});
	caps.codecs.push_back({"video", "video/rtx", 112, 90000, 0, {{"apt", 111}}, {}});

	caps.codecs.push_back({"video", "video/AV1", 113, 90000, 0,
		{{"level-idx", 5}, {"profile", 0}, {"tier", 0}},
		{{"nack", ""}, {"nack", "pli"}, {"ccm", "fir"}, {"transport-cc", ""}}});
	caps.codecs.push_back({"video", "video/rtx", 114, 90000, 0, {{"apt", 113}}, {}});

	// Header extensions
	caps.headerExtensions = {
		{"audio", "urn:ietf:params:rtp-hdrext:sdes:mid", 1, false, "sendrecv"},
		{"video", "urn:ietf:params:rtp-hdrext:sdes:mid", 1, false, "sendrecv"},
		{"video", "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id", 2, false, "recvonly"},
		{"video", "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id", 3, false, "recvonly"},
		{"audio", "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time", 4, false, "sendrecv"},
		{"video", "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time", 4, false, "sendrecv"},
		{"audio", "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01", 5, false, "recvonly"},
		{"video", "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01", 5, false, "recvonly"},
		{"audio", "urn:ietf:params:rtp-hdrext:ssrc-audio-level", 10, false, "sendrecv"},
		{"video", "urn:3gpp:video-orientation", 11, false, "sendrecv"},
		{"video", "urn:ietf:params:rtp-hdrext:toffset", 12, false, "sendrecv"},
		{"video", "http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time", 13, false, "sendrecv"},
		{"video", "http://www.webrtc.org/experiments/rtp-hdrext/playout-delay", 14, false, "sendrecv"},
		{"video", "https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension", 15, false, "sendrecv"},
	};

	return caps;
}

} // namespace mediasoup
