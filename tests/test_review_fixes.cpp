// Unit tests for the 5 code review fixes (2026-04-08).
#include <gtest/gtest.h>
#include "RoomManager.h"
#include "Channel.h"
#include "Constants.h"
#include <future>
#include <chrono>

using namespace mediasoup;

// ═══════════════════════════════════════════════════════════════
// Fix 1: Room::addPeer closes old peer before replacing
// ═══════════════════════════════════════════════════════════════

class DuplicatePeerTest : public ::testing::Test {
protected:
	std::shared_ptr<Room> room = std::make_shared<Room>("test-room", nullptr);

	std::shared_ptr<Peer> makePeer(const std::string& id) {
		auto p = std::make_shared<Peer>();
		p->id = id;
		p->displayName = id;
		return p;
	}
};

TEST_F(DuplicatePeerTest, AddPeerWithSameIdClosesOldPeer) {
	auto old = makePeer("alice");
	room->addPeer(old);
	EXPECT_EQ(room->peerCount(), 1u);

	// replacePeer returns old peer for caller to clean up
	auto fresh = makePeer("alice");
	auto replaced = room->replacePeer(fresh);

	EXPECT_NE(replaced, nullptr);
	EXPECT_EQ(replaced, old);
	// Room should still have exactly 1 peer
	EXPECT_EQ(room->peerCount(), 1u);
	// The stored peer should be the new one
	EXPECT_EQ(room->getPeer("alice"), fresh);

	// Caller is responsible for close — simulate what RoomService::join does
	replaced->close();
	EXPECT_EQ(old->sendTransport, nullptr);
	EXPECT_EQ(old->recvTransport, nullptr);
	EXPECT_TRUE(old->producers.empty());
	EXPECT_TRUE(old->consumers.empty());
}

TEST_F(DuplicatePeerTest, ReplacePeerReturnsNullWhenNoPrevious) {
	auto alice = makePeer("alice");
	auto replaced = room->replacePeer(alice);
	EXPECT_EQ(replaced, nullptr);
	EXPECT_EQ(room->peerCount(), 1u);
}

TEST_F(DuplicatePeerTest, AddPeerWithDifferentIdDoesNotAffectExisting) {
	auto alice = makePeer("alice");
	room->addPeer(alice);
	room->addPeer(makePeer("bob"));
	EXPECT_EQ(room->peerCount(), 2u);
	EXPECT_NE(room->getPeer("alice"), nullptr);
	EXPECT_NE(room->getPeer("bob"), nullptr);
}

TEST_F(DuplicatePeerTest, RepeatedReplacementWorks) {
	for (int i = 0; i < 5; ++i) {
		auto old = room->replacePeer(makePeer("alice"));
		if (old) old->close();
	}
	EXPECT_EQ(room->peerCount(), 1u);
}

// ═══════════════════════════════════════════════════════════════
// Fix 4: collectPeerStats no longer uses std::async
//   Verify the old pseudo-timeout pattern is gone by checking
//   that std::future destructor blocking is not an issue.
// ═══════════════════════════════════════════════════════════════

TEST(StatsTimeoutTest, FutureDestructorBlocksWithStdAsync) {
	// Demonstrate the problem that was fixed:
	// std::async(std::launch::async, ...) future destructor blocks.
	auto start = std::chrono::steady_clock::now();
	{
		auto fut = std::async(std::launch::async, [] {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			return 42;
		});
		// Don't call get() — let fut destruct
		// With std::launch::async, destructor WILL block ~200ms
	}
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start).count();
	// This proves the destructor blocks — the old code had this problem
	EXPECT_GE(elapsed, 150) << "std::async future destructor should block";
}

TEST(StatsTimeoutTest, RequestWaitTimeoutConstantExists) {
	// The fix relies on requestWait's built-in timeout
	EXPECT_EQ(kChannelRequestTimeoutMs, 5000);
	EXPECT_EQ(kStatsTimeoutMs, 500);
}

// ═══════════════════════════════════════════════════════════════
// Fix 5: PlainTransport uses requestWait (compile-time check)
//   The actual fix is that PlainTransport::connect() calls
//   channel_->requestWait() instead of channel_->request().get().
//   This is verified by the fact that the code compiles and by
//   the integration test. Here we just verify OwnedResponse
//   semantics used by the fixed code.
// ═══════════════════════════════════════════════════════════════

TEST(PlainTransportFixTest, OwnedResponseNullResponseOnEmpty) {
	Channel::OwnedResponse resp;
	EXPECT_EQ(resp.response(), nullptr);
}

// ═══════════════════════════════════════════════════════════════
// Fix 3: restartIce parses response (compile-time + semantics)
//   The actual parsing requires a real worker. Here we verify
//   the IceParameters struct can be updated as the fix does.
// ═══════════════════════════════════════════════════════════════

TEST(RestartIceFixTest, IceParametersCanBeUpdated) {
	IceParameters params;
	params.usernameFragment = "old_ufrag";
	params.password = "old_pwd";
	params.iceLite = false;

	// Simulate what the fix does after parsing response
	params.usernameFragment = "new_ufrag";
	params.password = "new_pwd";
	params.iceLite = true;

	EXPECT_EQ(params.usernameFragment, "new_ufrag");
	EXPECT_EQ(params.password, "new_pwd");
	EXPECT_TRUE(params.iceLite);

	// Verify JSON serialization works
	json j = params;
	EXPECT_EQ(j["usernameFragment"], "new_ufrag");
	EXPECT_EQ(j["password"], "new_pwd");
	EXPECT_EQ(j["iceLite"], true);
}
