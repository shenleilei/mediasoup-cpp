// Black-box integration tests for the 5 code review fixes (2026-04-08).
#include <gtest/gtest.h>
#include "TestHttpsClient.h"
#include "TestWsClient.h"
#include "TestProcessUtils.h"

static const int SFU_PORT = 14002;
static const std::string HOST = "127.0.0.1";

class ReviewFixIntegration : public ::testing::Test {
protected:
	TestSfuProcess sfu_;
	std::string testRoom_;

	void SetUp() override {
		testRoom_ = "fix_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

		ASSERT_TRUE(sfu_.start(SFU_PORT, {}, makeTestSfuLogPath("sfu_review_fix", SFU_PORT)))
			<< "failed to start review-fix SFU on port " << SFU_PORT
			<< ", log: " << sfu_.logPath();
	}

	void TearDown() override {
		(void)sfu_.stop();
	}

	static json rtpCaps() {
		return {
			{"codecs", {{
				{"mimeType", "audio/opus"}, {"kind", "audio"},
				{"clockRate", 48000}, {"channels", 2},
				{"preferredPayloadType", 100}
			}, {
				{"mimeType", "video/VP8"}, {"kind", "video"},
				{"clockRate", 90000},
				{"preferredPayloadType", 101}
			}}},
			{"headerExtensions", json::array()}
		};
	}

	struct JoinedClient {
		std::unique_ptr<TestWsClient> ws;
		std::string peerId;
		json joinData;
	};

	JoinedClient joinRoom(const std::string& roomId, const std::string& peerId) {
		JoinedClient c;
		c.peerId = peerId;
		c.ws = std::make_unique<TestWsClient>();
		EXPECT_TRUE(c.ws->connect(HOST, SFU_PORT));
		auto resp = c.ws->request("join", {
			{"roomId", roomId}, {"peerId", peerId},
			{"displayName", peerId}, {"rtpCapabilities", rtpCaps()}
		});
		EXPECT_TRUE(resp.value("ok", false)) << "join failed: " << resp.dump();
		if (resp.value("ok", false)) {
			c.joinData = resp["data"];
		}
		return c;
	}
};

// ═══════════════════════════════════════════════════════════════
// Fix 1: Duplicate peerId reconnect — old close must NOT kick new
// ═══════════════════════════════════════════════════════════════

TEST_F(ReviewFixIntegration, ReconnectEmitsPeerJoinedWithReconnectFlag) {
	auto alice = joinRoom(testRoom_, "alice");
	ASSERT_EQ(alice.joinData.value("joinMode", ""), "new-peer") << alice.joinData.dump();
	usleep(50000);
	auto bob = joinRoom(testRoom_, "bob");
	bob.ws->drainNotifications(); // clear peerJoined for alice

	// Alice reconnects
	auto alice2 = joinRoom(testRoom_, "alice");
	ASSERT_EQ(alice2.joinData.value("joinMode", ""), "replaced-session") << alice2.joinData.dump();
	usleep(300000);

	// Bob should get peerJoined with reconnect:true, not a new event name
	auto notifs = bob.ws->drainNotifications();
	bool gotReconnectJoin = false;
	int joinCount = 0;
	for (auto& n : notifs) {
		std::string method = n.value("method", "");
		if (method == "peerJoined" && n.contains("data") &&
			n["data"].value("peerId", "") == "alice") {
			joinCount++;
			if (n["data"].value("reconnect", false))
				gotReconnectJoin = true;
		}
	}
	EXPECT_EQ(joinCount, 1) << "Should get exactly one peerJoined for alice reconnect";
	EXPECT_TRUE(gotReconnectJoin) << "peerJoined should have reconnect:true";

	// alice2 should still work
	auto resp = alice2.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	EXPECT_TRUE(resp.value("ok", false));
}

