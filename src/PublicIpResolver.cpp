#include "PublicIpResolver.h"

#include <arpa/inet.h>
#include <netdb.h>

namespace mediasoup {
namespace {

bool IsPrivateOrReservedIpv4(const std::string& value)
{
	in_addr addr{};
	if (::inet_pton(AF_INET, value.c_str(), &addr) != 1) {
		return true;
	}
	const uint32_t ip = ntohl(addr.s_addr);
	const uint8_t a = static_cast<uint8_t>((ip >> 24) & 0xff);
	const uint8_t b = static_cast<uint8_t>((ip >> 16) & 0xff);

	if (a == 10) return true;
	if (a == 127) return true;
	if (a == 169 && b == 254) return true;
	if (a == 192 && b == 168) return true;
	if (a == 172 && b >= 16 && b <= 31) return true;
	if (a == 100 && b >= 64 && b <= 127) return true;
	if (a >= 224) return true;
	return false;
}

} // namespace

bool IsPublicIpv4Address(const std::string& value)
{
	in_addr addr{};
	if (::inet_pton(AF_INET, value.c_str(), &addr) != 1) {
		return false;
	}
	return !IsPrivateOrReservedIpv4(value);
}

std::optional<std::string> ResolvePublicIpv4Address(const std::string& host)
{
	if (host.empty()) return std::nullopt;
	if (IsPublicIpv4Address(host)) return host;

	addrinfo hints{};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_ADDRCONFIG;

	addrinfo* result = nullptr;
	if (::getaddrinfo(host.c_str(), nullptr, &hints, &result) != 0) {
		return std::nullopt;
	}

	for (addrinfo* ai = result; ai; ai = ai->ai_next) {
		auto* addr = reinterpret_cast<sockaddr_in*>(ai->ai_addr);
		char buf[INET_ADDRSTRLEN]{};
		if (::inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof(buf)) == nullptr) {
			continue;
		}
		const std::string candidate = buf;
		if (IsPublicIpv4Address(candidate)) {
			::freeaddrinfo(result);
			return candidate;
		}
	}

	::freeaddrinfo(result);
	return std::nullopt;
}

} // namespace mediasoup
