#pragma once

#include <string>
#include <vector>

namespace mediasoup {

struct GeoDbResolution {
	std::string resolvedPath;
	std::vector<std::string> candidates;
	bool usedFallback = false;
};

GeoDbResolution resolveGeoDbPath(const std::string& requestedPath, bool explicitPath);

} // namespace mediasoup
