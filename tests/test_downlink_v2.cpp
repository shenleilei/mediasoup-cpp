#include <gtest/gtest.h>
#include "qos/SubscriberBudgetAllocator.h"
#include "qos/ProducerDemandAggregator.h"
#include "qos/PublisherSupplyController.h"
#include "qos/RoomDownlinkPlanner.h"

using namespace mediasoup::qos;

// ── helpers ──

static DownlinkSubscription MakeSub(const std::string& cid, const std::string& pid,
	bool visible, bool pinned = false, bool screenShare = false,
	uint32_t tw = 640, uint32_t th = 480)
{
	DownlinkSubscription s;
	s.consumerId = cid;
	s.producerId = pid;
	s.visible = visible;
	s.pinned = pinned;
	s.isScreenShare = screenShare;
	s.targetWidth = tw;
	s.targetHeight = th;
	return s;
}

static DownlinkSnapshot MakeSnapshot(const std::string& peerId,
	double bw, std::vector<DownlinkSubscription> subs)
{
	DownlinkSnapshot snap;
	snap.subscriberPeerId = peerId;
	snap.availableIncomingBitrate = bw;
	snap.subscriptions = std::move(subs);
	return snap;
}

// ── SubscriberBudgetAllocator tests ──

TEST(SubscriberBudgetAllocatorTest, AllVisibleReceiveBaseLayerFirst) {
	SubscriberBudgetAllocator alloc;
	auto snap = MakeSnapshot("sub1", 200'000.0, {
		MakeSub("c1", "p1", true),
		MakeSub("c2", "p2", true),
		MakeSub("c3", "p3", true),
	});
	auto plan = alloc.Allocate(snap, 0);
	// All visible should get at least a resume + setLayers
	int resumeCount = 0;
	for (auto& a : plan.actions)
		if (a.type == DownlinkAction::Type::kResume) resumeCount++;
	EXPECT_EQ(resumeCount, 3);
}

TEST(SubscriberBudgetAllocatorTest, HiddenGetsPaused) {
	SubscriberBudgetAllocator alloc;
	auto snap = MakeSnapshot("sub1", 1'000'000.0, {
		MakeSub("c1", "p1", true),
		MakeSub("c2", "p2", false), // hidden
	});
	auto plan = alloc.Allocate(snap, 0);
	bool c2Paused = false;
	for (auto& a : plan.actions)
		if (a.consumerId == "c2" && a.type == DownlinkAction::Type::kPause) c2Paused = true;
	EXPECT_TRUE(c2Paused);
}

TEST(SubscriberBudgetAllocatorTest, PinnedGetsUpgradeBeforeGrid) {
	SubscriberBudgetAllocator alloc;
	// Enough budget for some upgrades but not all
	auto snap = MakeSnapshot("sub1", 500'000.0, {
		MakeSub("c1", "p1", true, true),  // pinned
		MakeSub("c2", "p2", true, false), // grid
	});
	auto plan = alloc.Allocate(snap, 0);
	uint8_t pinnedSpatial = 0, gridSpatial = 0;
	for (auto& a : plan.actions) {
		if (a.type == DownlinkAction::Type::kSetLayers) {
			if (a.consumerId == "c1") pinnedSpatial = a.spatialLayer;
			if (a.consumerId == "c2") gridSpatial = a.spatialLayer;
		}
	}
	EXPECT_GE(pinnedSpatial, gridSpatial);
}

TEST(SubscriberBudgetAllocatorTest, ScreenShareDominatesGridUnderBudget) {
	SubscriberBudgetAllocator alloc;
	auto snap = MakeSnapshot("sub1", 600'000.0, {
		MakeSub("c1", "p1", true, false, true),  // screenshare
		MakeSub("c2", "p2", true, false, false),  // grid
	});
	auto plan = alloc.Allocate(snap, 0);
	uint8_t ssSpatial = 0, gridSpatial = 0;
	for (auto& a : plan.actions) {
		if (a.type == DownlinkAction::Type::kSetLayers) {
			if (a.consumerId == "c1") ssSpatial = a.spatialLayer;
			if (a.consumerId == "c2") gridSpatial = a.spatialLayer;
		}
	}
	EXPECT_GE(ssSpatial, gridSpatial);
}

