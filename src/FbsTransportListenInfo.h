#pragma once

#include "transport_generated.h"

#include <flatbuffers/flatbuffers.h>
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace mediasoup::fbsutils {

inline flatbuffers::Offset<FBS::Transport::ListenInfo> BuildListenInfo(
	flatbuffers::FlatBufferBuilder& builder,
	const nlohmann::json& listenInfo,
	const std::string& defaultIp = "0.0.0.0",
	const std::string& defaultProtocol = "udp")
{
	const std::string ip = listenInfo.value("ip", defaultIp);
	const std::string announcedAddress = listenInfo.value("announcedAddress", "");
	const std::string protocol = listenInfo.value("protocol", defaultProtocol);
	const int portValue = listenInfo.value("port", 0);
	const uint16_t port = portValue > 0 && portValue <= 65535
		? static_cast<uint16_t>(portValue)
		: uint16_t(0);
	uint16_t portRangeMin = 0;
	uint16_t portRangeMax = 0;
	if (listenInfo.contains("portRange") && listenInfo["portRange"].is_object()) {
		const auto& portRangeJson = listenInfo["portRange"];
		const int minValue = portRangeJson.value("min", 0);
		const int maxValue = portRangeJson.value("max", 0);
		if (minValue > 0 && minValue <= 65535 && maxValue > 0 && maxValue <= 65535 && minValue <= maxValue) {
			portRangeMin = static_cast<uint16_t>(minValue);
			portRangeMax = static_cast<uint16_t>(maxValue);
		}
	}
	auto portRange = FBS::Transport::CreatePortRange(builder, portRangeMin, portRangeMax);
	auto flags = FBS::Transport::CreateSocketFlags(builder, false, false);

	return FBS::Transport::CreateListenInfo(
		builder,
		protocol == "tcp" ? FBS::Transport::Protocol::TCP : FBS::Transport::Protocol::UDP,
		builder.CreateString(ip),
		announcedAddress.empty() ? 0 : builder.CreateString(announcedAddress),
		port,
		portRange,
		flags,
		0,
		0);
}

inline std::vector<flatbuffers::Offset<FBS::Transport::ListenInfo>> BuildListenInfos(
	flatbuffers::FlatBufferBuilder& builder,
	const std::vector<nlohmann::json>& listenInfos,
	const std::string& defaultIp = "0.0.0.0",
	const std::string& defaultProtocol = "udp")
{
	std::vector<flatbuffers::Offset<FBS::Transport::ListenInfo>> result;
	result.reserve(listenInfos.size());
	for (const auto& listenInfo : listenInfos) {
		result.push_back(BuildListenInfo(builder, listenInfo, defaultIp, defaultProtocol));
	}
	return result;
}

} // namespace mediasoup::fbsutils
