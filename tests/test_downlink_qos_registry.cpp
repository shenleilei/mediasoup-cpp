#include <gtest/gtest.h>
#include "qos/DownlinkQosRegistry.h"

using namespace mediasoup::qos;

namespace {

DownlinkSnapshot MakeSnapshot(uint64_t seq, int64_t tsMs = 0) {
	DownlinkSnapshot snap;
	snap.schema = "mediasoup.qos.downlink.client.v1";
	snap.seq = seq;
	snap.tsMs = tsMs;
	snap.subscriberPeerId = "peer1";
	snap.availableIncomingBitrate = 1000000.0;
	snap.currentRoundTripTime = 0.05;
	return snap;
}

} // namespace

// --- Upsert and Get ---

TEST(DownlinkQosRegistryTest, UpsertAndGetBasic) {
	DownlinkQosRegistry registry;
	auto snap = MakeSnapshot(1, 1000);

	EXPECT_TRUE(registry.Upsert("room1", "alice", snap, 1000));

	auto* entry = registry.Get("room1", "alice");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->snapshot.seq, 1u);
	EXPECT_EQ(entry->receivedAtMs, 1000);
}

TEST(DownlinkQosRegistryTest, GetReturnsNullForUnknownRoom) {
	DownlinkQosRegistry registry;
	EXPECT_EQ(registry.Get("nonexistent", "alice"), nullptr);
}

TEST(DownlinkQosRegistryTest, GetReturnsNullForUnknownPeer) {
	DownlinkQosRegistry registry;
	auto snap = MakeSnapshot(1);
	registry.Upsert("room1", "alice", snap, 1000);
	EXPECT_EQ(registry.Get("room1", "bob"), nullptr);
}

// --- Sequence validation ---

TEST(DownlinkQosRegistryTest, RejectsStaleSeq) {
	DownlinkQosRegistry registry;
	auto snap = MakeSnapshot(10);
	EXPECT_TRUE(registry.Upsert("room1", "alice", snap, 1000));

	auto stale = MakeSnapshot(5);
	std::string reason;
	EXPECT_FALSE(registry.Upsert("room1", "alice", stale, 2000, &reason));
	EXPECT_EQ(reason, "stale-seq");
}

TEST(DownlinkQosRegistryTest, RejectsDuplicateSeq) {
	DownlinkQosRegistry registry;
	auto snap = MakeSnapshot(10);
	EXPECT_TRUE(registry.Upsert("room1", "alice", snap, 1000));

	std::string reason;
	EXPECT_FALSE(registry.Upsert("room1", "alice", snap, 2000, &reason));
	EXPECT_EQ(reason, "stale-seq");
}

TEST(DownlinkQosRegistryTest, AcceptsHigherSeq) {
	DownlinkQosRegistry registry;
	auto snap1 = MakeSnapshot(10);
	EXPECT_TRUE(registry.Upsert("room1", "alice", snap1, 1000));

	auto snap2 = MakeSnapshot(11);
	EXPECT_TRUE(registry.Upsert("room1", "alice", snap2, 2000));

	auto* entry = registry.Get("room1", "alice");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->snapshot.seq, 11u);
}

// --- Seq reset detection ---

TEST(DownlinkQosRegistryTest, SeqResetWithinThresholdRejected) {
	DownlinkQosRegistry registry;
	auto snap = MakeSnapshot(2000);
	EXPECT_TRUE(registry.Upsert("room1", "alice", snap, 1000));

	// Jump back by exactly 1000 → within threshold → rejected
	auto reset = MakeSnapshot(1000);
	std::string reason;
	EXPECT_FALSE(registry.Upsert("room1", "alice", reset, 2000, &reason));
	EXPECT_EQ(reason, "stale-seq");
}

TEST(DownlinkQosRegistryTest, SeqResetBeyondThresholdAccepted) {
	DownlinkQosRegistry registry;
	auto snap = MakeSnapshot(2001);
	EXPECT_TRUE(registry.Upsert("room1", "alice", snap, 1000));

	// Jump back by 1001 → exceeds threshold → accepted as reset
	auto reset = MakeSnapshot(1000);
	EXPECT_TRUE(registry.Upsert("room1", "alice", reset, 2000));

	auto* entry = registry.Get("room1", "alice");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->snapshot.seq, 1000u);
}