TEST_F(ReviewFixIntegration, ReconnectSamePeerIdPreservesExistingTransport) {
	auto alice = joinRoom(testRoom_, "alice");
	ASSERT_EQ(alice.joinData.value("joinMode", ""), "new-peer") << alice.joinData.dump();

	auto createResp = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(createResp.value("ok", false)) << createResp.dump();
	const std::string transportId = createResp["data"]["id"];
	const std::string oldUfrag = createResp["data"]["iceParameters"]["usernameFragment"];

	auto alice2 = joinRoom(testRoom_, "alice");
	ASSERT_EQ(alice2.joinData.value("joinMode", ""), "replaced-session") << alice2.joinData.dump();
	usleep(300000);

	auto iceResp = alice2.ws->request("restartIce", {{"transportId", transportId}});
	ASSERT_TRUE(iceResp.value("ok", false))
		<< "reconnected session should still see old transport: " << iceResp.dump();
	ASSERT_TRUE(iceResp["data"].contains("iceParameters")) << iceResp.dump();
	EXPECT_NE(iceResp["data"]["iceParameters"]["usernameFragment"].get<std::string>(), oldUfrag);
}

TEST_F(ReviewFixIntegration, RejoinAfterServerCloseCreatesNewPeerAndOldTransportIsGone) {
	auto alice = joinRoom(testRoom_, "alice");
	ASSERT_EQ(alice.joinData.value("joinMode", ""), "new-peer") << alice.joinData.dump();

	auto createResp = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(createResp.value("ok", false)) << createResp.dump();
	const std::string oldTransportId = createResp["data"]["id"];

	alice.ws->close();
	usleep(500000);

	auto alice2 = joinRoom(testRoom_, "alice");
	ASSERT_EQ(alice2.joinData.value("joinMode", ""), "new-peer") << alice2.joinData.dump();

	auto oldIceResp = alice2.ws->request("restartIce", {{"transportId", oldTransportId}});
	EXPECT_FALSE(oldIceResp.value("ok", false))
		<< "old transport should be gone after server-side close cleanup: " << oldIceResp.dump();
	EXPECT_EQ(oldIceResp.value("error", ""), "transport not found") << oldIceResp.dump();
}

// ═══════════════════════════════════════════════════════════════
// Fix 1 (strengthened): Old ws is kicked and can't send requests
// ═══════════════════════════════════════════════════════════════

TEST_F(ReviewFixIntegration, ReconnectSamePeerIdDoesNotKickNew) {
	// Alice joins
	auto alice = joinRoom(testRoom_, "alice");
	usleep(50000);

	// Bob joins (observer)
	auto bob = joinRoom(testRoom_, "bob");
	bob.ws->waitNotification("peerJoined", 1000); // drain alice's join if any

	// Alice "reconnects" — new ws, same peerId
	auto alice2 = joinRoom(testRoom_, "alice");

	// Give server time to process join defer (which should kick old ws)
	usleep(300000);

	// Old alice connection should have been closed by server
	// Try sending a request on old ws — should fail or be ignored
	auto oldResp = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	}, 2000);
	// Old ws was kicked, so either timeout or error
	bool oldFailed = !oldResp.value("ok", false);
	EXPECT_TRUE(oldFailed)
		<< "Old ws should not be able to send control requests after reconnect: " << oldResp.dump();

	// Bob should NOT receive peerLeft for alice
	auto leftNotif = bob.ws->waitNotification("peerLeft", 1500);
	if (!leftNotif.empty() && leftNotif.contains("data")) {
		EXPECT_NE(leftNotif["data"].value("peerId", ""), "alice")
			<< "Old connection close should NOT kick the new alice session";
	}

	// Verify alice2 can still operate
	auto resp = alice2.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	EXPECT_TRUE(resp.value("ok", false))
		<< "New alice session should still work: " << resp.dump();
}

// Verify stale requests from old connection are rejected even without waiting
TEST_F(ReviewFixIntegration, ReconnectStaleRequestRejectedImmediately) {
	auto alice = joinRoom(testRoom_, "alice");

	// Alice creates a transport on the old session
	auto t1 = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(t1.value("ok", false));

	// Alice "reconnects" with same peerId — do NOT wait for defer/kick
	auto alice2 = joinRoom(testRoom_, "alice");

	// Immediately fire a request on the OLD connection (race window)
	auto staleResp = alice.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true}
	}, 2000);

	// Should fail: either timeout (ws already closed) or rejected by sessionId check
	EXPECT_FALSE(staleResp.value("ok", false))
		<< "Stale request on old session should be rejected: " << staleResp.dump();

	// New session must still work
	auto freshResp = alice2.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	EXPECT_TRUE(freshResp.value("ok", false))
		<< "New session should still work: " << freshResp.dump();
}