TEST(SubscriberBudgetAllocatorTest, DegradeLevelStillCapsBudgetAllocator) {
	SubscriberBudgetAllocator alloc;
	auto snap = MakeSnapshot("sub1", 5'000'000.0, {
		MakeSub("c1", "p1", true, true),
	});
	auto plan = alloc.Allocate(snap, 3); // max degrade
	for (auto& a : plan.actions) {
		if (a.type == DownlinkAction::Type::kSetLayers) {
			EXPECT_EQ(a.spatialLayer, 0);
			EXPECT_EQ(a.temporalLayer, 0);
		}
	}
}

// ── ProducerDemandAggregator tests ──

TEST(ProducerDemandAggregatorTest, MaxLayerComesFromHighestSubscriberDemand) {
	ProducerDemandAggregator agg;
	SubscriberBudgetAllocator alloc;

	// Sub1: high bandwidth -> gets high layer for p1
	auto snap1 = MakeSnapshot("sub1", 5'000'000.0, {
		MakeSub("c1", "p1", true, true),
	});
	auto plan1 = alloc.Allocate(snap1, 0);
	agg.addSubscriberPlan("sub1", snap1, plan1);

	// Sub2: low bandwidth -> gets low layer for p1
	auto snap2 = MakeSnapshot("sub2", 200'000.0, {
		MakeSub("c2", "p1", true),
	});
	auto plan2 = alloc.Allocate(snap2, 0);
	agg.addSubscriberPlan("sub2", snap2, plan2);

	auto demands = agg.finalize(1000, {});
	ASSERT_EQ(demands.size(), 1u);
	// Should take the max from sub1
	uint8_t maxS = 0;
	for (auto& a : plan1.actions)
		if (a.type == DownlinkAction::Type::kSetLayers) maxS = a.spatialLayer;
	EXPECT_EQ(demands[0].maxSpatialLayer, maxS);
}

TEST(ProducerDemandAggregatorTest, HiddenSubscriberDoesNotCreateVisibleDemand) {
	ProducerDemandAggregator agg;
	auto snap = MakeSnapshot("sub1", 1'000'000.0, {
		MakeSub("c1", "p1", false), // hidden
	});
	SubscriberBudgetAllocator alloc;
	auto plan = alloc.Allocate(snap, 0);
	agg.addSubscriberPlan("sub1", snap, plan);

	auto demands = agg.finalize(1000, {});
	// p1 should have zero visible subscribers
	for (auto& d : demands) {
		if (d.producerId == "p1")
			EXPECT_EQ(d.visibleSubscriberCount, 0u);
	}
}

TEST(ProducerDemandAggregatorTest, ZeroDemandStartsTimer) {
	ProducerDemandAggregator agg;
	auto snap = MakeSnapshot("sub1", 1'000'000.0, {
		MakeSub("c1", "p1", false), // hidden -> zero visible demand
	});
	SubscriberBudgetAllocator alloc;
	auto plan = alloc.Allocate(snap, 0);
	agg.addSubscriberPlan("sub1", snap, plan);

	int64_t now = 5000;
	auto demands = agg.finalize(now, {});
	for (auto& d : demands) {
		if (d.producerId == "p1") {
			EXPECT_EQ(d.zeroDemandSinceMs, now);
			EXPECT_GT(d.holdUntilMs, now);
		}
	}
}