TEST(DownlinkQosRegistryTest, SeqResetFromZeroAccepted) {
	DownlinkQosRegistry registry;
	auto snap = MakeSnapshot(5000);
	EXPECT_TRUE(registry.Upsert("room1", "alice", snap, 1000));

	auto reset = MakeSnapshot(0);
	EXPECT_TRUE(registry.Upsert("room1", "alice", reset, 2000));
}

// --- ErasePeer ---

TEST(DownlinkQosRegistryTest, ErasePeerRemovesOnlyTargetPeer) {
	DownlinkQosRegistry registry;
	auto snap = MakeSnapshot(1);
	registry.Upsert("room1", "alice", snap, 1000);
	registry.Upsert("room1", "bob", snap, 1000);

	registry.ErasePeer("room1", "alice");
	EXPECT_EQ(registry.Get("room1", "alice"), nullptr);
	EXPECT_NE(registry.Get("room1", "bob"), nullptr);
}

TEST(DownlinkQosRegistryTest, ErasePeerCleansUpEmptyRoom) {
	DownlinkQosRegistry registry;
	auto snap = MakeSnapshot(1);
	registry.Upsert("room1", "alice", snap, 1000);

	registry.ErasePeer("room1", "alice");
	// Room should be cleaned up internally
	EXPECT_EQ(registry.Get("room1", "alice"), nullptr);
}

TEST(DownlinkQosRegistryTest, ErasePeerNonExistentRoomDoesNotCrash) {
	DownlinkQosRegistry registry;
	EXPECT_NO_THROW(registry.ErasePeer("nonexistent", "alice"));
}

TEST(DownlinkQosRegistryTest, ErasePeerNonExistentPeerDoesNotCrash) {
	DownlinkQosRegistry registry;
	auto snap = MakeSnapshot(1);
	registry.Upsert("room1", "alice", snap, 1000);
	EXPECT_NO_THROW(registry.ErasePeer("room1", "bob"));
	EXPECT_NE(registry.Get("room1", "alice"), nullptr);
}

// --- EraseRoom ---

TEST(DownlinkQosRegistryTest, EraseRoomRemovesAllPeers) {
	DownlinkQosRegistry registry;
	auto snap = MakeSnapshot(1);
	registry.Upsert("room1", "alice", snap, 1000);
	registry.Upsert("room1", "bob", snap, 1000);
	registry.Upsert("room2", "carol", snap, 1000);

	registry.EraseRoom("room1");
	EXPECT_EQ(registry.Get("room1", "alice"), nullptr);
	EXPECT_EQ(registry.Get("room1", "bob"), nullptr);
	EXPECT_NE(registry.Get("room2", "carol"), nullptr);
}

TEST(DownlinkQosRegistryTest, EraseRoomNonExistentDoesNotCrash) {
	DownlinkQosRegistry registry;
	EXPECT_NO_THROW(registry.EraseRoom("nonexistent"));
}

// --- Multi-room isolation ---

TEST(DownlinkQosRegistryTest, DifferentRoomsAreIsolated) {
	DownlinkQosRegistry registry;
	auto snap1 = MakeSnapshot(1);
	auto snap2 = MakeSnapshot(2);
	registry.Upsert("room1", "alice", snap1, 1000);
	registry.Upsert("room2", "alice", snap2, 2000);

	auto* entry1 = registry.Get("room1", "alice");
	auto* entry2 = registry.Get("room2", "alice");
	ASSERT_NE(entry1, nullptr);
	ASSERT_NE(entry2, nullptr);
	EXPECT_EQ(entry1->snapshot.seq, 1u);
	EXPECT_EQ(entry2->snapshot.seq, 2u);
}

// --- RejectReason nullptr is safe ---

TEST(DownlinkQosRegistryTest, UpsertWithNullRejectReasonIsSafe) {
	DownlinkQosRegistry registry;
	auto snap = MakeSnapshot(10);
	EXPECT_TRUE(registry.Upsert("room1", "alice", snap, 1000));

	auto stale = MakeSnapshot(5);
	EXPECT_FALSE(registry.Upsert("room1", "alice", stale, 2000, nullptr));
}
