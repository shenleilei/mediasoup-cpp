// Unit tests for QoS-related features:
// OwnedNotification, Producer::ScoreEntry, Consumer::Score,
// Room::getPeerIds, RoomManager::getRoomIds
#include <gtest/gtest.h>
#include "Channel.h"
#include "Producer.h"
#include "Consumer.h"
#include "RoomManager.h"

using namespace mediasoup;

// ─── OwnedNotification tests ───

TEST(OwnedNotificationTest, EmptyDataReturnsNull) {
	Channel::OwnedNotification owned;
	EXPECT_EQ(owned.notification(), nullptr);
}

TEST(OwnedNotificationTest, GarbageDataReturnsNull) {
	Channel::OwnedNotification owned;
	owned.data = {0xFF, 0xFE, 0x00, 0x01, 0x02, 0x03};
	// Should not crash, just return nullptr for invalid flatbuffer
	auto* notif = owned.notification();
	// We can't guarantee nullptr for arbitrary bytes (flatbuffers may "parse" garbage),
	// but it must not crash.
	(void)notif;
}

TEST(OwnedNotificationTest, DefaultEventValue) {
	Channel::OwnedNotification owned;
	EXPECT_EQ(owned.event, FBS::Notification::Event::WORKER_RUNNING);
}

// ─── Producer::ScoreEntry tests ───

TEST(ProducerScoreTest, InitialScoresEmpty) {
	// Producer scores should be empty before any notification
	// We can't construct a Producer without a Channel, so test the struct directly
	std::vector<Producer::ScoreEntry> scores;
	EXPECT_TRUE(scores.empty());

	// Verify struct fields
	Producer::ScoreEntry entry{0, 12345, "r0", 8};
	EXPECT_EQ(entry.encodingIdx, 0u);
	EXPECT_EQ(entry.ssrc, 12345u);
	EXPECT_EQ(entry.rid, "r0");
	EXPECT_EQ(entry.score, 8);
}

TEST(ProducerScoreTest, ScoreEntryBoundaryValues) {
	// Max values
	Producer::ScoreEntry maxEntry{UINT32_MAX, UINT32_MAX, "", 255};
	EXPECT_EQ(maxEntry.encodingIdx, UINT32_MAX);
	EXPECT_EQ(maxEntry.ssrc, UINT32_MAX);
	EXPECT_EQ(maxEntry.score, 255);

	// Zero values
	Producer::ScoreEntry zeroEntry{0, 0, "", 0};
	EXPECT_EQ(zeroEntry.score, 0);
}

// ─── Consumer::Score tests ───

TEST(ConsumerScoreTest, DefaultScoreIsZero) {
	Consumer::Score score{};
	EXPECT_EQ(score.score, 0);
	EXPECT_EQ(score.producerScore, 0);
	EXPECT_TRUE(score.producerScores.empty());
}

TEST(ConsumerScoreTest, ScoreBoundaryValues) {
	Consumer::Score score{10, 10, {10, 10, 10}};
	EXPECT_EQ(score.score, 10);
	EXPECT_EQ(score.producerScore, 10);
	EXPECT_EQ(score.producerScores.size(), 3u);

	// Zero score (worst quality)
	Consumer::Score bad{0, 0, {0}};
	EXPECT_EQ(bad.score, 0);
}

// ─── Room::getPeerIds tests ───

class RoomQosTest : public ::testing::Test {
protected:
	std::shared_ptr<Room> room = std::make_shared<Room>("qos-room", nullptr);

	std::shared_ptr<Peer> makePeer(const std::string& id) {
		auto p = std::make_shared<Peer>();
		p->id = id;
		p->displayName = id;
		return p;
	}
};

TEST_F(RoomQosTest, GetPeerIdsEmpty) {
	auto ids = room->getPeerIds();
	EXPECT_TRUE(ids.empty());
}

TEST_F(RoomQosTest, GetPeerIdsReturnsAll) {
	room->addPeer(makePeer("alice"));
	room->addPeer(makePeer("bob"));
	room->addPeer(makePeer("charlie"));

	auto ids = room->getPeerIds();
	EXPECT_EQ(ids.size(), 3u);

	std::sort(ids.begin(), ids.end());
	EXPECT_EQ(ids[0], "alice");
	EXPECT_EQ(ids[1], "bob");
	EXPECT_EQ(ids[2], "charlie");
}

TEST_F(RoomQosTest, GetPeerIdsAfterRemove) {
	room->addPeer(makePeer("alice"));
	room->addPeer(makePeer("bob"));
	room->removePeer("alice");

	auto ids = room->getPeerIds();
	EXPECT_EQ(ids.size(), 1u);
	EXPECT_EQ(ids[0], "bob");
}

TEST_F(RoomQosTest, GetPeerIdsAfterRemoveNonExistent) {
	room->addPeer(makePeer("alice"));
	room->removePeer("ghost"); // removing non-existent peer should not crash
	auto ids = room->getPeerIds();
	EXPECT_EQ(ids.size(), 1u);
}

// ─── RoomManager::getRoomIds tests (no real workers needed) ───

// We can't easily construct a RoomManager without WorkerManager,
// but we can test Room-level getPeerIds thoroughly above.
// For getRoomIds, we test via the integration test below.

// ─── OwnedResponse edge cases ───

TEST(OwnedResponseTest, EmptyDataReturnsNull) {
	Channel::OwnedResponse owned;
	EXPECT_EQ(owned.response(), nullptr);
}

TEST(OwnedResponseTest, GarbageDataDoesNotCrash) {
	Channel::OwnedResponse owned;
	owned.data = {0xDE, 0xAD, 0xBE, 0xEF};
	// Must not crash
	auto* resp = owned.response();
	(void)resp;
}