TEST(ProducerDemandAggregatorTest, MissingProducerEntersZeroDemandHold) {
	ProducerDemandAggregator agg;
	std::unordered_map<std::string, ProducerDemandState> prev;
	prev["p1"] = ProducerDemandState{
		.producerId = "p1",
		.maxSpatialLayer = 2,
		.maxTemporalLayer = 2,
		.visibleSubscriberCount = 1,
	};

	int64_t now = 5000;
	auto demands = agg.finalize(now, prev);

	ASSERT_EQ(demands.size(), 1u);
	EXPECT_EQ(demands[0].producerId, "p1");
	EXPECT_EQ(demands[0].visibleSubscriberCount, 0u);
	EXPECT_EQ(demands[0].zeroDemandSinceMs, now);
	EXPECT_GT(demands[0].holdUntilMs, now);
}

TEST(ProducerDemandAggregatorTest, MissingProducerDropsAfterHoldExpires) {
	ProducerDemandAggregator agg;
	std::unordered_map<std::string, ProducerDemandState> prev;
	prev["p1"] = ProducerDemandState{
		.producerId = "p1",
		.zeroDemandSinceMs = 1000,
		.holdUntilMs = 3000,
	};

	auto demands = agg.finalize(4000, prev);
	EXPECT_TRUE(demands.empty());
}

// ── RoomDownlinkPlanner tests ──

TEST(RoomDownlinkPlannerTest, PlanRoomProducesSubscriberAndDemandPlans) {
	RoomDownlinkPlanner planner;
	std::vector<SubscriberInput> inputs;

	SubscriberInput s1;
	s1.peerId = "sub1";
	s1.snapshot = MakeSnapshot("sub1", 2'000'000.0, {
		MakeSub("c1", "p1", true, true),
		MakeSub("c2", "p2", true),
	});
	s1.degradeLevel = 0;
	inputs.push_back(std::move(s1));

	auto plan = planner.PlanRoom(inputs, 1000, {});
	EXPECT_EQ(plan.subscriberPlans.size(), 1u);
	EXPECT_FALSE(plan.producerDemands.empty());
}

// ── PublisherSupplyController tests ──

using ResolvePair = std::optional<std::pair<std::string, std::string>>;

TEST(PublisherSupplyControllerTest, ClampsWhenDemandBelowMax) {
	PublisherSupplyController ctrl;
	std::vector<ProducerDemandState> states;
	ProducerDemandState s;
	s.producerId = "p1";
	s.maxSpatialLayer = 1;
	s.visibleSubscriberCount = 2;
	s.supplyState = ProducerSupplyState::kLowClamped;
	states.push_back(s);

	auto overrides = ctrl.BuildOverrides(states,
		[](const std::string&) -> ResolvePair { return std::make_pair("alice", "track-1"); },
		1000);
	ASSERT_EQ(overrides.size(), 1u);
	EXPECT_EQ(overrides[0].targetPeerId, "alice");
	EXPECT_EQ(overrides[0].overrideData.scope, "track");
	EXPECT_EQ(overrides[0].overrideData.trackId, "track-1");
	EXPECT_TRUE(overrides[0].overrideData.hasMaxLevelClamp);
	EXPECT_EQ(overrides[0].overrideData.maxLevelClamp, 1u);
}

TEST(PublisherSupplyControllerTest, ClearsWhenFullDemandRestored) {
	PublisherSupplyController ctrl;
	std::vector<ProducerDemandState> states;
	ProducerDemandState s;
	s.producerId = "p1";
	s.maxSpatialLayer = 2;
	s.visibleSubscriberCount = 1;
	states.push_back(s);

	auto overrides = ctrl.BuildOverrides(states,
		[](const std::string&) -> ResolvePair { return std::make_pair("alice", "track-1"); },
		1000);
	ASSERT_EQ(overrides.size(), 1u);
	EXPECT_EQ(overrides[0].overrideData.reason, "downlink_v2_demand_restored");
}

