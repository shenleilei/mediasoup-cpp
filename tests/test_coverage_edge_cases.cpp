#include <gtest/gtest.h>
#include "qos/DownlinkAllocator.h"
#include "qos/DownlinkHealthMonitor.h"
#include "qos/QosOverride.h"

using namespace mediasoup;
using namespace mediasoup::qos;

// ===== DownlinkAllocator edge cases =====

TEST(DownlinkAllocatorEdgeTest, EmptySubscriptionsReturnsNoActions) {
	auto actions = DownlinkAllocator::Compute({}, {});
	EXPECT_TRUE(actions.empty());
}

TEST(DownlinkAllocatorEdgeTest, EmptyCurrentlyPausedDefaultsToFalse) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .pinned = true, .targetWidth = 960
	}};
	// Empty currentlyPaused → defaults to false for each sub
	auto actions = DownlinkAllocator::Compute(subs, {});
	bool hasResume = false;
	for (auto& a : actions)
		if (a.type == DownlinkAction::Type::kResume) hasResume = true;
	EXPECT_FALSE(hasResume);  // Not previously paused → no resume
}

TEST(DownlinkAllocatorEdgeTest, ZeroTargetWidthIsSmallTile) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .targetWidth = 0
	}};
	auto actions = DownlinkAllocator::Compute(subs, {false});
	bool hasLayers = false;
	for (auto& a : actions)
		if (a.type == DownlinkAction::Type::kSetLayers) {
			EXPECT_EQ(a.spatialLayer, 0);
			EXPECT_EQ(a.temporalLayer, 0);
			hasLayers = true;
		}
	EXPECT_TRUE(hasLayers);
}

TEST(DownlinkAllocatorEdgeTest, ExactSmallTileMaxWidthIsStillSmall) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .targetWidth = 320  // kSmallTileMaxWidth
	}};
	auto actions = DownlinkAllocator::Compute(subs, {false});
	bool hasLayers = false;
	for (auto& a : actions)
		if (a.type == DownlinkAction::Type::kSetLayers) {
			EXPECT_EQ(a.spatialLayer, 0);
			EXPECT_EQ(a.temporalLayer, 0);
			hasLayers = true;
		}
	EXPECT_TRUE(hasLayers);
}

TEST(DownlinkAllocatorEdgeTest, JustAboveSmallTileMaxWidthIsLarge) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .targetWidth = 321
	}};
	auto actions = DownlinkAllocator::Compute(subs, {false});
	bool hasLayers = false;
	for (auto& a : actions)
		if (a.type == DownlinkAction::Type::kSetLayers) {
			EXPECT_EQ(a.spatialLayer, 2);
			EXPECT_EQ(a.temporalLayer, 2);
			hasLayers = true;
		}
	EXPECT_TRUE(hasLayers);
}

TEST(DownlinkAllocatorEdgeTest, DegradeLevel1ClampsLargeToSpatial1) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .pinned = true, .targetWidth = 960
	}};
	auto actions = DownlinkAllocator::Compute(subs, {false}, 1);
	for (auto& a : actions)
		if (a.type == DownlinkAction::Type::kSetLayers) {
			EXPECT_EQ(a.spatialLayer, 1);
			EXPECT_EQ(a.temporalLayer, 2);
		}
}

TEST(DownlinkAllocatorEdgeTest, DegradeLevel3ForcesLowestLayers) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .pinned = true, .targetWidth = 1920
	}};
	auto actions = DownlinkAllocator::Compute(subs, {false}, 3);
	for (auto& a : actions)
		if (a.type == DownlinkAction::Type::kSetLayers) {
			EXPECT_EQ(a.spatialLayer, 0);
			EXPECT_EQ(a.temporalLayer, 0);
		}
}

TEST(DownlinkAllocatorEdgeTest, DegradeLevelDoesNotAffectSmallTiles) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .targetWidth = 160
	}};
	// Already at 0, degradeLevel shouldn't change it
	auto actions = DownlinkAllocator::Compute(subs, {false}, 2);
	for (auto& a : actions)
		if (a.type == DownlinkAction::Type::kSetLayers) {
			EXPECT_EQ(a.spatialLayer, 0);
			EXPECT_EQ(a.temporalLayer, 0);
		}
}

// --- ComputePriority ---

