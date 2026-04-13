#include <gtest/gtest.h>
#include "qos/DownlinkHealthMonitor.h"
#include "qos/DownlinkAllocator.h"

using namespace mediasoup::qos;

static DownlinkSnapshot makeSnapshot(double freezeRate, double jitter, uint32_t packetsLost, double rtt) {
	DownlinkSnapshot snap;
	snap.schema = "mediasoup.qos.downlink.client.v1";
	snap.currentRoundTripTime = rtt;
	snap.subscriptions.push_back({
		.consumerId = "c1", .visible = true, .targetWidth = 640,
		.packetsLost = packetsLost, .jitter = jitter,
		.framesPerSecond = 25.0, .freezeRate = freezeRate
	});
	return snap;
}

TEST(DownlinkHealthMonitorTest, StartsStable) {
	DownlinkHealthMonitor mon;
	EXPECT_EQ(mon.state(), DownlinkHealth::kStable);
	EXPECT_EQ(mon.degradeLevel(), 0);
}

TEST(DownlinkHealthMonitorTest, FreezeTriggersDegrade) {
	DownlinkHealthMonitor mon;
	// Congested freeze
	auto snap = makeSnapshot(0.2, 0.0, 0, 0.0);
	mon.update(snap);
	EXPECT_EQ(mon.state(), DownlinkHealth::kCongested);
	EXPECT_GE(mon.degradeLevel(), 1);
}

TEST(DownlinkHealthMonitorTest, RecoveryIsStepwise) {
	DownlinkHealthMonitor mon;
	// Drive to congested
	auto bad = makeSnapshot(0.2, 0.0, 0, 0.0);
	mon.update(bad);
	mon.update(bad);
	int peakLevel = mon.degradeLevel();
	EXPECT_GT(peakLevel, 0);

	// Send stable snapshots — recovery requires 3 stable in a row
	auto good = makeSnapshot(0.0, 0.0, 0, 0.0);
	mon.update(good); // enters recovering
	mon.update(good);
	mon.update(good); // should step down by 1
	int afterRecovery = mon.degradeLevel();
	EXPECT_LT(afterRecovery, peakLevel);
	// Should not jump to 0 immediately if peak was high
	if (peakLevel > 1) {
		EXPECT_GT(afterRecovery, 0);
	}
}

TEST(DownlinkHealthMonitorTest, EarlyWarningMildDegrade) {
	DownlinkHealthMonitor mon;
	// Warning-level freeze (between warn and congested thresholds)
	auto snap = makeSnapshot(0.08, 0.0, 0, 0.0);
	mon.update(snap);
	EXPECT_EQ(mon.state(), DownlinkHealth::kEarlyWarning);
	EXPECT_EQ(mon.degradeLevel(), 1);
}

TEST(DownlinkHealthMonitorTest, DegradeLevelClampsAllocatorLayers) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .pinned = true, .targetWidth = 1280
	}};
	auto findLayers = [](const std::vector<DownlinkAction>& actions) -> const DownlinkAction* {
		for (auto& a : actions)
			if (a.type == DownlinkAction::Type::kSetLayers) return &a;
		return nullptr;
	};

	auto a0 = findLayers(DownlinkAllocator::Compute(subs, {false}, 0));
	ASSERT_NE(a0, nullptr);
	EXPECT_EQ(a0->spatialLayer, 2);

	auto a2 = findLayers(DownlinkAllocator::Compute(subs, {false}, 2));
	ASSERT_NE(a2, nullptr);
	EXPECT_EQ(a2->spatialLayer, 1);

	auto a3 = findLayers(DownlinkAllocator::Compute(subs, {false}, 3));
	ASSERT_NE(a3, nullptr);
	EXPECT_EQ(a3->spatialLayer, 0);
}
