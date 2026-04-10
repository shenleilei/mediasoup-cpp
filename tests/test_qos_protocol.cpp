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

TEST(QosProtocolTest, ParsesValidClientSnapshotFixture) {
	auto fixture = LoadFixture("valid_client_v1");
	auto parsed = qos::QosValidator::ParseClientSnapshot(fixture);

	ASSERT_TRUE(parsed.ok) << parsed.error;
	EXPECT_EQ(parsed.value.schema, "mediasoup.qos.client.v1");
	EXPECT_EQ(parsed.value.seq, 42u);
	EXPECT_EQ(parsed.value.peerMode, "audio-video");
	EXPECT_EQ(parsed.value.peerQuality, "poor");
	ASSERT_EQ(parsed.value.tracks.size(), 1u);
	EXPECT_EQ(parsed.value.tracks[0].localTrackId, "camera-main");
	EXPECT_EQ(parsed.value.tracks[0].source, "camera");
}

TEST(QosProtocolTest, ParsesValidPolicyFixture) {
	auto fixture = LoadFixture("valid_policy_v1");
	auto parsed = qos::QosValidator::ParsePolicy(fixture);

	ASSERT_TRUE(parsed.ok) << parsed.error;
	EXPECT_EQ(parsed.value.sampleIntervalMs, 1000u);
	EXPECT_EQ(parsed.value.snapshotIntervalMs, 2000u);
	EXPECT_TRUE(parsed.value.allowAudioOnly);
	EXPECT_EQ(parsed.value.cameraProfile, "default");
}

TEST(QosProtocolTest, ParsesValidOverrideFixture) {
	auto fixture = LoadFixture("valid_override_v1");
	auto parsed = qos::QosValidator::ParseOverride(fixture);

	ASSERT_TRUE(parsed.ok) << parsed.error;
	EXPECT_EQ(parsed.value.scope, "peer");
	EXPECT_TRUE(parsed.value.hasMaxLevelClamp);
	EXPECT_EQ(parsed.value.maxLevelClamp, 2u);
	EXPECT_TRUE(parsed.value.hasForceAudioOnly);
	EXPECT_FALSE(parsed.value.forceAudioOnly);
	EXPECT_EQ(parsed.value.ttlMs, 10000u);
}
