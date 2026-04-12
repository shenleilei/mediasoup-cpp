#include <gtest/gtest.h>
#include "qos/QosRoomAggregator.h"

using namespace mediasoup;

TEST(QosRoomAggregatorTest, AggregatesRoomWorstQualityAndCounts) {
	qos::PeerQosAggregate a;
	a.hasSnapshot = true;
	a.quality = "good";
	a.data = {{"quality", "good"}};

	qos::PeerQosAggregate b;
	b.hasSnapshot = true;
	b.quality = "poor";
	b.data = {{"quality", "poor"}};

	qos::PeerQosAggregate c;
	c.hasSnapshot = true;
	c.quality = "lost";
	c.lost = true;
	c.data = {{"quality", "lost"}};

	auto aggregate = qos::QosRoomAggregator::Aggregate({a, b, c});

	EXPECT_TRUE(aggregate.hasQos);
	EXPECT_EQ(aggregate.peerCount, 3u);
	EXPECT_EQ(aggregate.poorPeers, 1u);
	EXPECT_EQ(aggregate.lostPeers, 1u);
	EXPECT_EQ(aggregate.quality, "lost");
	ASSERT_TRUE(aggregate.data.is_object());
	EXPECT_EQ(aggregate.data["quality"], "lost");
}

TEST(QosRoomAggregatorTest, EmptyRoomProducesNoQos) {
	auto aggregate = qos::QosRoomAggregator::Aggregate({});
	EXPECT_FALSE(aggregate.hasQos);
	EXPECT_EQ(aggregate.peerCount, 0u);
	EXPECT_TRUE(aggregate.data.is_null() || aggregate.data.empty());
}
