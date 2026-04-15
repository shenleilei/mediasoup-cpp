#include <gtest/gtest.h>
#include "qos/QosSnapshot.h"
#include <thread>
#include <chrono>

using namespace mediasoup;
using json = nlohmann::json;

TEST(QosSnapshotTest, NowMsIsNonNegativeAndMonotonic) {
	const auto t1 = qos::NowMs();
	std::this_thread::sleep_for(std::chrono::milliseconds(2));
	const auto t2 = qos::NowMs();

	EXPECT_GE(t1, 0);
	EXPECT_GE(t2, t1);
}

TEST(QosSnapshotTest, ToJsonClientSnapshotReturnsRawPayload) {
	qos::ClientQosSnapshot snapshot;
	snapshot.raw = {{"schema", "client/v1"}, {"seq", 42}, {"quality", "good"}};

	auto encoded = qos::ToJson(snapshot);
	EXPECT_EQ(encoded, snapshot.raw);

	encoded["seq"] = 100;
	EXPECT_EQ(snapshot.raw["seq"], 42);
}

TEST(QosSnapshotTest, ToJsonPolicyReturnsRawPayload) {
	qos::QosPolicy policy;
	policy.raw = {{"schema", "policy/v1"}, {"sampleIntervalMs", 1000}, {"allowAudioOnly", true}};

	auto encoded = qos::ToJson(policy);
	EXPECT_EQ(encoded, policy.raw);

	encoded["allowAudioOnly"] = false;
	EXPECT_EQ(policy.raw["allowAudioOnly"], true);
}

TEST(QosSnapshotTest, ToJsonOverrideReturnsRawPayload) {
	qos::QosOverride overrideData;
	overrideData.raw = {{"schema", "override/v1"}, {"scope", "peer"}, {"ttlMs", 5000}};

	auto encoded = qos::ToJson(overrideData);
	EXPECT_EQ(encoded, overrideData.raw);

	encoded["ttlMs"] = 0;
	EXPECT_EQ(overrideData.raw["ttlMs"], 5000);
}

TEST(QosSnapshotTest, ToJsonPeerAggregateReturnsDataPayload) {
	qos::PeerQosAggregate aggregate;
	aggregate.data = {{"quality", "poor"}, {"stale", true}, {"lost", false}};

	auto encoded = qos::ToJson(aggregate);
	EXPECT_EQ(encoded, aggregate.data);

	encoded["quality"] = "good";
	EXPECT_EQ(aggregate.data["quality"], "poor");
}
