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

TEST(QosValidatorTest, ParsesOverrideOptionalFields) {
	auto fixture = LoadFixture("valid_override_v1");
	fixture["trackId"] = "track-1";
	fixture["maxLevelClamp"] = 2;
	fixture["forceAudioOnly"] = true;
	fixture["disableRecovery"] = false;

	auto parsed = qos::QosValidator::ParseOverride(fixture);
	ASSERT_TRUE(parsed.ok) << parsed.error;
	EXPECT_TRUE(parsed.value.hasTrackId);
	EXPECT_EQ(parsed.value.trackId, "track-1");
	EXPECT_TRUE(parsed.value.hasMaxLevelClamp);
	EXPECT_EQ(parsed.value.maxLevelClamp, 2u);
	EXPECT_TRUE(parsed.value.hasForceAudioOnly);
	EXPECT_TRUE(parsed.value.forceAudioOnly);
	EXPECT_TRUE(parsed.value.hasDisableRecovery);
	EXPECT_FALSE(parsed.value.disableRecovery);
}

TEST(QosValidatorTest, ParsesOverrideWithNullTrackId) {
	auto fixture = LoadFixture("valid_override_v1");
	fixture["trackId"] = nullptr;

	auto parsed = qos::QosValidator::ParseOverride(fixture);
	ASSERT_TRUE(parsed.ok) << parsed.error;
	EXPECT_FALSE(parsed.value.hasTrackId);
}

TEST(QosValidatorTest, RejectsInvalidOverrideForceAudioOnlyType) {
	auto fixture = LoadFixture("valid_override_v1");
	fixture["forceAudioOnly"] = "true";

	auto parsed = qos::QosValidator::ParseOverride(fixture);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("forceAudioOnly"), std::string::npos);
}

TEST(QosValidatorTest, ParsesCanonicalDownlinkSnapshot) {
	json payload = {
		{"schema", "mediasoup.qos.downlink.client.v1"},
		{"seq", 77u},
		{"tsMs", 1700000000000LL},
		{"subscriberPeerId", "peer-1"},
		{"transport", {{"availableIncomingBitrate", 45678.0}, {"currentRoundTripTime", 33.5}}},
		{"subscriptions",
			{
				{
					{"consumerId", "consumer-1"},
					{"producerId", "producer-1"},
					{"visible", true},
					{"pinned", false},
					{"activeSpeaker", true},
					{"isScreenShare", false},
					{"targetWidth", 640u},
					{"targetHeight", 360u}
				}
			}}
	};

	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(payload);
	ASSERT_TRUE(parsed.ok) << parsed.error;
	EXPECT_EQ(parsed.value.schema, "mediasoup.qos.downlink.client.v1");
	EXPECT_EQ(parsed.value.seq, 77u);
	EXPECT_EQ(parsed.value.subscriberPeerId, "peer-1");
	ASSERT_EQ(parsed.value.subscriptions.size(), 1u);
	EXPECT_EQ(parsed.value.subscriptions[0].consumerId, "consumer-1");
	EXPECT_EQ(parsed.value.subscriptions[0].producerId, "producer-1");
	EXPECT_DOUBLE_EQ(parsed.value.availableIncomingBitrate, 45678.0);
	EXPECT_DOUBLE_EQ(parsed.value.currentRoundTripTime, 33.5);
}

TEST(QosValidatorTest, ParsesLegacyDownlinkSchemaByNormalizing) {
	json payload = {
		{"schema", "mediasoup.downlink.v1"},
		{"seq", 1u},
		{"tsMs", 1LL},
		{"subscriberPeerId", "peer-2"},
		{"subscriptions", {{{"consumerId", "c"}, {"producerId", "p"}}}}
	};

	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(payload);
	ASSERT_TRUE(parsed.ok) << parsed.error;
	EXPECT_EQ(parsed.value.schema, "mediasoup.qos.downlink.client.v1");
	EXPECT_EQ(parsed.value.raw["schema"], "mediasoup.qos.downlink.client.v1");
}

TEST(QosValidatorTest, RejectsUnsupportedDownlinkSchema) {
	json payload = {
		{"schema", "mediasoup.qos.downlink.client.v2"},
		{"seq", 1u},
		{"tsMs", 1LL},
		{"subscriberPeerId", "peer-3"},
		{"subscriptions", json::array()}
	};

	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(payload);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("unsupported schema"), std::string::npos);
}

TEST(QosValidatorTest, RejectsDownlinkSeqOutOfRange) {
	json payload = {
		{"schema", "mediasoup.qos.downlink.client.v1"},
		{"seq", qos::kDownlinkMaxSeq + 1u},
		{"tsMs", 1LL},
		{"subscriberPeerId", "peer-4"},
		{"subscriptions", json::array()}
	};

	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(payload);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("seq"), std::string::npos);
}

TEST(QosValidatorTest, RejectsMissingSubscriberPeerIdInDownlinkSnapshot) {
	json payload = {
		{"schema", "mediasoup.qos.downlink.client.v1"},
		{"seq", 1u},
		{"tsMs", 1LL},
		{"subscriberPeerId", ""},
		{"subscriptions", json::array()}
	};

	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(payload);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("subscriberPeerId"), std::string::npos);
}

TEST(QosValidatorTest, RejectsMissingSubscriptionsArrayInDownlinkSnapshot) {
	json payload = {
		{"schema", "mediasoup.qos.downlink.client.v1"},
		{"seq", 1u},
		{"tsMs", 1LL},
		{"subscriberPeerId", "peer-5"},
		{"subscriptions", "not-an-array"}
	};

	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(payload);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("subscriptions"), std::string::npos);
}

TEST(QosValidatorTest, RejectsTooManySubscriptionsInDownlinkSnapshot) {
	json subscriptions = json::array();
	for (size_t i = 0; i < qos::kDownlinkMaxSubscriptions + 1u; ++i) {
		subscriptions.push_back(
			{{"consumerId", "consumer-" + std::to_string(i)}, {"producerId", "producer-" + std::to_string(i)}}
		);
	}

	json payload = {
		{"schema", "mediasoup.qos.downlink.client.v1"},
		{"seq", 1u},
		{"tsMs", 1LL},
		{"subscriberPeerId", "peer-6"},
		{"subscriptions", subscriptions}
	};

	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(payload);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("too many subscriptions"), std::string::npos);
}

TEST(QosValidatorTest, RejectsSubscriptionMissingIdsInDownlinkSnapshot) {
	json payload = {
		{"schema", "mediasoup.qos.downlink.client.v1"},
		{"seq", 1u},
		{"tsMs", 1LL},
		{"subscriberPeerId", "peer-7"},
		{"subscriptions", {{{"consumerId", "consumer-only"}}}}
	};

	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(payload);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("consumerId and producerId"), std::string::npos);
}