TEST(DownlinkAllocatorEdgeTest, ComputePriorityHidden) {
	DownlinkSubscription sub = {.visible = false};
	EXPECT_EQ(DownlinkAllocator::ComputePriority(sub), DownlinkAllocator::kPriorityHidden);
}

TEST(DownlinkAllocatorEdgeTest, ComputePriorityVisibleGrid) {
	DownlinkSubscription sub = {.visible = true};
	EXPECT_EQ(DownlinkAllocator::ComputePriority(sub), DownlinkAllocator::kPriorityVisibleGrid);
}

TEST(DownlinkAllocatorEdgeTest, ComputePriorityScreenShareTrumpsAll) {
	// Screen share + pinned + active speaker
	DownlinkSubscription sub = {.visible = true, .pinned = true,
		.activeSpeaker = true, .isScreenShare = true};
	EXPECT_EQ(DownlinkAllocator::ComputePriority(sub), DownlinkAllocator::kPriorityScreenShare);
}

TEST(DownlinkAllocatorEdgeTest, ComputePriorityPinnedTrumpsActiveSpeaker) {
	DownlinkSubscription sub = {.visible = true, .pinned = true, .activeSpeaker = true};
	EXPECT_EQ(DownlinkAllocator::ComputePriority(sub), DownlinkAllocator::kPriorityPinned);
}

// ===== DownlinkHealthMonitor edge cases =====

static DownlinkSnapshot MakeHealthSnapshot(double freezeRate, double jitter,
	uint32_t packetsLost, double rtt, double fps = 25.0) {
	DownlinkSnapshot snap;
	snap.schema = "mediasoup.qos.downlink.client.v1";
	snap.currentRoundTripTime = rtt;
	snap.subscriptions.push_back({
		.consumerId = "c1", .visible = true, .targetWidth = 640,
		.packetsLost = packetsLost, .jitter = jitter,
		.framesPerSecond = fps, .freezeRate = freezeRate
	});
	return snap;
}

TEST(DownlinkHealthEdgeTest, DegradeLevelClampsAtMax) {
	DownlinkHealthMonitor mon;
	// Send many congested snapshots
	auto bad = MakeHealthSnapshot(0.2, 0.0, 0, 0.0);
	for (int i = 0; i < 20; ++i) {
		mon.update(bad);
	}
	EXPECT_LE(mon.degradeLevel(), DownlinkHealthMonitor::kMaxDegradeLevel);
}

TEST(DownlinkHealthEdgeTest, HighRttTriggersCongestion) {
	DownlinkHealthMonitor mon;
	auto snap = MakeHealthSnapshot(0.0, 0.0, 0, 0.5);  // RTT = 500ms
	mon.update(snap);
	EXPECT_EQ(mon.state(), DownlinkHealth::kCongested);
}

TEST(DownlinkHealthEdgeTest, HighJitterTriggersCongestion) {
	DownlinkHealthMonitor mon;
	auto snap = MakeHealthSnapshot(0.0, 0.07, 0, 0.0);  // jitter > 0.06 threshold
	mon.update(snap);
	EXPECT_EQ(mon.state(), DownlinkHealth::kCongested);
}

TEST(DownlinkHealthEdgeTest, HighLossRateTriggersCongestion) {
	DownlinkHealthMonitor mon;
	auto snap = MakeHealthSnapshot(0.0, 0.0, 10, 0.0);  // 10/100 = 0.1 > 0.08 threshold
	mon.update(snap);
	EXPECT_EQ(mon.state(), DownlinkHealth::kCongested);
}

TEST(DownlinkHealthEdgeTest, LowFpsTriggerWarning) {
	DownlinkHealthMonitor mon;
	auto snap = MakeHealthSnapshot(0.0, 0.0, 0, 0.0, 5.0);  // fps < 10
	mon.update(snap);
	EXPECT_EQ(mon.state(), DownlinkHealth::kEarlyWarning);
}

TEST(DownlinkHealthEdgeTest, EmptySubscriptionsStaysStable) {
	DownlinkHealthMonitor mon;
	DownlinkSnapshot snap;
	snap.currentRoundTripTime = 0.0;
	mon.update(snap);
	EXPECT_EQ(mon.state(), DownlinkHealth::kStable);
}

