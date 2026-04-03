#pragma once
#include <string>
#include <random>
#include <sstream>
#include <iomanip>

namespace mediasoup {
namespace utils {

inline std::string generateUUIDv4() {
	static thread_local std::mt19937 gen(std::random_device{}());
	std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFF);

	uint32_t a = dist(gen), b = dist(gen), c = dist(gen), d = dist(gen);
	// Set version 4 and variant bits
	b = (b & 0xFFFF0FFF) | 0x00004000;
	c = (c & 0x3FFFFFFF) | 0x80000000;

	std::ostringstream ss;
	ss << std::hex << std::setfill('0')
	   << std::setw(8) << a << '-'
	   << std::setw(4) << (b >> 16) << '-'
	   << std::setw(4) << (b & 0xFFFF) << '-'
	   << std::setw(4) << (c >> 16) << '-'
	   << std::setw(4) << (c & 0xFFFF)
	   << std::setw(8) << d;
	return ss.str();
}

} // namespace utils
} // namespace mediasoup
