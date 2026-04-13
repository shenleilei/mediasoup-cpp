#include <gtest/gtest.h>
#include "qos/SubscriberQosController.h"

using namespace mediasoup::qos;

// --- isPaused ---

TEST(SubscriberQosControllerTest, InitiallyNoPausedConsumers) {
	SubscriberQosController ctrl;
	EXPECT_FALSE(ctrl.isPaused("c1"));
	EXPECT_FALSE(ctrl.isPaused("c2"));
}

// --- getPausedFlags ---

TEST(SubscriberQosControllerTest, GetPausedFlagsEmptySubscriptions) {
	SubscriberQosController ctrl;
	std::vector<DownlinkSubscription> subs;
	auto flags = ctrl.getPausedFlags(subs);
	EXPECT_TRUE(flags.empty());
}

TEST(SubscriberQosControllerTest, GetPausedFlagsAllUnpaused) {
	SubscriberQosController ctrl;
	std::vector<DownlinkSubscription> subs = {
		{.consumerId = "c1"},
		{.consumerId = "c2"},
		{.consumerId = "c3"},
	};
	auto flags = ctrl.getPausedFlags(subs);
	ASSERT_EQ(flags.size(), 3u);
	EXPECT_FALSE(flags[0]);
	EXPECT_FALSE(flags[1]);
	EXPECT_FALSE(flags[2]);
}

// --- healthMonitor access ---

TEST(SubscriberQosControllerTest, HealthMonitorStartsStable) {
	SubscriberQosController ctrl;
	EXPECT_EQ(ctrl.healthMonitor().state(), DownlinkHealth::kStable);
	EXPECT_EQ(ctrl.healthMonitor().degradeLevel(), 0);
}

TEST(SubscriberQosControllerTest, HealthMonitorAccessibleConst) {
	const SubscriberQosController ctrl;
	EXPECT_EQ(ctrl.healthMonitor().state(), DownlinkHealth::kStable);
}
