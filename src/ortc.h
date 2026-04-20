#pragma once
#include "RtpTypes.h"
#include "supportedRtpCapabilities.h"
#include <array>
#include <stdexcept>
#include <algorithm>
#include <random>

namespace mediasoup {
namespace ortc {

// Dynamic payload types pool
inline constexpr std::array<uint8_t, 32> DynamicPayloadTypes = {
	100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,
	116,117,118,119,120,121,122,123,124,125,126,127,96,97,98,99
};

inline std::string toLower(const std::string& s) {
	std::string r = s;
	std::transform(r.begin(), r.end(), r.begin(), ::tolower);
	return r;
}

inline bool isRtxCodec(const RtpCodecCapability& codec) {
	return toLower(codec.mimeType).find("/rtx") != std::string::npos;
}

inline bool isRtxCodec(const RtpCodecParameters& codec) {
	return toLower(codec.mimeType).find("/rtx") != std::string::npos;
}

// Check if H264 profiles match
inline bool h264ProfilesMatch(const json& aParams, const json& bParams) {
	std::string aProfile = "4d0032";
	std::string bProfile = "4d0032";
	if (aParams.contains("profile-level-id"))
		aProfile = aParams["profile-level-id"].get<std::string>();
	if (bParams.contains("profile-level-id"))
		bProfile = bParams["profile-level-id"].get<std::string>();

	// Compare first 4 chars (profile_idc + profile_iop) for profile matching
	if (aProfile.size() >= 4 && bProfile.size() >= 4) {
		return toLower(aProfile.substr(0, 4)) == toLower(bProfile.substr(0, 4));
	}
	return aProfile == bProfile;
}

struct MatchCodecsOptions {
	bool strict = false;
	bool modify = false;
};

inline bool matchCodecs(
	const RtpCodecCapability& aCodec,
	const RtpCodecCapability& bCodec,
	MatchCodecsOptions opts = {})
{
	if (toLower(aCodec.mimeType) != toLower(bCodec.mimeType)) return false;
	if (aCodec.clockRate != bCodec.clockRate) return false;
	if (aCodec.channels > 0 && bCodec.channels > 0 && aCodec.channels != bCodec.channels) return false;

	// Per-codec matching
	std::string mime = toLower(aCodec.mimeType);

	if (mime == "video/h264") {
		if (opts.strict) {
			int aPm = aCodec.parameters.value("packetization-mode", 0);
			int bPm = bCodec.parameters.value("packetization-mode", 0);
			if (aPm != bPm) return false;
		}
		if (!h264ProfilesMatch(aCodec.parameters, bCodec.parameters)) return false;
	} else if (mime == "video/vp9") {
		if (opts.strict) {
			int aPid = aCodec.parameters.value("profile-id", 0);
			int bPid = bCodec.parameters.value("profile-id", 0);
			if (aPid != bPid) return false;
		}
	} else if (mime == "video/h265") {
		if (opts.strict) {
			int aPid = aCodec.parameters.value("profile-id", 1);
			int bPid = bCodec.parameters.value("profile-id", 1);
			if (aPid != bPid) return false;
		}
	}

	return true;
}

inline bool matchCodecs(
	const RtpCodecParameters& aCodec,
	const RtpCodecCapability& bCodec,
	MatchCodecsOptions opts = {})
{
	RtpCodecCapability a;
	a.mimeType = aCodec.mimeType;
	a.clockRate = aCodec.clockRate;
	a.channels = aCodec.channels;
	a.parameters = aCodec.parameters;
	return matchCodecs(a, bCodec, opts);
}

inline RtpCapabilities generateRouterRtpCapabilities(
	const std::vector<RtpCodecCapability>& mediaCodecs)
{
	auto supported = getSupportedRtpCapabilities();
	std::vector<uint8_t> dynamicPTs(DynamicPayloadTypes.begin(), DynamicPayloadTypes.end());

	RtpCapabilities caps;
	caps.headerExtensions = supported.headerExtensions;

	for (auto& mediaCodec : mediaCodecs) {
		if (isRtxCodec(mediaCodec)) throw std::runtime_error("media codecs must not include RTX");

		auto it = std::find_if(supported.codecs.begin(), supported.codecs.end(),
			[&](const RtpCodecCapability& sc) {
				return matchCodecs(mediaCodec, sc, {false, false});
			});

		if (it == supported.codecs.end())
			throw std::runtime_error("unsupported codec: " + mediaCodec.mimeType);

		auto codec = *it;
		// Use preferred PT from mediaCodec if set, otherwise from supported
		if (mediaCodec.preferredPayloadType > 0) {
			codec.preferredPayloadType = mediaCodec.preferredPayloadType;
		}
		// Merge parameters from mediaCodec
		for (auto& [k, v] : mediaCodec.parameters.items()) {
			codec.parameters[k] = v;
		}

		// Assign dynamic PT if needed
		if (codec.preferredPayloadType == 0 ||
			std::find(dynamicPTs.begin(), dynamicPTs.end(), codec.preferredPayloadType) == dynamicPTs.end()) {
			// PT is 0 or already taken, assign a new one
			if (dynamicPTs.empty()) throw std::runtime_error("no dynamic PTs left");
			codec.preferredPayloadType = dynamicPTs.front();
			dynamicPTs.erase(dynamicPTs.begin());
		} else {
			dynamicPTs.erase(std::remove(dynamicPTs.begin(), dynamicPTs.end(),
				codec.preferredPayloadType), dynamicPTs.end());
		}

		caps.codecs.push_back(codec);

		// Add RTX codec
		if (!isRtxCodec(codec)) {
			if (dynamicPTs.empty()) throw std::runtime_error("no dynamic PTs left for RTX");
			uint8_t rtxPt = dynamicPTs.front();
			dynamicPTs.erase(dynamicPTs.begin());

			RtpCodecCapability rtxCodec;
			rtxCodec.kind = codec.kind;
			rtxCodec.mimeType = codec.kind + "/rtx";
			rtxCodec.preferredPayloadType = rtxPt;
			rtxCodec.clockRate = codec.clockRate;
			rtxCodec.parameters = {{"apt", codec.preferredPayloadType}};
			caps.codecs.push_back(rtxCodec);
		}
	}

	return caps;
}

inline RtpMapping getProducerRtpParametersMapping(
	const RtpParameters& params,
	const RtpCapabilities& caps)
{
	RtpMapping mapping;

	// Map codecs
	std::map<uint8_t, uint8_t> codecToCapPt; // producer PT -> cap PT
	for (auto& codec : params.codecs) {
		if (isRtxCodec(codec)) continue;

		auto it = std::find_if(caps.codecs.begin(), caps.codecs.end(),
			[&](const RtpCodecCapability& capCodec) {
				return !isRtxCodec(capCodec) && matchCodecs(codec, capCodec, {true, false});
			});

		if (it == caps.codecs.end())
			throw std::runtime_error("unsupported codec: " + codec.mimeType);

		codecToCapPt[codec.payloadType] = static_cast<uint8_t>(it->preferredPayloadType);
		mapping.codecs.push_back({codec.payloadType, static_cast<uint8_t>(it->preferredPayloadType)});
	}

	// Map RTX codecs
	for (auto& codec : params.codecs) {
		if (!isRtxCodec(codec)) continue;
		uint8_t apt = codec.parameters.value("apt", 0);
		auto capIt = codecToCapPt.find(apt);
		if (capIt == codecToCapPt.end()) continue;

		// Find RTX in caps for the mapped codec
		for (auto& capCodec : caps.codecs) {
			if (isRtxCodec(capCodec) && capCodec.parameters.value("apt", 0) == capIt->second) {
				mapping.codecs.push_back({codec.payloadType, static_cast<uint8_t>(capCodec.preferredPayloadType)});
				break;
			}
		}
	}

	// Map encodings
	static thread_local std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<uint32_t> dist(100000000, 999999999);

	for (auto& encoding : params.encodings) {
		EncodingMapping em;
		em.rid = encoding.rid;
		em.ssrc = encoding.ssrc;
		em.scalabilityMode = encoding.scalabilityMode;
		em.mappedSsrc = dist(rng);
		mapping.encodings.push_back(em);
	}

	return mapping;
}

inline RtpParameters getConsumableRtpParameters(
	const std::string& kind,
	const RtpParameters& params,
	const RtpCapabilities& caps,
	const RtpMapping& rtpMapping)
{
	RtpParameters consumable;

	// Map codecs
	for (auto& cm : rtpMapping.codecs) {
		auto it = std::find_if(caps.codecs.begin(), caps.codecs.end(),
			[&](const RtpCodecCapability& c) { return c.preferredPayloadType == cm.mappedPayloadType; });
		if (it == caps.codecs.end()) continue;

		RtpCodecParameters codec;
		codec.mimeType = it->mimeType;
		codec.payloadType = it->preferredPayloadType;
		codec.clockRate = it->clockRate;
		codec.channels = it->channels;
		codec.parameters = it->parameters;
		codec.rtcpFeedback = it->rtcpFeedback;
		consumable.codecs.push_back(codec);
	}

	// Map encodings
	for (size_t i = 0; i < params.encodings.size(); i++) {
		auto& encoding = params.encodings[i];
		RtpEncodingParameters ce;
		ce.ssrc = (i < rtpMapping.encodings.size()) ? std::optional<uint32_t>(rtpMapping.encodings[i].mappedSsrc) : std::nullopt;
		ce.scalabilityMode = encoding.scalabilityMode.empty() ? "S1T1" : encoding.scalabilityMode;
		if (encoding.maxBitrate) ce.maxBitrate = encoding.maxBitrate;
		consumable.encodings.push_back(ce);
	}

	consumable.rtcp.cname = params.rtcp.cname;
	consumable.rtcp.reducedSize = true;

	return consumable;
}

inline RtpParameters getConsumerRtpParameters(
	const RtpParameters& consumableParams,
	const RtpCapabilities& remoteRtpCapabilities,
	bool pipe = false)
{
	RtpParameters consumer;

	// For pipe consumers, use consumable params directly
	if (pipe) {
		consumer = consumableParams;
		// Assign unique SSRC for each encoding
		static thread_local std::mt19937 rng(std::random_device{}());
		std::uniform_int_distribution<uint32_t> dist(100000000, 999999999);
		for (auto& enc : consumer.encodings) {
			enc.ssrc = dist(rng);
			enc.rtxSsrc = dist(rng);
		}
		return consumer;
	}

	// Match codecs with remote capabilities.
	for (auto& codec : consumableParams.codecs) {
		auto it = std::find_if(remoteRtpCapabilities.codecs.begin(), remoteRtpCapabilities.codecs.end(),
			[&](const RtpCodecCapability& remoteCodec) {
				return matchCodecs(codec, remoteCodec, {true, false});
			});

		if (it == remoteRtpCapabilities.codecs.end())
			continue;

		auto matchedCodec = codec;
		std::vector<RtcpFeedback> matchedFeedback;
		for (const auto& fb : codec.rtcpFeedback) {
			auto fbIt = std::find_if(it->rtcpFeedback.begin(), it->rtcpFeedback.end(),
				[&](const RtcpFeedback& remoteFb) {
					return remoteFb.type == fb.type && remoteFb.parameter == fb.parameter;
				});
			if (fbIt != it->rtcpFeedback.end())
				matchedFeedback.push_back(fb);
		}
		matchedCodec.rtcpFeedback = std::move(matchedFeedback);
		consumer.codecs.push_back(std::move(matchedCodec));
	}

	// Remove RTX codecs that do not have an associated matched media codec.
	bool hasRtx = false;
	for (auto it = consumer.codecs.begin(); it != consumer.codecs.end(); ) {
		if (!isRtxCodec(*it)) {
			++it;
			continue;
		}

		uint8_t apt = it->parameters.value("apt", 0);
		auto mediaIt = std::find_if(consumer.codecs.begin(), consumer.codecs.end(),
			[apt](const RtpCodecParameters& codec) {
				return !isRtxCodec(codec) && codec.payloadType == apt;
			});
		if (mediaIt == consumer.codecs.end()) {
			it = consumer.codecs.erase(it);
		} else {
			hasRtx = true;
			++it;
		}
	}

	if (consumer.codecs.empty() || isRtxCodec(consumer.codecs.front()))
		throw std::runtime_error("no compatible codecs");

	// Header extensions - match by URI and negotiated id.
	for (auto& ext : consumableParams.headerExtensions) {
		for (auto& remoteExt : remoteRtpCapabilities.headerExtensions) {
			if (ext.uri == remoteExt.uri && ext.id == remoteExt.preferredId) {
				consumer.headerExtensions.push_back(ext);
				break;
			}
		}
	}

	// Single encoding with new SSRC
	static thread_local std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<uint32_t> dist(100000000, 999999999);

	RtpEncodingParameters enc;
	enc.ssrc = dist(rng);

	if (hasRtx) enc.rtxSsrc = dist(rng);

	consumer.encodings.push_back(enc);
	consumer.rtcp = consumableParams.rtcp;
	consumer.rtcp.reducedSize = true;

	return consumer;
}

inline RtpParameters getPipeConsumerRtpParameters(
	const RtpParameters& consumableParams)
{
	return getConsumerRtpParameters(consumableParams, {}, true);
}

} // namespace ortc
} // namespace mediasoup
