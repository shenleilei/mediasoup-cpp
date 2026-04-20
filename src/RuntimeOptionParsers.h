#pragma once

#include <string>

namespace mediasoup {

inline bool TryParseIntArgValue(const std::string& value, int& out)
{
	try {
		size_t pos = 0;
		const int parsed = std::stoi(value, &pos);
		if (pos != value.size()) {
			return false;
		}
		out = parsed;
		return true;
	} catch (...) {
		return false;
	}
}

inline bool TryParseDoubleArgValue(const std::string& value, double& out)
{
	try {
		size_t pos = 0;
		const double parsed = std::stod(value, &pos);
		if (pos != value.size()) {
			return false;
		}
		out = parsed;
		return true;
	} catch (...) {
		return false;
	}
}

} // namespace mediasoup
