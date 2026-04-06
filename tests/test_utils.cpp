#include <gtest/gtest.h>
#include "Utils.h"
#include <regex>
#include <set>

using namespace mediasoup;

TEST(UtilsTest, GenerateUUIDv4HasValidFormatAndBits) {
	const auto uuid = utils::generateUUIDv4();
	const std::regex pattern(
		"^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$");
	EXPECT_TRUE(std::regex_match(uuid, pattern)) << uuid;
}

TEST(UtilsTest, GenerateUUIDv4ProducesUniqueValuesInSample) {
	std::set<std::string> values;
	for (int i = 0; i < 200; ++i) {
		values.insert(utils::generateUUIDv4());
	}
	EXPECT_EQ(values.size(), 200u);
}