TEST(PublisherSupplyControllerTest, SkipsWhenTrackIdUnresolved) {
	PublisherSupplyController ctrl;
	std::vector<ProducerDemandState> states;
	ProducerDemandState s;
	s.producerId = "p1";
	s.maxSpatialLayer = 1;
	s.visibleSubscriberCount = 1;
	states.push_back(s);

	auto overrides = ctrl.BuildOverrides(states,
		[](const std::string&) -> ResolvePair { return std::nullopt; },
		1000);
	EXPECT_TRUE(overrides.empty());
}

// ── base-layer admission ordering ──

TEST(SubscriberBudgetAllocatorTest, PinnedGetsBaseLayerBeforeGridUnderTightBudget) {
	SubscriberBudgetAllocator alloc;
	// Budget only enough for ~1 base layer
	auto snap = MakeSnapshot("sub1", 40'000.0, {
		MakeSub("c1", "p1", true, false), // grid — listed first
		MakeSub("c2", "p2", true, true),  // pinned — listed second but higher utility
	});
	auto plan = alloc.Allocate(snap, 0);
	bool c1Paused = false, c2Paused = false;
	bool c2Resumed = false;
	for (auto& a : plan.actions) {
		if (a.consumerId == "c1" && a.type == DownlinkAction::Type::kPause) c1Paused = true;
		if (a.consumerId == "c2" && a.type == DownlinkAction::Type::kPause) c2Paused = true;
		if (a.consumerId == "c2" && a.type == DownlinkAction::Type::kResume) c2Resumed = true;
	}
	// Pinned should get the base layer, grid should be dropped
	EXPECT_TRUE(c2Resumed) << "pinned should be admitted";
	EXPECT_FALSE(c2Paused) << "pinned should not be paused";
	// grid may or may not be admitted depending on exact cost, but pinned must win
}

// ── TTL refresh ──

TEST(PublisherSupplyControllerTest, ClampResendAfterTTLExpiry) {
	// This is a design-level test: verify that the same override can be
	// produced twice (the controller itself is stateless; the RoomService
	// dedup layer is what was broken).
	PublisherSupplyController ctrl;
	ProducerDemandState s;
	s.producerId = "p1";
	s.maxSpatialLayer = 1;
	s.visibleSubscriberCount = 1;
	s.supplyState = ProducerSupplyState::kLowClamped;

	auto r1 = ctrl.BuildOverrides({s},
		[](const std::string&) -> ResolvePair { return std::make_pair("alice", "t1"); }, 1000);
	auto r2 = ctrl.BuildOverrides({s},
		[](const std::string&) -> ResolvePair { return std::make_pair("alice", "t1"); }, 7000);
	ASSERT_EQ(r1.size(), 1u);
	ASSERT_EQ(r2.size(), 1u);
	// Both should produce identical overrides — the dedup is RoomService's job
	EXPECT_EQ(r1[0].overrideData.raw.dump(), r2[0].overrideData.raw.dump());
}

// ── v3 ProducerSupplyState tests ──

TEST(ProducerDemandAggregatorTest, ZeroDemandTransitionsToPausedAfterConfirm) {
	ProducerDemandAggregator agg;
	auto snap = MakeSnapshot("sub1", 1'000'000.0, {
		MakeSub("c1", "p1", false), // hidden
	});
	SubscriberBudgetAllocator alloc;
	auto plan = alloc.Allocate(snap, 0);
	agg.addSubscriberPlan("sub1", snap, plan);

	// First finalize: enters zero-demand hold
	int64_t t0 = 1000;
	auto d1 = agg.finalize(t0, {});
	ASSERT_EQ(d1.size(), 1u);
	EXPECT_EQ(d1[0].supplyState, ProducerSupplyState::kZeroDemandHold);

	// Build prev map
	std::unordered_map<std::string, ProducerDemandState> prev;
	prev[d1[0].producerId] = d1[0];

	// Second finalize at t0 + kPauseConfirmMs: should transition to paused
	ProducerDemandAggregator agg2;
	agg2.addSubscriberPlan("sub1", snap, plan);
	auto d2 = agg2.finalize(t0 + ProducerDemandAggregator::kPauseConfirmMs, prev);
	ASSERT_EQ(d2.size(), 1u);
	EXPECT_EQ(d2[0].supplyState, ProducerSupplyState::kPaused);
}