TEST_F(ReviewFixIntegration, RepeatedJoinOnSameSocketRejected) {
	TestWsClient ws;
	ASSERT_TRUE(ws.connect(HOST, SFU_PORT));

	auto firstJoin = ws.request("join", {
		{"roomId", testRoom_},
		{"peerId", "repeat_join_user"},
		{"displayName", "repeat_join_user"},
		{"rtpCapabilities", rtpCaps()}
	});
	ASSERT_TRUE(firstJoin.value("ok", false)) << firstJoin.dump();

	auto secondJoin = ws.request("join", {
		{"roomId", testRoom_ + "_other"},
		{"peerId", "repeat_join_user_other"},
		{"displayName", "repeat_join_user_other"},
		{"rtpCapabilities", rtpCaps()}
	});
	EXPECT_FALSE(secondJoin.value("ok", false))
		<< "Repeated join on same socket must be rejected: " << secondJoin.dump();
	EXPECT_NE(secondJoin.value("error", "").find("already joined"), std::string::npos)
		<< secondJoin.dump();

	// Original joined session remains usable.
	auto transportResp = ws.request("createWebRtcTransport", {
		{"producing", true},
		{"consuming", false}
	});
	EXPECT_TRUE(transportResp.value("ok", false))
		<< "Original session should remain usable after rejected rejoin: "
		<< transportResp.dump();
}

TEST_F(ReviewFixIntegration, EarlyCloseJoinDoesNotLeaveGhostParticipants) {
	auto observer = joinRoom(testRoom_, "observer");

	static constexpr int kGhostAttempts = 40;
	for (int i = 0; i < kGhostAttempts; ++i) {
		TestWsClient ghostWs;
		ASSERT_TRUE(ghostWs.connect(HOST, SFU_PORT));
		const std::string ghostPeerId = "ghost_" + std::to_string(i);
		ghostWs.sendRequest("join", {
			{"roomId", testRoom_},
			{"peerId", ghostPeerId},
			{"displayName", ghostPeerId},
			{"rtpCapabilities", rtpCaps()}
		});
		ghostWs.close();
	}

	// Allow worker/main-loop cleanup to settle.
	usleep(800000);

	TestWsClient inspector;
	ASSERT_TRUE(inspector.connect(HOST, SFU_PORT));
	auto inspectJoin = inspector.request("join", {
		{"roomId", testRoom_},
		{"peerId", "inspector"},
		{"displayName", "inspector"},
		{"rtpCapabilities", rtpCaps()}
	});
	ASSERT_TRUE(inspectJoin.value("ok", false)) << inspectJoin.dump();
	ASSERT_TRUE(inspectJoin.contains("data")) << inspectJoin.dump();
	ASSERT_TRUE(inspectJoin["data"].contains("participants")) << inspectJoin.dump();

	const auto& participants = inspectJoin["data"]["participants"];
	ASSERT_TRUE(participants.is_array()) << inspectJoin.dump();
	for (const auto& participant : participants) {
		const std::string participantPeerId = participant.value("peerId", "");
		EXPECT_FALSE(participantPeerId.rfind("ghost_", 0) == 0)
			<< "Ghost participant leaked after early-close join race: "
			<< inspectJoin.dump();
	}
}

// ═══════════════════════════════════════════════════════════════
// Fix 3: restartIce returns fresh ICE parameters
// ═══════════════════════════════════════════════════════════════

TEST_F(ReviewFixIntegration, RestartIceReturnsFreshParameters) {
	auto alice = joinRoom(testRoom_, "alice");

	// Create transport, capture original ICE params
	auto createResp = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(createResp.value("ok", false));
	std::string transportId = createResp["data"]["id"];
	auto origIce = createResp["data"]["iceParameters"];
	std::string origUfrag = origIce["usernameFragment"];
	std::string origPwd = origIce["password"];

	// Restart ICE
	auto iceResp = alice.ws->request("restartIce", {{"transportId", transportId}});
	ASSERT_TRUE(iceResp.value("ok", false)) << "restartIce failed: " << iceResp.dump();
	ASSERT_TRUE(iceResp["data"].contains("iceParameters"))
		<< "restartIce response missing iceParameters";

	auto newIce = iceResp["data"]["iceParameters"];
	std::string newUfrag = newIce["usernameFragment"];
	std::string newPwd = newIce["password"];

	// New credentials must differ from original
	EXPECT_NE(newUfrag, origUfrag)
		<< "restartIce should return new usernameFragment, not cached value";
	EXPECT_NE(newPwd, origPwd)
		<< "restartIce should return new password, not cached value";
}

