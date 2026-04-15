#include <gtest/gtest.h>
#include "qos/QosValidator.h"
#include <fstream>
#include <limits>

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

TEST(QosValidatorTest, RejectsInvalidPauseUpstreamType) {
	auto fixture = LoadFixture("valid_override_v1");
	fixture["pauseUpstream"] = "yes";

	auto parsed = qos::QosValidator::ParseOverride(fixture);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("pauseUpstream"), std::string::npos);
}

TEST(QosValidatorTest, RejectsMutuallyExclusivePauseAndResumeUpstream) {
	auto fixture = LoadFixture("valid_override_v1");
	fixture["pauseUpstream"] = true;
	fixture["resumeUpstream"] = true;

	auto parsed = qos::QosValidator::ParseOverride(fixture);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("mutually exclusive"), std::string::npos);
}

// ── Downlink snapshot validation tests ──

namespace {

json MakeValidDownlinkSnapshot() {
	return {
		{"schema", "mediasoup.qos.downlink.client.v1"},
		{"seq", 1},
		{"tsMs", 1000},
		{"subscriberPeerId", "peer1"},
		{"transport", {{"availableIncomingBitrate", 1000000.0}, {"currentRoundTripTime", 0.05}}},
		{"subscriptions", {{
			{"consumerId", "c1"},
			{"producerId", "p1"},
			{"visible", true},
			{"targetWidth", 640},
			{"targetHeight", 480}
		}}}
	};
}

} // namespace

TEST(QosValidatorTest, DownlinkAcceptsValidSnapshot) {
	auto snap = MakeValidDownlinkSnapshot();
	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(snap);
	EXPECT_TRUE(parsed.ok) << parsed.error;
	EXPECT_EQ(parsed.value.subscriberPeerId, "peer1");
	EXPECT_EQ(parsed.value.subscriptions.size(), 1u);
}

TEST(QosValidatorTest, DownlinkRejectsOverlongPeerId) {
	auto snap = MakeValidDownlinkSnapshot();
	snap["subscriberPeerId"] = std::string(200, 'x');
	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(snap);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("too long"), std::string::npos);
}

TEST(QosValidatorTest, DownlinkRejectsOverlongConsumerId) {
	auto snap = MakeValidDownlinkSnapshot();
	snap["subscriptions"][0]["consumerId"] = std::string(200, 'y');
	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(snap);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("id too long"), std::string::npos);
}

TEST(QosValidatorTest, DownlinkClampsFreezeRate) {
	auto snap = MakeValidDownlinkSnapshot();
	snap["subscriptions"][0]["freezeRate"] = 2.5;
	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(snap);
	EXPECT_TRUE(parsed.ok) << parsed.error;
	EXPECT_LE(parsed.value.subscriptions[0].freezeRate, 1.0);
}

TEST(QosValidatorTest, DownlinkClampsNegativeJitter) {
	auto snap = MakeValidDownlinkSnapshot();
	snap["subscriptions"][0]["jitter"] = -0.5;
	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(snap);
	EXPECT_TRUE(parsed.ok) << parsed.error;
	EXPECT_GE(parsed.value.subscriptions[0].jitter, 0.0);
}

TEST(QosValidatorTest, DownlinkRejectsTooManySubscriptions) {
	auto snap = MakeValidDownlinkSnapshot();
	json subs = json::array();
	for (size_t i = 0; i <= qos::kDownlinkMaxSubscriptions; ++i) {
		subs.push_back({
			{"consumerId", "c" + std::to_string(i)},
			{"producerId", "p" + std::to_string(i)}
		});
	}
	snap["subscriptions"] = subs;
	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(snap);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("too many"), std::string::npos);
}

TEST(QosValidatorTest, DownlinkRejectsDuplicateConsumerIds) {
	auto snap = MakeValidDownlinkSnapshot();
	snap["subscriptions"].push_back({
		{"consumerId", "c1"},
		{"producerId", "p2"},
		{"visible", true}
	});
	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(snap);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("duplicate"), std::string::npos);
}

TEST(QosValidatorTest, DownlinkRejectsInvalidSubscriptionKind) {
	auto snap = MakeValidDownlinkSnapshot();
	snap["subscriptions"][0]["kind"] = "data";
	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(snap);
	EXPECT_FALSE(parsed.ok);
	EXPECT_NE(parsed.error.find("kind"), std::string::npos);
}

TEST(QosValidatorTest, DownlinkClampsNegativeRtt) {
	auto snap = MakeValidDownlinkSnapshot();
	snap["transport"]["currentRoundTripTime"] = -1.0;
	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(snap);
	EXPECT_TRUE(parsed.ok) << parsed.error;
	EXPECT_GE(parsed.value.currentRoundTripTime, 0.0);
}

TEST(QosValidatorTest, DownlinkSanitizesNonFiniteMetrics) {
	auto snap = MakeValidDownlinkSnapshot();
	snap["transport"]["availableIncomingBitrate"] = std::numeric_limits<double>::infinity();
	snap["transport"]["currentRoundTripTime"] = -std::numeric_limits<double>::infinity();
	snap["subscriptions"][0]["jitter"] = std::numeric_limits<double>::quiet_NaN();
	snap["subscriptions"][0]["framesPerSecond"] = std::numeric_limits<double>::infinity();
	snap["subscriptions"][0]["freezeRate"] = std::numeric_limits<double>::quiet_NaN();

	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(snap);
	EXPECT_TRUE(parsed.ok) << parsed.error;
	EXPECT_EQ(parsed.value.availableIncomingBitrate, 0.0);
	EXPECT_EQ(parsed.value.currentRoundTripTime, 0.0);
	EXPECT_EQ(parsed.value.subscriptions[0].jitter, 0.0);
	EXPECT_EQ(parsed.value.subscriptions[0].framesPerSecond, 0.0);
	EXPECT_EQ(parsed.value.subscriptions[0].freezeRate, 0.0);
}

TEST(QosValidatorTest, DownlinkAcceptsLegacySchema) {
	auto snap = MakeValidDownlinkSnapshot();
	snap["schema"] = "mediasoup.downlink.v1";
	auto parsed = qos::QosValidator::ParseDownlinkSnapshot(snap);
	EXPECT_TRUE(parsed.ok) << parsed.error;
	EXPECT_EQ(parsed.value.schema, "mediasoup.qos.downlink.client.v1");
}
