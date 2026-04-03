#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <optional>
#include <map>

namespace mediasoup {

using json = nlohmann::json;

struct RtcpFeedback {
	std::string type;
	std::string parameter;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RtcpFeedback, type, parameter)

struct RtpCodecCapability {
	std::string kind;
	std::string mimeType;
	uint32_t preferredPayloadType = 0;
	uint32_t clockRate = 0;
	uint8_t channels = 0;
	json parameters = json::object();
	std::vector<RtcpFeedback> rtcpFeedback;
};

inline void to_json(json& j, const RtpCodecCapability& c) {
	j = {{"mimeType", c.mimeType}, {"clockRate", c.clockRate}};
	if (c.preferredPayloadType) j["preferredPayloadType"] = c.preferredPayloadType;
	if (c.channels > 0) j["channels"] = c.channels;
	if (!c.parameters.empty()) j["parameters"] = c.parameters;
	if (!c.rtcpFeedback.empty()) j["rtcpFeedback"] = c.rtcpFeedback;
	if (!c.kind.empty()) j["kind"] = c.kind;
}

inline void from_json(const json& j, RtpCodecCapability& c) {
	j.at("mimeType").get_to(c.mimeType);
	j.at("clockRate").get_to(c.clockRate);
	if (j.contains("preferredPayloadType")) j.at("preferredPayloadType").get_to(c.preferredPayloadType);
	if (j.contains("channels")) j.at("channels").get_to(c.channels);
	if (j.contains("parameters")) c.parameters = j.at("parameters");
	if (j.contains("rtcpFeedback")) j.at("rtcpFeedback").get_to(c.rtcpFeedback);
	if (j.contains("kind")) j.at("kind").get_to(c.kind);
}

struct RtpHeaderExtension {
	std::string kind;
	std::string uri;
	uint8_t preferredId = 0;
	bool preferredEncrypt = false;
	std::string direction = "sendrecv";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RtpHeaderExtension, kind, uri, preferredId, preferredEncrypt, direction)

struct RtpCapabilities {
	std::vector<RtpCodecCapability> codecs;
	std::vector<RtpHeaderExtension> headerExtensions;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RtpCapabilities, codecs, headerExtensions)

struct RtpCodecParameters {
	std::string mimeType;
	uint8_t payloadType = 0;
	uint32_t clockRate = 0;
	uint8_t channels = 0;
	json parameters = json::object();
	std::vector<RtcpFeedback> rtcpFeedback;
};

inline void to_json(json& j, const RtpCodecParameters& c) {
	j = {{"mimeType", c.mimeType}, {"payloadType", c.payloadType}, {"clockRate", c.clockRate}};
	if (c.channels > 0) j["channels"] = c.channels;
	if (!c.parameters.empty()) j["parameters"] = c.parameters;
	if (!c.rtcpFeedback.empty()) j["rtcpFeedback"] = c.rtcpFeedback;
}

inline void from_json(const json& j, RtpCodecParameters& c) {
	j.at("mimeType").get_to(c.mimeType);
	j.at("payloadType").get_to(c.payloadType);
	j.at("clockRate").get_to(c.clockRate);
	if (j.contains("channels")) j.at("channels").get_to(c.channels);
	if (j.contains("parameters")) c.parameters = j.at("parameters");
	if (j.contains("rtcpFeedback")) j.at("rtcpFeedback").get_to(c.rtcpFeedback);
}

struct RtpHeaderExtensionParameters {
	std::string uri;
	uint8_t id = 0;
	bool encrypt = false;
	json parameters = json::object();
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RtpHeaderExtensionParameters, uri, id, encrypt, parameters)

struct RtpEncodingParameters {
	std::optional<uint32_t> ssrc;
	std::string rid;
	std::optional<uint8_t> codecPayloadType;
	std::optional<uint32_t> rtxSsrc;
	bool dtx = false;
	std::string scalabilityMode;
	std::optional<uint32_t> maxBitrate;
};

inline void to_json(json& j, const RtpEncodingParameters& e) {
	j = json::object();
	if (e.ssrc) j["ssrc"] = *e.ssrc;
	if (!e.rid.empty()) j["rid"] = e.rid;
	if (e.codecPayloadType) j["codecPayloadType"] = *e.codecPayloadType;
	if (e.rtxSsrc) j["rtx"] = {{"ssrc", *e.rtxSsrc}};
	if (e.dtx) j["dtx"] = e.dtx;
	if (!e.scalabilityMode.empty()) j["scalabilityMode"] = e.scalabilityMode;
	if (e.maxBitrate) j["maxBitrate"] = *e.maxBitrate;
}

inline void from_json(const json& j, RtpEncodingParameters& e) {
	if (j.contains("ssrc")) e.ssrc = j.at("ssrc").get<uint32_t>();
	if (j.contains("rid")) j.at("rid").get_to(e.rid);
	if (j.contains("codecPayloadType")) e.codecPayloadType = j.at("codecPayloadType").get<uint8_t>();
	if (j.contains("rtx") && j["rtx"].contains("ssrc")) e.rtxSsrc = j["rtx"]["ssrc"].get<uint32_t>();
	if (j.contains("dtx")) j.at("dtx").get_to(e.dtx);
	if (j.contains("scalabilityMode")) j.at("scalabilityMode").get_to(e.scalabilityMode);
	if (j.contains("maxBitrate")) e.maxBitrate = j.at("maxBitrate").get<uint32_t>();
}

struct RtcpParameters {
	std::string cname;
	bool reducedSize = true;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RtcpParameters, cname, reducedSize)

struct RtpParameters {
	std::string mid;
	std::vector<RtpCodecParameters> codecs;
	std::vector<RtpHeaderExtensionParameters> headerExtensions;
	std::vector<RtpEncodingParameters> encodings;
	RtcpParameters rtcp;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(RtpParameters, mid, codecs, headerExtensions, encodings, rtcp)

struct CodecMapping {
	uint8_t payloadType;
	uint8_t mappedPayloadType;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CodecMapping, payloadType, mappedPayloadType)

struct EncodingMapping {
	std::string rid;
	std::optional<uint32_t> ssrc;
	std::string scalabilityMode;
	uint32_t mappedSsrc = 0;
};

inline void to_json(json& j, const EncodingMapping& e) {
	j = json::object();
	if (!e.rid.empty()) j["rid"] = e.rid;
	if (e.ssrc) j["ssrc"] = *e.ssrc;
	if (!e.scalabilityMode.empty()) j["scalabilityMode"] = e.scalabilityMode;
	j["mappedSsrc"] = e.mappedSsrc;
}

inline void from_json(const json& j, EncodingMapping& e) {
	if (j.contains("rid")) j.at("rid").get_to(e.rid);
	if (j.contains("ssrc")) e.ssrc = j.at("ssrc").get<uint32_t>();
	if (j.contains("scalabilityMode")) j.at("scalabilityMode").get_to(e.scalabilityMode);
	j.at("mappedSsrc").get_to(e.mappedSsrc);
}

struct RtpMapping {
	std::vector<CodecMapping> codecs;
	std::vector<EncodingMapping> encodings;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RtpMapping, codecs, encodings)

} // namespace mediasoup
