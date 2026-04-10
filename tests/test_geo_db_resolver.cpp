#include <gtest/gtest.h>

#include "GeoDbResolver.h"

#include <filesystem>

using namespace mediasoup;

namespace fs = std::filesystem;

namespace {

std::string normalize(const std::string& path) {
	return fs::path(path).lexically_normal().generic_string();
}

} // namespace

TEST(GeoDbResolverTest, PrefersExplicitExistingPath) {
	auto resolution = resolveGeoDbPath("./third_party/ip2region/ip2region.xdb", true);
	ASSERT_FALSE(resolution.resolvedPath.empty());
	EXPECT_EQ(normalize(resolution.resolvedPath),
		normalize("./third_party/ip2region/ip2region.xdb"));
	EXPECT_FALSE(resolution.usedFallback);
}

TEST(GeoDbResolverTest, FallsBackToKnownDefaultLocations) {
	auto resolution = resolveGeoDbPath("./missing-ip2region-for-test.xdb", true);
	ASSERT_FALSE(resolution.resolvedPath.empty());

	std::string expected;
	if (fs::exists("./ip2region.xdb")) {
		expected = "./ip2region.xdb";
	} else {
		expected = "./third_party/ip2region/ip2region.xdb";
	}

	EXPECT_EQ(normalize(resolution.resolvedPath), normalize(expected));
	EXPECT_TRUE(resolution.usedFallback);
}