// ═══════════════════════════════════════════════════════════════
// Fix 4: Stats collection doesn't block control plane
// ═══════════════════════════════════════════════════════════════

TEST_F(ReviewFixIntegration, StatsDoNotBlockJoin) {
	auto alice = joinRoom(testRoom_, "alice");

	// Request stats (exercises the fixed collectPeerStats path)
	// Stats may fail on unconnected transports — that's fine, we just care
	// that the control plane isn't blocked afterward.
	alice.ws->request("getStats", {}, 5000);

	// Immediately after stats, a new peer should be able to join quickly
	auto start = std::chrono::steady_clock::now();
	auto bob = joinRoom(testRoom_, "bob");
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start).count();

	// Join should complete well under 3 seconds (not blocked by stats)
	EXPECT_LT(elapsed, 3000) << "Join took too long — stats may be blocking control plane";
}

// ═══════════════════════════════════════════════════════════════
// Input validation: invalid roomId/peerId rejected at join
// ═══════════════════════════════════════════════════════════════

TEST_F(ReviewFixIntegration, InvalidRoomIdRejected) {
	TestWsClient ws;
	ASSERT_TRUE(ws.connect(HOST, SFU_PORT));
	json rtpCaps = {{"codecs", json::array()}, {"headerExtensions", json::array()}};

	// roomId with slash
	auto resp = ws.request("join", {
		{"roomId", "room/../../etc"}, {"peerId", "alice"},
		{"displayName", "alice"}, {"rtpCapabilities", rtpCaps}
	});
	EXPECT_FALSE(resp.value("ok", false));
	EXPECT_NE(resp.value("error", "").find("invalid"), std::string::npos)
		<< "Should reject invalid roomId: " << resp.dump();
}

TEST_F(ReviewFixIntegration, InvalidPeerIdRejected) {
	TestWsClient ws;
	ASSERT_TRUE(ws.connect(HOST, SFU_PORT));
	json rtpCaps = {{"codecs", json::array()}, {"headerExtensions", json::array()}};

	// peerId with space
	auto resp = ws.request("join", {
		{"roomId", "valid-room"}, {"peerId", "alice bob"},
		{"displayName", "alice"}, {"rtpCapabilities", rtpCaps}
	});
	EXPECT_FALSE(resp.value("ok", false));
	EXPECT_NE(resp.value("error", "").find("invalid"), std::string::npos)
		<< "Should reject invalid peerId: " << resp.dump();
}

TEST_F(ReviewFixIntegration, EmptyPeerIdRejected) {
	TestWsClient ws;
	ASSERT_TRUE(ws.connect(HOST, SFU_PORT));
	json rtpCaps = {{"codecs", json::array()}, {"headerExtensions", json::array()}};

	auto resp = ws.request("join", {
		{"roomId", "valid-room"}, {"peerId", ""},
		{"displayName", "alice"}, {"rtpCapabilities", rtpCaps}
	});
	EXPECT_FALSE(resp.value("ok", false))
		<< "Should reject empty peerId: " << resp.dump();
}

// ═══════════════════════════════════════════════════════════════
// Geo-aware resolve: /api/resolve accepts X-Forwarded-For
// ═══════════════════════════════════════════════════════════════

TEST_F(ReviewFixIntegration, ResolveAcceptsXForwardedFor) {
	// Single-node mode: resolve should return this node regardless of IP
	// but the endpoint should accept the header without error
	std::string response = testHttpsGetRaw(
		HOST,
		SFU_PORT,
		"/api/resolve?roomId=geo_test",
		"X-Forwarded-For: 36.110.147.0\r\n");
	ASSERT_FALSE(response.empty());

	// Should get 200 with JSON containing wsUrl
	EXPECT_NE(response.find("200"), std::string::npos) << response;
	EXPECT_NE(response.find("wsUrl"), std::string::npos) << response;
}
