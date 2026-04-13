#include <gtest/gtest.h>
#include "qos/QosAggregator.h"
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

qos::QosRegistry::Entry MakeEntry(uint64_t seq, int64_t tsMs, int64_t receivedAtMs) {
	auto fixture = LoadFixture("valid_client_v1");
	fixture["seq"] = seq;
	fixture["tsMs"] = tsMs;
	auto parsed = qos::QosValidator::ParseClientSnapshot(fixture);
	EXPECT_TRUE(parsed.ok) << parsed.error;
	return { parsed.value, receivedAtMs };
}

} // namespace

TEST(QosAggregatorTest, AggregateMarksFreshSnapshot) {
	auto entry = MakeEntry(42u, 1000, 1000);
	auto aggregate = qos::QosAggregator::Aggregate(&entry, 4000, 6000, 15000);

	EXPECT_TRUE(aggregate.hasSnapshot);
	EXPECT_FALSE(aggregate.stale);
	EXPECT_FALSE(aggregate.lost);
	EXPECT_EQ(aggregate.lastUpdatedMs, 1000);
	EXPECT_EQ(aggregate.quality, "poor");
	ASSERT_TRUE(aggregate.data.is_object());
	EXPECT_EQ(aggregate.data["seq"], 42u);
	EXPECT_EQ(aggregate.data["quality"], "poor");
}

TEST(QosAggregatorTest, AggregateMarksStaleAndLostByAge) {
	auto entry = MakeEntry(42u, 1000, 1000);

	auto staleAggregate = qos::QosAggregator::Aggregate(&entry, 8005, 6000, 15000);
	EXPECT_TRUE(staleAggregate.stale);
	EXPECT_FALSE(staleAggregate.lost);
	EXPECT_EQ(staleAggregate.quality, "poor");

	auto lostAggregate = qos::QosAggregator::Aggregate(&entry, 17005, 6000, 15000);
	EXPECT_TRUE(lostAggregate.stale);
	EXPECT_TRUE(lostAggregate.lost);
	EXPECT_EQ(lostAggregate.quality, "lost");
	EXPECT_EQ(lostAggregate.data["quality"], "lost");
}

TEST(QosAggregatorTest, AggregateWithoutSnapshotReturnsEmpty) {
	auto aggregate = qos::QosAggregator::Aggregate(nullptr, 1000, 6000, 15000);
	EXPECT_FALSE(aggregate.hasSnapshot);
	EXPECT_TRUE(aggregate.data.is_null() || aggregate.data.empty());
}

TEST(QosAggregatorTest, AggregateAgeClampsToZeroWhenNowBeforeReceive) {
	auto entry = MakeEntry(42u, 1000, 5000);
	auto aggregate = qos::QosAggregator::Aggregate(&entry, 4000, 6000, 15000);

	EXPECT_TRUE(aggregate.hasSnapshot);
	EXPECT_EQ(aggregate.ageMs, 0);
	EXPECT_FALSE(aggregate.stale);
	EXPECT_FALSE(aggregate.lost);
	EXPECT_EQ(aggregate.data["ageMs"], 0);
}

TEST(QosAggregatorTest, ThresholdsAreStrictlyGreaterThanCutoffs) {
	auto entry = MakeEntry(43u, 1000, 1000);

	auto atStaleBoundary = qos::QosAggregator::Aggregate(&entry, 7000, 6000, 15000);
	EXPECT_EQ(atStaleBoundary.ageMs, 6000);
	EXPECT_FALSE(atStaleBoundary.stale);
	EXPECT_FALSE(atStaleBoundary.lost);

	auto atLostBoundary = qos::QosAggregator::Aggregate(&entry, 16000, 6000, 15000);
	EXPECT_EQ(atLostBoundary.ageMs, 15000);
	EXPECT_TRUE(atLostBoundary.stale);
	EXPECT_FALSE(atLostBoundary.lost);
	EXPECT_EQ(atLostBoundary.quality, "poor");
}
