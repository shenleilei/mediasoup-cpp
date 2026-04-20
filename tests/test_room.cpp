#include <gtest/gtest.h>
#include "Channel.h"
#include "Producer.h"
#include "RoomManager.h"
#include <future>
#include <unistd.h>

using namespace mediasoup;

// We can't create real Workers (they fork processes), so we test Room and
// RoomManager logic that doesn't require a Router.

// --- Room tests (using nullptr router where router isn't called) ---

class RoomTest : public ::testing::Test {
protected:
	std::shared_ptr<Room> room = std::make_shared<Room>("test-room", nullptr);

	std::shared_ptr<Peer> makePeer(const std::string& id) {
		auto p = std::make_shared<Peer>();
		p->id = id;
		p->displayName = id;
		return p;
	}
};

TEST_F(RoomTest, AddAndGetPeer) {
	auto peer = makePeer("alice");
	room->addPeer(peer);
	EXPECT_EQ(room->peerCount(), 1u);
	EXPECT_NE(room->getPeer("alice"), nullptr);
	EXPECT_EQ(room->getPeer("bob"), nullptr);
}

TEST_F(RoomTest, RemovePeer) {
	room->addPeer(makePeer("alice"));
	room->addPeer(makePeer("bob"));
	EXPECT_EQ(room->peerCount(), 2u);

	room->removePeer("alice");
	EXPECT_EQ(room->peerCount(), 1u);
	EXPECT_EQ(room->getPeer("alice"), nullptr);
	EXPECT_NE(room->getPeer("bob"), nullptr);
}

TEST_F(RoomTest, Empty) {
	EXPECT_TRUE(room->empty());
	room->addPeer(makePeer("alice"));
	EXPECT_FALSE(room->empty());
	room->removePeer("alice");
	EXPECT_TRUE(room->empty());
}

TEST_F(RoomTest, GetOtherPeers) {
	room->addPeer(makePeer("alice"));
	room->addPeer(makePeer("bob"));
	room->addPeer(makePeer("charlie"));

	auto others = room->getOtherPeers("alice");
	EXPECT_EQ(others.size(), 2u);
	for (auto& p : others) {
		EXPECT_NE(p->id, "alice");
	}
}

TEST_F(RoomTest, GetParticipants) {
	room->addPeer(makePeer("alice"));
	room->addPeer(makePeer("bob"));
	auto participants = room->getParticipants();
	EXPECT_EQ(participants.size(), 2u);
}

TEST_F(RoomTest, IsIdle) {
	// Empty room should be idle after 0 seconds
	EXPECT_TRUE(room->isIdle(0));

	// Room with peers is never idle
	room->addPeer(makePeer("alice"));
	EXPECT_FALSE(room->isIdle(0));

	// After removing peer, idle timer starts
	room->removePeer("alice");
	EXPECT_TRUE(room->isIdle(0));
	// Not idle if threshold is high enough
	EXPECT_FALSE(room->isIdle(9999));
}

TEST_F(RoomTest, RemovePeerDoesNotHoldRoomLockAcrossPeerClose) {
	int producerPipe[2];
	int consumerPipe[2];
	ASSERT_EQ(::pipe(producerPipe), 0);
	ASSERT_EQ(::pipe(consumerPipe), 0);

	Channel channel(
		/*producerFd=*/producerPipe[1],
		/*consumerFd=*/consumerPipe[0],
		/*pid=*/12345,
		/*threaded=*/false);

	auto alice = makePeer("alice");
	auto producer = std::make_shared<Producer>(
		"producer-1",
		"video",
		RtpParameters{},
		"simple",
		RtpParameters{},
		&channel,
		"transport-1");
	alice->producers[producer->id()] = producer;
	producer->emitter().once("@close", [room = room](const std::vector<std::any>&) {
		EXPECT_LE(room->peerCount(), 1u);
	});

	room->addPeer(alice);
	auto removed = std::async(std::launch::async, [&] {
		room->removePeer("alice");
	});

	EXPECT_EQ(removed.wait_for(std::chrono::milliseconds(500)), std::future_status::ready);
	removed.get();
	EXPECT_EQ(room->peerCount(), 0u);

	::close(producerPipe[0]);
	::close(consumerPipe[1]);
}

// --- Peer tests ---

TEST(PeerTest, ToJson) {
	auto peer = std::make_shared<Peer>();
	peer->id = "alice";
	peer->displayName = "Alice";

	auto j = peer->toJson();
	EXPECT_EQ(j["peerId"], "alice");
	EXPECT_EQ(j["displayName"], "Alice");
	EXPECT_TRUE(j["producers"].is_array());
	EXPECT_EQ(j["producers"].size(), 0u);
}

TEST(PeerTest, GetTransportReturnsNull) {
	Peer peer;
	peer.id = "alice";
	EXPECT_EQ(peer.getTransport("nonexistent"), nullptr);
}

TEST(PeerTest, CloseIdempotent) {
	Peer peer;
	peer.id = "alice";
	// Close with no transports should not crash
	peer.close();
	EXPECT_EQ(peer.sendTransport, nullptr);
	EXPECT_EQ(peer.recvTransport, nullptr);
	EXPECT_TRUE(peer.producers.empty());
	EXPECT_TRUE(peer.consumers.empty());
}