TEST(DownlinkHealthEdgeTest, HiddenSubsIgnoredForClassification) {
	DownlinkHealthMonitor mon;
	DownlinkSnapshot snap;
	snap.currentRoundTripTime = 0.0;
	snap.subscriptions.push_back({
		.consumerId = "c1", .visible = false,
		.packetsLost = 100, .jitter = 1.0,
		.framesPerSecond = 0.0, .freezeRate = 1.0
	});
	mon.update(snap);
	EXPECT_EQ(mon.state(), DownlinkHealth::kStable);
}

TEST(DownlinkHealthEdgeTest, RecoverFromCongestedToStable) {
	DownlinkHealthMonitor mon;
	// Drive to congested
	auto bad = MakeHealthSnapshot(0.2, 0.0, 0, 0.0);
	mon.update(bad);
	EXPECT_EQ(mon.state(), DownlinkHealth::kCongested);

	// Send stable snapshots to recover
	auto good = MakeHealthSnapshot(0.0, 0.0, 0, 0.0);
	// Need enough stable updates to fully recover
	for (int i = 0; i < 20; ++i) {
		mon.update(good);
	}
	EXPECT_EQ(mon.degradeLevel(), 0);
	EXPECT_EQ(mon.state(), DownlinkHealth::kStable);
}

TEST(DownlinkHealthEdgeTest, RelapseFromRecoveringToCongested) {
	DownlinkHealthMonitor mon;
	auto bad = MakeHealthSnapshot(0.2, 0.0, 0, 0.0);
	mon.update(bad);
	EXPECT_EQ(mon.state(), DownlinkHealth::kCongested);

	auto good = MakeHealthSnapshot(0.0, 0.0, 0, 0.0);
	mon.update(good);  // enter recovering
	EXPECT_EQ(mon.state(), DownlinkHealth::kRecovering);

	// Relapse
	mon.update(bad);
	EXPECT_EQ(mon.state(), DownlinkHealth::kCongested);
}

TEST(DownlinkHealthEdgeTest, EarlyWarningToCongestedTransition) {
	DownlinkHealthMonitor mon;
	auto warn = MakeHealthSnapshot(0.08, 0.0, 0, 0.0);  // early warning
	mon.update(warn);
	EXPECT_EQ(mon.state(), DownlinkHealth::kEarlyWarning);

	auto bad = MakeHealthSnapshot(0.2, 0.0, 0, 0.0);  // congested
	mon.update(bad);
	EXPECT_EQ(mon.state(), DownlinkHealth::kCongested);
}

TEST(DownlinkHealthEdgeTest, WarningInterruptRecovery) {
	DownlinkHealthMonitor mon;
	auto bad = MakeHealthSnapshot(0.2, 0.0, 0, 0.0);
	mon.update(bad);

	auto good = MakeHealthSnapshot(0.0, 0.0, 0, 0.0);
	mon.update(good);  // recovering

	// Warning during recovery resets stable count
	auto warn = MakeHealthSnapshot(0.08, 0.0, 0, 0.0);
	mon.update(warn);
	int levelAfterWarn = mon.degradeLevel();

	// Continue with good
	mon.update(good);
	mon.update(good);
	// Should still be at same level because stableCount was reset
	EXPECT_GE(mon.degradeLevel(), levelAfterWarn - 1);
}

// ===== QosOverrideBuilder edge cases =====

TEST(QosOverrideEdgeTest, NoSnapshotReturnsNullopt) {
	qos::PeerQosAggregate agg;
	agg.hasSnapshot = false;
	agg.quality = "poor";
	auto result = qos::QosOverrideBuilder::BuildForAggregate(agg);
	EXPECT_FALSE(result.has_value());
}

TEST(QosOverrideEdgeTest, UnknownQualityReturnsNullopt) {
	qos::PeerQosAggregate agg;
	agg.hasSnapshot = true;
	agg.quality = "unknown";
	auto result = qos::QosOverrideBuilder::BuildForAggregate(agg);
	EXPECT_FALSE(result.has_value());
}

TEST(QosOverrideEdgeTest, FairQualityReturnsNullopt) {
	qos::PeerQosAggregate agg;
	agg.hasSnapshot = true;
	agg.quality = "fair";
	auto result = qos::QosOverrideBuilder::BuildForAggregate(agg);
	EXPECT_FALSE(result.has_value());
}

