#include <gtest/gtest.h>
#include "qos/QosOverride.h"

using namespace mediasoup;

TEST(QosOverrideBuilderTest, PoorAggregateBuildsClampOverride) {
	qos::PeerQosAggregate aggregate;
	aggregate.hasSnapshot = true;
	aggregate.quality = "poor";

	auto decision = qos::QosOverrideBuilder::BuildForAggregate(aggregate);
	ASSERT_TRUE(decision.has_value());
	EXPECT_EQ(decision->overrideData.reason, "server_auto_poor");
	EXPECT_TRUE(decision->overrideData.hasMaxLevelClamp);
	EXPECT_EQ(decision->overrideData.maxLevelClamp, 2u);
	EXPECT_TRUE(decision->overrideData.hasDisableRecovery);
	EXPECT_TRUE(decision->overrideData.disableRecovery);
	EXPECT_EQ(decision->overrideData.ttlMs, 10000u);
}

TEST(QosOverrideBuilderTest, LostAggregateBuildsForceAudioOnlyOverride) {
	qos::PeerQosAggregate aggregate;
	aggregate.hasSnapshot = true;
	aggregate.quality = "lost";
	aggregate.lost = true;

	auto decision = qos::QosOverrideBuilder::BuildForAggregate(aggregate);
	ASSERT_TRUE(decision.has_value());
	EXPECT_EQ(decision->overrideData.reason, "server_auto_lost");
	EXPECT_TRUE(decision->overrideData.hasForceAudioOnly);
	EXPECT_TRUE(decision->overrideData.forceAudioOnly);
	EXPECT_TRUE(decision->overrideData.hasDisableRecovery);
	EXPECT_TRUE(decision->overrideData.disableRecovery);
	EXPECT_EQ(decision->overrideData.ttlMs, 15000u);
}

TEST(QosOverrideBuilderTest, GoodAggregateDoesNotBuildOverride) {
	qos::PeerQosAggregate aggregate;
	aggregate.hasSnapshot = true;
	aggregate.quality = "good";

	auto decision = qos::QosOverrideBuilder::BuildForAggregate(aggregate);
	EXPECT_FALSE(decision.has_value());
}

TEST(QosOverrideBuilderTest, AggregateWithoutSnapshotDoesNotBuildOverride) {
	qos::PeerQosAggregate aggregate;
	aggregate.hasSnapshot = false;
	aggregate.quality = "lost";
	aggregate.lost = true;

	auto decision = qos::QosOverrideBuilder::BuildForAggregate(aggregate);
	EXPECT_FALSE(decision.has_value());
}

TEST(QosOverrideBuilderTest, LostFlagBuildsLostOverrideEvenWhenQualityIsNotLost) {
	qos::PeerQosAggregate aggregate;
	aggregate.hasSnapshot = true;
	aggregate.quality = "good";
	aggregate.lost = true;

	auto decision = qos::QosOverrideBuilder::BuildForAggregate(aggregate);
	ASSERT_TRUE(decision.has_value());
	EXPECT_EQ(decision->overrideData.reason, "server_auto_lost");
	EXPECT_EQ(decision->signature, decision->overrideData.raw.dump());
}

TEST(QosOverrideBuilderTest, RoomAggregateBuildsPressureOverrideForLostPeers) {
	qos::RoomQosAggregate aggregate;
	aggregate.hasQos = true;
	aggregate.lostPeers = 1u;

	auto decision = qos::QosOverrideBuilder::BuildForRoomAggregate(aggregate);
	ASSERT_TRUE(decision.has_value());
	EXPECT_EQ(decision->overrideData.reason, "server_room_pressure");
	EXPECT_TRUE(decision->overrideData.hasMaxLevelClamp);
	EXPECT_EQ(decision->overrideData.maxLevelClamp, 1u);
	EXPECT_TRUE(decision->overrideData.hasDisableRecovery);
	EXPECT_TRUE(decision->overrideData.disableRecovery);
	EXPECT_EQ(decision->overrideData.ttlMs, 8000u);
	EXPECT_EQ(decision->signature, decision->overrideData.raw.dump());
}

TEST(QosOverrideBuilderTest, RoomAggregateBuildsPressureOverrideForTwoPoorPeers) {
	qos::RoomQosAggregate aggregate;
	aggregate.hasQos = true;
	aggregate.poorPeers = 2u;

	auto decision = qos::QosOverrideBuilder::BuildForRoomAggregate(aggregate);
	ASSERT_TRUE(decision.has_value());
	EXPECT_EQ(decision->overrideData.reason, "server_room_pressure");
}

TEST(QosOverrideBuilderTest, RoomAggregateWithSinglePoorPeerDoesNotBuildOverride) {
	qos::RoomQosAggregate aggregate;
	aggregate.hasQos = true;
	aggregate.poorPeers = 1u;
	aggregate.lostPeers = 0u;

	auto decision = qos::QosOverrideBuilder::BuildForRoomAggregate(aggregate);
	EXPECT_FALSE(decision.has_value());
}

TEST(QosOverrideBuilderTest, RoomAggregateWithoutQosDoesNotBuildOverride) {
	qos::RoomQosAggregate aggregate;
	aggregate.hasQos = false;
	aggregate.poorPeers = 3u;
	aggregate.lostPeers = 2u;

	auto decision = qos::QosOverrideBuilder::BuildForRoomAggregate(aggregate);
	EXPECT_FALSE(decision.has_value());
}
