#include <gtest/gtest.h>
#include "qos/QosSnapshot.h"

using namespace mediasoup::qos;

// --- NowMs ---

TEST(QosSnapshotTest, NowMsReturnsPositiveTimestamp) {
	auto ts = NowMs();
	EXPECT_GT(ts, 0);
}

TEST(QosSnapshotTest, NowMsIsMonotonic) {
	auto ts1 = NowMs();
	auto ts2 = NowMs();
	EXPECT_GE(ts2, ts1);
}

// --- ToJson(ClientQosSnapshot) ---

TEST(QosSnapshotTest, ClientQosSnapshotToJsonReturnsRaw) {
	ClientQosSnapshot snap;
	snap.raw = {{"schema", "v1"}, {"seq", 42}};
	auto j = ToJson(snap);
	EXPECT_EQ(j["schema"], "v1");
	EXPECT_EQ(j["seq"], 42);
}

TEST(QosSnapshotTest, ClientQosSnapshotToJsonEmptyRaw) {
	ClientQosSnapshot snap;
	snap.raw = json::object();
	auto j = ToJson(snap);
	EXPECT_TRUE(j.is_object());
	EXPECT_TRUE(j.empty());
}

// --- ToJson(DownlinkSnapshot) ---

TEST(QosSnapshotTest, DownlinkSnapshotToJsonReturnsRaw) {
	DownlinkSnapshot snap;
	snap.raw = {{"schema", "downlink.v1"}, {"bw", 1000000}};
	auto j = ToJson(snap);
	EXPECT_EQ(j["schema"], "downlink.v1");
	EXPECT_EQ(j["bw"], 1000000);
}

// --- ToJson(QosPolicy) ---

TEST(QosSnapshotTest, QosPolicyToJsonReturnsRaw) {
	QosPolicy policy;
	policy.raw = {{"schema", "policy.v1"}, {"sampleIntervalMs", 1000}};
	auto j = ToJson(policy);
	EXPECT_EQ(j["schema"], "policy.v1");
	EXPECT_EQ(j["sampleIntervalMs"], 1000);
}

// --- ToJson(QosOverride) ---

TEST(QosSnapshotTest, QosOverrideToJsonReturnsRaw) {
	QosOverride overrideData;
	overrideData.raw = {{"schema", "override.v1"}, {"reason", "server_auto_poor"}};
	auto j = ToJson(overrideData);
	EXPECT_EQ(j["schema"], "override.v1");
	EXPECT_EQ(j["reason"], "server_auto_poor");
}

// --- ToJson(PeerQosAggregate) ---

TEST(QosSnapshotTest, PeerQosAggregateToJsonReturnsData) {
	PeerQosAggregate agg;
	agg.data = {{"quality", "good"}, {"stale", false}};
	auto j = ToJson(agg);
	EXPECT_EQ(j["quality"], "good");
	EXPECT_EQ(j["stale"], false);
}

TEST(QosSnapshotTest, PeerQosAggregateToJsonWithEmptyData) {
	PeerQosAggregate agg;
	agg.data = json::object();
	auto j = ToJson(agg);
	EXPECT_TRUE(j.is_object());
	EXPECT_TRUE(j.empty());
}

TEST(QosSnapshotTest, PeerQosAggregateToJsonWithNullData) {
	PeerQosAggregate agg;
	// agg.data is default constructed as null
	auto j = ToJson(agg);
	EXPECT_TRUE(j.is_null());
}