TEST(ProducerDemandAggregatorTest, PausedTransitionsToResumeWarmupOnDemand) {
	std::unordered_map<std::string, ProducerDemandState> prev;
	ProducerDemandState paused;
	paused.producerId = "p1";
	paused.supplyState = ProducerSupplyState::kPaused;
	paused.zeroDemandSinceMs = 1000;
	paused.pauseEligibleAtMs = 5000;
	prev["p1"] = paused;

	ProducerDemandAggregator agg;
	auto snap = MakeSnapshot("sub1", 1'000'000.0, {
		MakeSub("c1", "p1", true), // now visible
	});
	SubscriberBudgetAllocator alloc;
	auto plan = alloc.Allocate(snap, 0);
	agg.addSubscriberPlan("sub1", snap, plan);

	auto d = agg.finalize(10000, prev);
	ASSERT_EQ(d.size(), 1u);
	EXPECT_EQ(d[0].supplyState, ProducerSupplyState::kResumeWarmup);
	EXPECT_GT(d[0].resumeEligibleAtMs, 10000);
}

TEST(ProducerDemandAggregatorTest, ResumeWarmupCompletesToActive) {
	std::unordered_map<std::string, ProducerDemandState> prev;
	ProducerDemandState warmup;
	warmup.producerId = "p1";
	warmup.supplyState = ProducerSupplyState::kResumeWarmup;
	warmup.resumeEligibleAtMs = 11000;
	warmup.lastNonZeroDemandAtMs = 10000;
	prev["p1"] = warmup;

	ProducerDemandAggregator agg;
	auto snap = MakeSnapshot("sub1", 5'000'000.0, {
		MakeSub("c1", "p1", true, true),
	});
	SubscriberBudgetAllocator alloc;
	auto plan = alloc.Allocate(snap, 0);
	agg.addSubscriberPlan("sub1", snap, plan);

	// Before warmup completes
	auto d1 = agg.finalize(10500, prev);
	ASSERT_EQ(d1.size(), 1u);
	EXPECT_EQ(d1[0].supplyState, ProducerSupplyState::kResumeWarmup);

	// After warmup completes
	prev["p1"] = d1[0];
	ProducerDemandAggregator agg2;
	agg2.addSubscriberPlan("sub1", snap, plan);
	auto d2 = agg2.finalize(12000, prev);
	ASSERT_EQ(d2.size(), 1u);
	EXPECT_NE(d2[0].supplyState, ProducerSupplyState::kResumeWarmup);
}

TEST(ProducerDemandAggregatorTest, SingleVisibleFlickerDoesNotCausePause) {
	// Rapid hide/show should not reach paused state
	ProducerDemandAggregator agg;
	SubscriberBudgetAllocator alloc;

	// Start visible
	auto snapVisible = MakeSnapshot("sub1", 1'000'000.0, {
		MakeSub("c1", "p1", true),
	});
	auto planV = alloc.Allocate(snapVisible, 0);
	agg.addSubscriberPlan("sub1", snapVisible, planV);
	auto d1 = agg.finalize(1000, {});
	ASSERT_EQ(d1.size(), 1u);
	EXPECT_TRUE(d1[0].supplyState == ProducerSupplyState::kActive ||
		d1[0].supplyState == ProducerSupplyState::kLowClamped);

	// Hide briefly
	std::unordered_map<std::string, ProducerDemandState> prev;
	prev[d1[0].producerId] = d1[0];
	ProducerDemandAggregator agg2;
	auto snapHidden = MakeSnapshot("sub1", 1'000'000.0, {
		MakeSub("c1", "p1", false),
	});
	auto planH = alloc.Allocate(snapHidden, 0);
	agg2.addSubscriberPlan("sub1", snapHidden, planH);
	auto d2 = agg2.finalize(1100, prev); // 100ms later
	EXPECT_EQ(d2[0].supplyState, ProducerSupplyState::kZeroDemandHold);

	// Show again quickly — should go back to active, not paused
	prev[d2[0].producerId] = d2[0];
	ProducerDemandAggregator agg3;
	agg3.addSubscriberPlan("sub1", snapVisible, planV);
	auto d3 = agg3.finalize(1200, prev);
	ASSERT_EQ(d3.size(), 1u);
	EXPECT_NE(d3[0].supplyState, ProducerSupplyState::kPaused);
}

