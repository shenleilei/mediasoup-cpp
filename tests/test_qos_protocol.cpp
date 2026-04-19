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

TEST(QosProtocolTest, ParsesVideoOnlyClientSnapshotWithExtendedSignalFields) {
	auto fixture = LoadFixture("valid_client_v1");
	fixture["peerState"]["mode"] = "video-only";
	fixture["tracks"][0]["signals"]["bitrateUtilization"] = 0.82;
	fixture["tracks"][0]["signals"]["lossEwma"] = 0.05;
	fixture["tracks"][0]["signals"]["rttEwma"] = 140.0;
	fixture["tracks"][0]["signals"]["jitterEwma"] = 16.0;

	auto parsed = qos::QosValidator::ParseClientSnapshot(fixture);

	ASSERT_TRUE(parsed.ok) << parsed.error;
	EXPECT_EQ(parsed.value.peerMode, "video-only");
	EXPECT_DOUBLE_EQ(parsed.value.tracks[0].signals["bitrateUtilization"].get<double>(), 0.82);
	EXPECT_DOUBLE_EQ(parsed.value.tracks[0].signals["lossEwma"].get<double>(), 0.05);
	EXPECT_DOUBLE_EQ(parsed.value.tracks[0].signals["rttEwma"].get<double>(), 140.0);
	EXPECT_DOUBLE_EQ(parsed.value.tracks[0].signals["jitterEwma"].get<double>(), 16.0);
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

TEST(QosProtocolTest, ParsesV3PauseResumeOverrideFields) {
	json payload = {
		{"schema", "mediasoup.qos.override.v1"},
		{"scope", "track"},
		{"trackId", "track-1"},
		{"pauseUpstream", true},
		{"resumeUpstream", false},
		{"ttlMs", 5000},
		{"reason", "downlink_v3_zero_demand_pause"}
	};

	auto parsed = qos::QosValidator::ParseOverride(payload);

	ASSERT_TRUE(parsed.ok) << parsed.error;
	EXPECT_TRUE(parsed.value.hasTrackId);
	EXPECT_EQ(parsed.value.trackId, "track-1");
	EXPECT_TRUE(parsed.value.hasPauseUpstream);
	EXPECT_TRUE(parsed.value.pauseUpstream);
	EXPECT_TRUE(parsed.value.hasResumeUpstream);
	EXPECT_FALSE(parsed.value.resumeUpstream);
}

TEST(QosProtocolTest, RejectsTrackScopedOverrideWithoutTrackId) {
	json payload = {
		{"schema", "mediasoup.qos.override.v1"},
		{"scope", "track"},
		{"ttlMs", 5000},
		{"reason", "downlink_v3_zero_demand_pause"}
	};

	auto parsed = qos::QosValidator::ParseOverride(payload);

	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("trackId"), std::string::npos);
}
