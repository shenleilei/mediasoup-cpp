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
