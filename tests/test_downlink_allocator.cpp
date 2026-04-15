#include <gtest/gtest.h>
#include "qos/DownlinkAllocator.h"

using namespace mediasoup::qos;

TEST(DownlinkAllocatorTest, HiddenPausesConsumer) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = false
	}};
	auto actions = DownlinkAllocator::Compute(subs, {false});
	// Expect: pause + setPriority(1)
	ASSERT_GE(actions.size(), 1u);
	EXPECT_EQ(actions[0].type, DownlinkAction::Type::kPause);
	EXPECT_EQ(actions[0].consumerId, "c1");
	// Find setPriority
	bool hasPriority = false;
	for (auto& a : actions)
		if (a.type == DownlinkAction::Type::kSetPriority) {
			EXPECT_EQ(a.priority, DownlinkAllocator::kPriorityHidden);
			hasPriority = true;
		}
	EXPECT_TRUE(hasPriority);
}

TEST(DownlinkAllocatorTest, AlreadyPausedHiddenOnlySetsPriority) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = false
	}};
	auto actions = DownlinkAllocator::Compute(subs, {true});
	// No pause (already paused), just setPriority
	ASSERT_EQ(actions.size(), 1u);
	EXPECT_EQ(actions[0].type, DownlinkAction::Type::kSetPriority);
	EXPECT_EQ(actions[0].priority, DownlinkAllocator::kPriorityHidden);
}

TEST(DownlinkAllocatorTest, PinnedGetsHigherLayerAndPriority) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .pinned = true, .targetWidth = 1280
	}};
	auto actions = DownlinkAllocator::Compute(subs, {false});
	bool hasLayers = false, hasPriority = false;
	for (auto& a : actions) {
		if (a.type == DownlinkAction::Type::kSetLayers) {
			EXPECT_EQ(a.spatialLayer, 2);
			EXPECT_EQ(a.temporalLayer, 2);
			hasLayers = true;
		}
		if (a.type == DownlinkAction::Type::kSetPriority) {
			EXPECT_EQ(a.priority, DownlinkAllocator::kPriorityPinned);
			hasPriority = true;
		}
	}
	EXPECT_TRUE(hasLayers);
	EXPECT_TRUE(hasPriority);
}

TEST(DownlinkAllocatorTest, SmallTileSelectsLowLayer) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .pinned = false, .targetWidth = 160
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

TEST(DownlinkAllocatorTest, ResumeFromPausedIncludesKeyFrame) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .pinned = true, .targetWidth = 960
	}};
	auto actions = DownlinkAllocator::Compute(subs, {true});
	ASSERT_GE(actions.size(), 2u);
	EXPECT_EQ(actions[0].type, DownlinkAction::Type::kResume);
	EXPECT_EQ(actions[1].type, DownlinkAction::Type::kSetLayers);
	EXPECT_TRUE(actions[1].requestKeyFrame);
}

TEST(DownlinkAllocatorTest, ScreenShareGetsHighestPriority) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .isScreenShare = true, .targetWidth = 200
	}};
	auto actions = DownlinkAllocator::Compute(subs, {false});
	bool found = false;
	for (auto& a : actions)
		if (a.type == DownlinkAction::Type::kSetPriority) {
			EXPECT_EQ(a.priority, DownlinkAllocator::kPriorityScreenShare);
			found = true;
		}
	EXPECT_TRUE(found);
}

TEST(DownlinkAllocatorTest, ActiveSpeakerPriority) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .activeSpeaker = true, .targetWidth = 640
	}};
	auto actions = DownlinkAllocator::Compute(subs, {false});
	bool found = false;
	for (auto& a : actions)
		if (a.type == DownlinkAction::Type::kSetPriority) {
			EXPECT_EQ(a.priority, DownlinkAllocator::kPriorityActiveSpeaker);
			found = true;
		}
	EXPECT_TRUE(found);
}

TEST(DownlinkAllocatorTest, PriorityOrderingCorrect) {
	// screenshare > pinned > activeSpeaker > visible > hidden
	EXPECT_GT(DownlinkAllocator::kPriorityScreenShare, DownlinkAllocator::kPriorityPinned);
	EXPECT_GT(DownlinkAllocator::kPriorityPinned, DownlinkAllocator::kPriorityActiveSpeaker);
	EXPECT_GT(DownlinkAllocator::kPriorityActiveSpeaker, DownlinkAllocator::kPriorityVisibleGrid);
	EXPECT_GT(DownlinkAllocator::kPriorityVisibleGrid, DownlinkAllocator::kPriorityHidden);
}

