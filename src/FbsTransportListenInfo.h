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

	auto portRange = FBS::Transport::CreatePortRange(builder, uint16_t(0), uint16_t(0));
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