// ── v3 PublisherSupplyController pause/resume tests ──

TEST(PublisherSupplyControllerTest, PausedStateEmitsPauseUpstream) {
	PublisherSupplyController ctrl;
	ProducerDemandState s;
	s.producerId = "p1";
	s.supplyState = ProducerSupplyState::kPaused;
	s.zeroDemandSinceMs = 1000;

	auto overrides = ctrl.BuildOverrides({s},
		[](const std::string&) -> ResolvePair { return std::make_pair("alice", "track-1"); },
		6000);
	// Should emit clear (to remove old resume) + pause
	ASSERT_GE(overrides.size(), 2u);
	// First: clear old resume
	EXPECT_EQ(overrides[0].overrideData.reason, "downlink_v3_demand_resumed");
	EXPECT_EQ(overrides[0].overrideData.ttlMs, 0u);
	// Second: pause
	EXPECT_TRUE(overrides[1].overrideData.hasPauseUpstream);
	EXPECT_TRUE(overrides[1].overrideData.pauseUpstream);
	EXPECT_EQ(overrides[1].overrideData.reason, "downlink_v3_zero_demand_pause");
}

TEST(PublisherSupplyControllerTest, ResumeWarmupEmitsResumeUpstream) {
	PublisherSupplyController ctrl;
	ProducerDemandState s;
	s.producerId = "p1";
	s.supplyState = ProducerSupplyState::kResumeWarmup;
	s.visibleSubscriberCount = 1;
	s.maxSpatialLayer = 1;

	auto overrides = ctrl.BuildOverrides({s},
		[](const std::string&) -> ResolvePair { return std::make_pair("alice", "track-1"); },
		10000);
	// Should emit clear (to remove old pause) + resume
	ASSERT_GE(overrides.size(), 2u);
	// First: clear the old pause
	EXPECT_EQ(overrides[0].overrideData.reason, "downlink_v3_zero_demand_pause");
	EXPECT_EQ(overrides[0].overrideData.ttlMs, 0u);
	// Second: resume
	EXPECT_TRUE(overrides[1].overrideData.hasResumeUpstream);
	EXPECT_TRUE(overrides[1].overrideData.resumeUpstream);
	EXPECT_EQ(overrides[1].overrideData.reason, "downlink_v3_demand_resumed");
}

TEST(PublisherSupplyControllerTest, ZeroDemandHoldEmitsLowClamp) {
	PublisherSupplyController ctrl;
	ProducerDemandState s;
	s.producerId = "p1";
	s.supplyState = ProducerSupplyState::kZeroDemandHold;
	s.zeroDemandSinceMs = 1000;
	s.holdUntilMs = 3000;

	auto overrides = ctrl.BuildOverrides({s},
		[](const std::string&) -> ResolvePair { return std::make_pair("alice", "track-1"); },
		2000);
	ASSERT_EQ(overrides.size(), 1u);
	EXPECT_TRUE(overrides[0].overrideData.hasMaxLevelClamp);
	EXPECT_EQ(overrides[0].overrideData.maxLevelClamp, 0u);
	EXPECT_EQ(overrides[0].overrideData.reason, "downlink_v2_zero_demand_hold");
}
