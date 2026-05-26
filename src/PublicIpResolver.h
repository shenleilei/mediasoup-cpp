#pragma once

#include <optional>
#include <string>

namespace mediasoup {

std::optional<std::string> ResolvePublicIpv4Address(const std::string& host);
bool IsPublicIpv4Address(const std::string& value);

} // namespace mediasoup