TEST(DownlinkAllocatorTest, MixedPriorityAllocation) {
	std::vector<DownlinkSubscription> subs = {
		{.consumerId = "screen", .visible = true, .isScreenShare = true, .targetWidth = 1920},
		{.consumerId = "pinned", .visible = true, .pinned = true, .targetWidth = 960},
		{.consumerId = "hidden", .visible = false},
	};
	auto actions = DownlinkAllocator::Compute(subs, {false, false, false});

	std::unordered_map<std::string, uint8_t> priorities;
	for (auto& a : actions)
		if (a.type == DownlinkAction::Type::kSetPriority)
			priorities[a.consumerId] = a.priority;

	EXPECT_EQ(priorities["screen"], DownlinkAllocator::kPriorityScreenShare);
	EXPECT_EQ(priorities["pinned"], DownlinkAllocator::kPriorityPinned);
	EXPECT_EQ(priorities["hidden"], DownlinkAllocator::kPriorityHidden);
	EXPECT_GT(priorities["screen"], priorities["pinned"]);
	EXPECT_GT(priorities["pinned"], priorities["hidden"]);
}

// ── ComputeDiff tests ──

TEST(DownlinkAllocatorTest, ComputeDiffSuppressesRedundantActions) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .pinned = true, .targetWidth = 1280
	}};
	std::unordered_map<std::string, ConsumerLastState> lastState;

	// First call — everything is new, so all actions should be emitted.
	auto first = DownlinkAllocator::ComputeDiff(subs, {false}, 0, lastState);
	EXPECT_FALSE(first.empty());

	// Second call with identical input — should emit nothing.
	auto second = DownlinkAllocator::ComputeDiff(subs, {false}, 0, lastState);
	EXPECT_TRUE(second.empty());
}

TEST(DownlinkAllocatorTest, ComputeDiffEmitsOnStateChange) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .pinned = true, .targetWidth = 1280
	}};
	std::unordered_map<std::string, ConsumerLastState> lastState;

	// Seed the state.
	DownlinkAllocator::ComputeDiff(subs, {false}, 0, lastState);

	// Change degradeLevel from 0 → 2 — layers should change.
	auto actions = DownlinkAllocator::ComputeDiff(subs, {false}, 2, lastState);
	bool hasLayers = false;
	for (auto& a : actions)
		if (a.type == DownlinkAction::Type::kSetLayers) hasLayers = true;
	EXPECT_TRUE(hasLayers);
}

TEST(DownlinkAllocatorTest, ComputeDiffEmitsKeyFrameOnResume) {
	std::vector<DownlinkSubscription> subs = {{
		.consumerId = "c1", .visible = true, .pinned = true, .targetWidth = 960
	}};
	std::unordered_map<std::string, ConsumerLastState> lastState;
	// Seed as paused.
	lastState["c1"] = {.paused = true, .spatialLayer = 0, .temporalLayer = 0, .priority = 1};

	auto actions = DownlinkAllocator::ComputeDiff(subs, {true}, 0, lastState);
	bool hasResume = false;
	for (auto& a : actions)
		if (a.type == DownlinkAction::Type::kResume) hasResume = true;
	EXPECT_TRUE(hasResume);
}

TEST(DownlinkAllocatorTest, ComputeBudgetDiffEmitsKeyFrameOnResume) {
	std::vector<DownlinkAction> planActions = {
		{DownlinkAction::Type::kResume, "c1"},
		{DownlinkAction::Type::kSetLayers, "c1", 0, 0, false, 0},
		{DownlinkAction::Type::kSetPriority, "c1", 0, 0, false, DownlinkAllocator::kPriorityPinned},
	};
	std::unordered_map<std::string, ConsumerLastState> lastState;
	lastState["c1"] = {.paused = true, .spatialLayer = 0, .temporalLayer = 0, .priority = 1};

	auto actions = DownlinkAllocator::ComputeBudgetDiff(planActions, lastState);

	bool hasResume = false;
	bool hasKeyFrameLayers = false;
	for (auto& a : actions) {
		if (a.type == DownlinkAction::Type::kResume) hasResume = true;
		if (a.type == DownlinkAction::Type::kSetLayers && a.requestKeyFrame)
			hasKeyFrameLayers = true;
	}
	EXPECT_TRUE(hasResume);
	EXPECT_TRUE(hasKeyFrameLayers);
}

TEST(DownlinkAllocatorTest, ComputeBudgetDiffTracksLayerSwitchMetrics) {
	std::vector<DownlinkAction> planActions = {
		{DownlinkAction::Type::kSetLayers, "c1", 1, 0, false, 0},
		{DownlinkAction::Type::kSetPriority, "c1", 0, 0, false, DownlinkAllocator::kPriorityPinned},
	};
	std::unordered_map<std::string, ConsumerLastState> lastState;
	lastState["c1"] = {
		.paused = false,
		.spatialLayer = 0,
		.temporalLayer = 0,
		.priority = DownlinkAllocator::kPriorityPinned,
		.hasLayerState = true,
		.lastLayerChangeAtMs = 1000,
	};

	DownlinkAllocator::ComputeBudgetDiff(planActions, lastState, 2000);

	ASSERT_TRUE(lastState.count("c1") > 0);
	EXPECT_EQ(lastState["c1"].layerSwitchCount, 1u);
	EXPECT_EQ(lastState["c1"].flappingEvents, 1u);
	EXPECT_EQ(lastState["c1"].lastLayerChangeAtMs, 2000);
	EXPECT_EQ(lastState["c1"].lastUpgradeAtMs, 2000);
}
