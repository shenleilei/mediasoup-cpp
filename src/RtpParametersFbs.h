#pragma once

#include "RtpTypes.h"
#include "rtpParameters_generated.h"
#include <flatbuffers/flatbuffers.h>

namespace mediasoup {

inline flatbuffers::Offset<FBS::RtpParameters::RtpParameters> BuildFbsRtpParameters(
	flatbuffers::FlatBufferBuilder& builder,
	const RtpParameters& params)
{
	std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpCodecParameters>> fbCodecs;
	for (const auto& codec : params.codecs) {
		std::vector<flatbuffers::Offset<FBS::RtpParameters::Parameter>> fbParams;
		for (const auto& [key, value] : codec.parameters.items()) {
			flatbuffers::Offset<void> valueOffset;
			FBS::RtpParameters::Value valueType;

			if (value.is_number_integer()) {
				valueOffset = FBS::RtpParameters::CreateInteger32(
					builder,
					value.get<int32_t>()).Union();
				valueType = FBS::RtpParameters::Value::Integer32;
			} else if (value.is_number_float()) {
				valueOffset = FBS::RtpParameters::CreateDouble(
					builder,
					value.get<double>()).Union();
				valueType = FBS::RtpParameters::Value::Double;
			} else if (value.is_string()) {
				valueOffset = FBS::RtpParameters::CreateString(
					builder,
					builder.CreateString(value.get<std::string>())).Union();
				valueType = FBS::RtpParameters::Value::String;
			} else {
				valueOffset = FBS::RtpParameters::CreateBoolean(
					builder,
					value.is_boolean() ? (value.get<bool>() ? 1 : 0) : 0).Union();
				valueType = FBS::RtpParameters::Value::Boolean;
			}

			fbParams.push_back(FBS::RtpParameters::CreateParameter(
				builder,
				builder.CreateString(key),
				valueType,
				valueOffset));
		}

		std::vector<flatbuffers::Offset<FBS::RtpParameters::RtcpFeedback>> fbFeedback;
		for (const auto& feedback : codec.rtcpFeedback) {
			fbFeedback.push_back(FBS::RtpParameters::CreateRtcpFeedback(
				builder,
				builder.CreateString(feedback.type),
				feedback.parameter.empty() ? 0 : builder.CreateString(feedback.parameter)));
		}

		fbCodecs.push_back(FBS::RtpParameters::CreateRtpCodecParameters(
			builder,
			builder.CreateString(codec.mimeType),
			codec.payloadType,
			codec.clockRate,
			codec.channels > 0 ? codec.channels : flatbuffers::Optional<uint8_t>(),
			builder.CreateVector(fbParams),
			builder.CreateVector(fbFeedback)));
	}

	std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpHeaderExtensionParameters>> fbExtensions;
	for (const auto& extension : params.headerExtensions) {
		auto uri = FBS::RtpParameters::RtpHeaderExtensionUri::Mid;
		if (extension.uri.find("abs-send-time") != std::string::npos) {
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::AbsSendTime;
		} else if (extension.uri.find("transport-wide-cc") != std::string::npos) {
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::TransportWideCcDraft01;
		} else if (extension.uri.find("ssrc-audio-level") != std::string::npos) {
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::AudioLevel;
		} else if (extension.uri.find("video-orientation") != std::string::npos) {
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::VideoOrientation;
		} else if (extension.uri.find("rtp-stream-id") != std::string::npos) {
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::RtpStreamId;
		} else if (extension.uri.find("repaired-rtp-stream-id") != std::string::npos) {
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::RepairRtpStreamId;
		} else if (extension.uri.find("sdes:mid") != std::string::npos) {
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::Mid;
		} else if (extension.uri.find("toffset") != std::string::npos) {
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::TimeOffset;
		} else if (extension.uri.find("abs-capture-time") != std::string::npos) {
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::AbsCaptureTime;
		} else if (extension.uri.find("playout-delay") != std::string::npos) {
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::PlayoutDelay;
		} else if (extension.uri.find("dependency-descriptor") != std::string::npos) {
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::Mid;
		}

		fbExtensions.push_back(FBS::RtpParameters::CreateRtpHeaderExtensionParameters(
			builder,
			uri,
			extension.id,
			extension.encrypt,
			0));
	}

	std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters>> fbEncodings;
	for (const auto& encoding : params.encodings) {
		flatbuffers::Offset<FBS::RtpParameters::Rtx> rtxOffset = 0;
		if (encoding.rtxSsrc) {
			rtxOffset = FBS::RtpParameters::CreateRtx(builder, *encoding.rtxSsrc);
		}

		fbEncodings.push_back(FBS::RtpParameters::CreateRtpEncodingParameters(
			builder,
			encoding.ssrc ? flatbuffers::Optional<uint32_t>(*encoding.ssrc) : flatbuffers::Optional<uint32_t>(),
			encoding.rid.empty() ? 0 : builder.CreateString(encoding.rid),
			encoding.codecPayloadType ? flatbuffers::Optional<uint8_t>(*encoding.codecPayloadType) : flatbuffers::Optional<uint8_t>(),
			rtxOffset,
			encoding.dtx,
			encoding.scalabilityMode.empty() ? 0 : builder.CreateString(encoding.scalabilityMode),
			encoding.maxBitrate ? flatbuffers::Optional<uint32_t>(*encoding.maxBitrate) : flatbuffers::Optional<uint32_t>()));
	}

	auto rtcpOffset = FBS::RtpParameters::CreateRtcpParameters(
		builder,
		params.rtcp.cname.empty() ? 0 : builder.CreateString(params.rtcp.cname),
		params.rtcp.reducedSize);

	return FBS::RtpParameters::CreateRtpParameters(
		builder,
		params.mid.empty() ? 0 : builder.CreateString(params.mid),
		builder.CreateVector(fbCodecs),
		builder.CreateVector(fbExtensions),
		builder.CreateVector(fbEncodings),
		rtcpOffset);
}

inline flatbuffers::Offset<FBS::RtpParameters::RtpMapping> BuildFbsRtpMapping(
	flatbuffers::FlatBufferBuilder& builder,
	const RtpMapping& mapping)
{
	std::vector<flatbuffers::Offset<FBS::RtpParameters::CodecMapping>> fbCodecs;
	for (const auto& codec : mapping.codecs) {
		fbCodecs.push_back(FBS::RtpParameters::CreateCodecMapping(
			builder,
			codec.payloadType,
			codec.mappedPayloadType));
	}

	std::vector<flatbuffers::Offset<FBS::RtpParameters::EncodingMapping>> fbEncodings;
	for (const auto& encoding : mapping.encodings) {
		fbEncodings.push_back(FBS::RtpParameters::CreateEncodingMapping(
			builder,
			encoding.rid.empty() ? 0 : builder.CreateString(encoding.rid),
			encoding.ssrc ? flatbuffers::Optional<uint32_t>(*encoding.ssrc) : flatbuffers::Optional<uint32_t>(),
			encoding.scalabilityMode.empty() ? 0 : builder.CreateString(encoding.scalabilityMode),
			encoding.mappedSsrc));
	}

	return FBS::RtpParameters::CreateRtpMapping(
		builder,
		builder.CreateVector(fbCodecs),
		builder.CreateVector(fbEncodings));
}

} // namespace mediasoup
