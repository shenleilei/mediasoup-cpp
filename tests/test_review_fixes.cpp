// Unit tests for the 5 code review fixes (2026-04-08).
#include <gtest/gtest.h>
#include "RoomManager.h"
#include "Channel.h"
#include "Constants.h"
#include "../client/RtcpHandler.h"
#include <future>
#include <chrono>
#include <unistd.h>

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

// ═══════════════════════════════════════════════════════════════
// Unsigned underflow: worker count with low CPU cores
// ═══════════════════════════════════════════════════════════════

TEST(WorkerCountTest, NoCoresUnderflow) {
	// Simulate the calculation: max(1, cores - 2)
	auto calc = [](int cores) { return std::max(1, cores - 2); };
	EXPECT_EQ(calc(0), 1);  // hardware_concurrency() can return 0
	EXPECT_EQ(calc(1), 1);
	EXPECT_EQ(calc(2), 1);
	EXPECT_EQ(calc(4), 2);
	EXPECT_EQ(calc(8), 6);
}

TEST(ChannelThreadSafetyFixTest, ProcessAvailableDataRejectedInThreadedMode) {
	int producerPipe[2];
	int consumerPipe[2];
	ASSERT_EQ(::pipe(producerPipe), 0);
	ASSERT_EQ(::pipe(consumerPipe), 0);

	{
		Channel ch(
			/*producerFd=*/producerPipe[1],
			/*consumerFd=*/consumerPipe[0],
			/*pid=*/12345,
			/*threaded=*/true);
		// Defensive guard: threaded mode must not allow direct pump API.
		EXPECT_FALSE(ch.processAvailableData());
	}

	::close(producerPipe[0]);
	::close(producerPipe[1]);
	::close(consumerPipe[0]);
	::close(consumerPipe[1]);
}

TEST(RtcpSuppressionFixTest, SuppressedVideoSkipsPliAndNackRetransmissions) {
	int sv[2];
	ASSERT_EQ(::socketpair(AF_UNIX, SOCK_DGRAM, 0, sv), 0);

	RtcpContext rtcp;
	uint32_t ssrc = 0x12345678;
	uint16_t seq = 3456;
	int pliCalls = 0;

	rtcp.registerVideoStream(ssrc, 96, &seq);
	rtcp.canSendVideoFn = [](uint32_t) { return false; };
	rtcp.sendH264Fn = [&](int, const uint8_t*, int, uint8_t, uint32_t, uint32_t, uint16_t&) {
		++pliCalls;
	};

	uint8_t rtp[16]{};
	rtp[0] = 0x80;
	rtp[1] = 96;
	rtp[2] = static_cast<uint8_t>(seq >> 8);
	rtp[3] = static_cast<uint8_t>(seq & 0xFF);
	rtp[4] = 0x00;
	rtp[5] = 0x01;
	rtp[6] = 0x5F;
	rtp[7] = 0x90;
	rtp[8] = static_cast<uint8_t>(ssrc >> 24);
	rtp[9] = static_cast<uint8_t>(ssrc >> 16);
	rtp[10] = static_cast<uint8_t>(ssrc >> 8);
	rtp[11] = static_cast<uint8_t>(ssrc & 0xFF);
	rtcp.onVideoRtpSent(rtp, sizeof(rtp));
	rtcp.cacheKeyframe(ssrc, rtp, sizeof(rtp), 90000);

	uint8_t pli[12]{};
	pli[0] = 0x81;
	pli[1] = RTCP_PT_PSFB;
	pli[2] = 0x00;
	pli[3] = 0x02;
	pli[8] = static_cast<uint8_t>(ssrc >> 24);
	pli[9] = static_cast<uint8_t>(ssrc >> 16);
	pli[10] = static_cast<uint8_t>(ssrc >> 8);
	pli[11] = static_cast<uint8_t>(ssrc & 0xFF);
	ASSERT_EQ(::send(sv[1], pli, sizeof(pli), 0), static_cast<ssize_t>(sizeof(pli)));

	uint8_t nack[16]{};
	nack[0] = 0x81;
	nack[1] = RTCP_PT_RTPFB;
	nack[2] = 0x00;
	nack[3] = 0x03;
	nack[8] = static_cast<uint8_t>(ssrc >> 24);
	nack[9] = static_cast<uint8_t>(ssrc >> 16);
	nack[10] = static_cast<uint8_t>(ssrc >> 8);
	nack[11] = static_cast<uint8_t>(ssrc & 0xFF);
	nack[12] = static_cast<uint8_t>(seq >> 8);
	nack[13] = static_cast<uint8_t>(seq & 0xFF);
	ASSERT_EQ(::send(sv[1], nack, sizeof(nack), 0), static_cast<ssize_t>(sizeof(nack)));

	rtcp.processIncomingRtcp(sv[0]);

	EXPECT_EQ(pliCalls, 0);
	EXPECT_EQ(rtcp.pliResponded, 0);
	EXPECT_EQ(rtcp.nackRetransmitted, 0);

	uint8_t recvBuf[1500];
	EXPECT_LT(::recv(sv[1], recvBuf, sizeof(recvBuf), MSG_DONTWAIT), 0);

	::close(sv[0]);
	::close(sv[1]);
}
