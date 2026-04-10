#include "GeoDbResolver.h"

#include <array>
#include <filesystem>
#include <string>
#include <unordered_set>
#include <system_error>
#include <unistd.h>

namespace mediasoup {

namespace {

namespace fs = std::filesystem;

bool isReadableFile(const fs::path& path) {
	if (path.empty()) return false;
	std::error_code ec;
	return fs::exists(path, ec) && fs::is_regular_file(path, ec);
}

void addCandidate(std::vector<fs::path>& ordered, std::unordered_set<std::string>& seen, const fs::path& path) {
	if (path.empty()) return;
	auto normalized = path.lexically_normal();
	auto key = normalized.generic_string();
	if (!seen.insert(key).second) return;
	ordered.push_back(normalized);
}

fs::path currentDefaultGeoDbPath() {
	return fs::path("./ip2region.xdb");
}

fs::path repoGeoDbPath() {
	return fs::path("./third_party/ip2region/ip2region.xdb");
}

fs::path executableDir() {
	std::array<char, 4096> buf{};
	ssize_t len = ::readlink("/proc/self/exe", buf.data(), buf.size() - 1);
	if (len <= 0) return {};
	buf[static_cast<size_t>(len)] = '\0';
	return fs::path(buf.data()).parent_path();
}

} // namespace

GeoDbResolution resolveGeoDbPath(const std::string& requestedPath, bool explicitPath) {
	std::vector<fs::path> ordered;
	std::unordered_set<std::string> seen;

	addCandidate(ordered, seen, fs::path(requestedPath));
	addCandidate(ordered, seen, currentDefaultGeoDbPath());
	addCandidate(ordered, seen, repoGeoDbPath());

	auto exeDir = executableDir();
	if (!exeDir.empty()) {
		addCandidate(ordered, seen, exeDir / "ip2region.xdb");
		addCandidate(ordered, seen, exeDir / ".." / "third_party" / "ip2region" / "ip2region.xdb");
	}

	GeoDbResolution resolution;
	resolution.usedFallback = false;
	for (const auto& candidate : ordered) {
		resolution.candidates.push_back(candidate.generic_string());
		if (resolution.resolvedPath.empty() && isReadableFile(candidate)) {
			resolution.resolvedPath = candidate.generic_string();
		}
	}

	if (!resolution.resolvedPath.empty() && (explicitPath || !requestedPath.empty())) {
		auto requested = fs::path(requestedPath).lexically_normal().generic_string();
		resolution.usedFallback = (resolution.resolvedPath != requested);
	}

	return resolution;
}

} // namespace mediasoup
