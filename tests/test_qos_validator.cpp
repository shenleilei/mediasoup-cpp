#include <gtest/gtest.h>
#include "qos/QosValidator.h"
#include <fstream>

using namespace mediasoup;
using json = nlohmann::json;

namespace {

json LoadFixture(const std::string& name) {
	std::ifstream f("tests/fixtures/qos_protocol/" + name + ".json");
	EXPECT_TRUE(f.is_open()) << "fixture not found: " << name;
	json data;
	f >> data;
	return data;
}

} // namespace

TEST(QosValidatorTest, RejectsMissingSchema) {
	auto fixture = LoadFixture("invalid_missing_schema");
	auto parsed = qos::QosValidator::ParseClientSnapshot(fixture);

	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("schema"), std::string::npos);
}

TEST(QosValidatorTest, RejectsBadSeqType) {
	auto fixture = LoadFixture("invalid_bad_seq");
	auto parsed = qos::QosValidator::ParseClientSnapshot(fixture);

	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("seq"), std::string::npos);
}

TEST(QosValidatorTest, RejectsInvalidTrackKind) {
	auto fixture = LoadFixture("valid_client_v1");
	fixture["tracks"][0]["kind"] = "data";

	auto parsed = qos::QosValidator::ParseClientSnapshot(fixture);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("kind"), std::string::npos);
}

TEST(QosValidatorTest, RejectsInvalidOverrideScope) {
	auto fixture = LoadFixture("valid_override_v1");
	fixture["scope"] = "room";

	auto parsed = qos::QosValidator::ParseOverride(fixture);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("scope"), std::string::npos);
}