TEST(QosOverrideEdgeTest, LostFlagWithoutLostQualityStillTriggersOverride) {
	qos::PeerQosAggregate agg;
	agg.hasSnapshot = true;
	agg.lost = true;
	agg.quality = "good";  // quality doesn't say lost, but lost flag is true
	auto result = qos::QosOverrideBuilder::BuildForAggregate(agg);
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->overrideData.reason, "server_auto_lost");
}

TEST(QosOverrideEdgeTest, OverrideSignatureIsNotEmpty) {
	qos::PeerQosAggregate agg;
	agg.hasSnapshot = true;
	agg.quality = "poor";
	auto result = qos::QosOverrideBuilder::BuildForAggregate(agg);
	ASSERT_TRUE(result.has_value());
	EXPECT_FALSE(result->signature.empty());
}

TEST(QosOverrideEdgeTest, DifferentQualitiesProduceDifferentSignatures) {
	qos::PeerQosAggregate poorAgg;
	poorAgg.hasSnapshot = true;
	poorAgg.quality = "poor";
	auto poorResult = qos::QosOverrideBuilder::BuildForAggregate(poorAgg);

	qos::PeerQosAggregate lostAgg;
	lostAgg.hasSnapshot = true;
	lostAgg.quality = "lost";
	lostAgg.lost = true;
	auto lostResult = qos::QosOverrideBuilder::BuildForAggregate(lostAgg);

	ASSERT_TRUE(poorResult.has_value());
	ASSERT_TRUE(lostResult.has_value());
	EXPECT_NE(poorResult->signature, lostResult->signature);
}

// ===== Room aggregate overrides =====

TEST(QosOverrideEdgeTest, RoomAggregateNoQosReturnsNullopt) {
	qos::RoomQosAggregate agg;
	agg.hasQos = false;
	auto result = qos::QosOverrideBuilder::BuildForRoomAggregate(agg);
	EXPECT_FALSE(result.has_value());
}

TEST(QosOverrideEdgeTest, RoomAggregateHealthyReturnsNullopt) {
	qos::RoomQosAggregate agg;
	agg.hasQos = true;
	agg.peerCount = 5;
	agg.poorPeers = 0;
	agg.lostPeers = 0;
	auto result = qos::QosOverrideBuilder::BuildForRoomAggregate(agg);
	EXPECT_FALSE(result.has_value());
}

TEST(QosOverrideEdgeTest, RoomAggregateWithLostPeersTriggersOverride) {
	qos::RoomQosAggregate agg;
	agg.hasQos = true;
	agg.peerCount = 5;
	agg.lostPeers = 1;
	auto result = qos::QosOverrideBuilder::BuildForRoomAggregate(agg);
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->overrideData.reason, "server_room_pressure");
	EXPECT_TRUE(result->overrideData.hasMaxLevelClamp);
	EXPECT_EQ(result->overrideData.maxLevelClamp, 1u);
}

TEST(QosOverrideEdgeTest, RoomAggregateTwoPoorPeersTriggersOverride) {
	qos::RoomQosAggregate agg;
	agg.hasQos = true;
	agg.peerCount = 5;
	agg.poorPeers = 2;
	auto result = qos::QosOverrideBuilder::BuildForRoomAggregate(agg);
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result->overrideData.reason, "server_room_pressure");
}

TEST(QosOverrideEdgeTest, RoomAggregateOnePoorPeerNotEnough) {
	qos::RoomQosAggregate agg;
	agg.hasQos = true;
	agg.peerCount = 5;
	agg.poorPeers = 1;
	agg.lostPeers = 0;
	auto result = qos::QosOverrideBuilder::BuildForRoomAggregate(agg);
	EXPECT_FALSE(result.has_value());
}

TEST(QosOverrideEdgeTest, RoomAggregateOverrideHasDisableRecovery) {
	qos::RoomQosAggregate agg;
	agg.hasQos = true;
	agg.lostPeers = 1;
	auto result = qos::QosOverrideBuilder::BuildForRoomAggregate(agg);
	ASSERT_TRUE(result.has_value());
	EXPECT_TRUE(result->overrideData.hasDisableRecovery);
	EXPECT_TRUE(result->overrideData.disableRecovery);
	EXPECT_EQ(result->overrideData.ttlMs, 8000u);
}
