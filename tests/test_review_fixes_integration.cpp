// Black-box integration tests for the 5 code review fixes (2026-04-08).
// Requires a running mediasoup-sfu + mediasoup-worker.
#include <gtest/gtest.h>
#include "TestRedisServer.h"
#include "TestWsClient.h"
#include "TestProcessUtils.h"
#include <signal.h>
#include <sys/wait.h>
#include <hiredis/hiredis.h>
#include <poll.h>

// Recv a full HTTP response (headers + body) with timeout.
static std::string recvHttp(int fd, int timeoutMs = 3000) {
	std::string data;
	char buf[4096];
	auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
	while (std::chrono::steady_clock::now() < deadline) {
		int remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
			deadline - std::chrono::steady_clock::now()).count();
		if (remaining <= 0) break;
		struct pollfd pfd{fd, POLLIN, 0};
		int pr = poll(&pfd, 1, remaining);
		if (pr <= 0) break;
		int n = ::recv(fd, buf, sizeof(buf) - 1, 0);
		if (n <= 0) break;
		data.append(buf, n);
		// Check if we have full HTTP response
		auto hdrEnd = data.find("\r\n\r\n");
		if (hdrEnd != std::string::npos) {
			auto cl = data.find("Content-Length:");
			if (cl != std::string::npos && cl < hdrEnd) {
				size_t bodyExpected = std::stoul(data.substr(cl + 15));
				if (data.size() >= hdrEnd + 4 + bodyExpected) break;
			} else {
				// No Content-Length, assume complete after headers + some body
				break;
			}
		}
	}
	return data;
}

static const int SFU_PORT = 14002;
static const std::string HOST = "127.0.0.1";

class ReviewFixIntegration : public ::testing::Test {
protected:
	pid_t sfuPid_ = -1;
	std::string testRoom_;

	void SetUp() override {
		testRoom_ = "fix_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(SFU_PORT) +
			" --workers=1"
			" --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1"
			" --listenIp=127.0.0.1"
			" --redisHost=0.0.0.0 --redisPort=1"
			" > /dev/null 2>&1 & echo $!";
		FILE* fp = popen(cmd.c_str(), "r");
		ASSERT_NE(fp, nullptr);
		char buf[64]{};
		fgets(buf, sizeof(buf), fp);
		pclose(fp);
		sfuPid_ = atoi(buf);
		ASSERT_GT(sfuPid_, 0);

		for (int i = 0; i < 50; ++i) {
			usleep(100000);
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(SFU_PORT);
			inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
			if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0) {
				::close(fd);
				usleep(200000);
				return;
			}
			::close(fd);
		}
		FAIL() << "SFU did not start within 5 seconds";
	}

	void TearDown() override {
		if (sfuPid_ > 0) {
			terminateSfuProcess(sfuPid_);
			for (int i = 0; i < 20; ++i) {
				usleep(50000);
				int fd = socket(AF_INET, SOCK_STREAM, 0);
				sockaddr_in addr{};
				addr.sin_family = AF_INET;
				addr.sin_port = htons(SFU_PORT);
				addr.sin_addr.s_addr = htonl(INADDR_ANY);
				int opt = 1;
				setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
				bool free = (bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
				::close(fd);
				if (free) return;
			}
		}
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
		return c;
	}
};

// ═══════════════════════════════════════════════════════════════
// Fix 1: Duplicate peerId reconnect — old close must NOT kick new
// ═══════════════════════════════════════════════════════════════

TEST_F(ReviewFixIntegration, ReconnectEmitsPeerJoinedWithReconnectFlag) {
	auto alice = joinRoom(testRoom_, "alice");
	usleep(50000);
	auto bob = joinRoom(testRoom_, "bob");
	bob.ws->drainNotifications(); // clear peerJoined for alice

	// Alice reconnects
	auto alice2 = joinRoom(testRoom_, "alice");
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
// Fix 5: PlainTransport connect timeout (indirect)
//   We can't easily trigger a PlainTransport timeout in black-box,
//   but we verify recording setup works (which uses PlainTransport::connect).
//   If the fix broke something, autoRecord would fail.
// ═══════════════════════════════════════════════════════════════

TEST_F(ReviewFixIntegration, ProduceTriggersRecordingWithoutHang) {
	auto alice = joinRoom(testRoom_, "alice");

	auto sendResp = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(sendResp.value("ok", false));

	json rtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 88888888}}}},
		{"mid", "0"}
	};

	// produce triggers autoRecord which calls PlainTransport::connect()
	// With the old code this could hang indefinitely; with the fix it has a timeout
	auto start = std::chrono::steady_clock::now();
	auto resp = alice.ws->request("produce", {
		{"transportId", sendResp["data"]["id"]}, {"kind", "audio"},
		{"rtpParameters", rtpParams}
	}, 8000);
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start).count();

	EXPECT_TRUE(resp.value("ok", false)) << "produce failed: " << resp.dump();
	EXPECT_LT(elapsed, 7000) << "produce+autoRecord took too long, possible PlainTransport hang";
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
// Geo-aware resolve: /api/resolve accepts clientIp
// ═══════════════════════════════════════════════════════════════

TEST_F(ReviewFixIntegration, ResolveAcceptsClientIp) {
	// Single-node mode: resolve should return this node regardless of clientIp
	// but the endpoint should accept the parameter without error
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SFU_PORT);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	ASSERT_EQ(::connect(fd, (sockaddr*)&addr, sizeof(addr)), 0);

	std::string req = "GET /api/resolve?roomId=geo_test&clientIp=36.110.147.0 HTTP/1.1\r\n"
		"Host: 127.0.0.1\r\n\r\n";
	::send(fd, req.data(), req.size(), 0);

	std::string response = recvHttp(fd);
	::close(fd);
	ASSERT_FALSE(response.empty());

	// Should get 200 with JSON containing wsUrl
	EXPECT_NE(response.find("200"), std::string::npos) << response;
	EXPECT_NE(response.find("wsUrl"), std::string::npos) << response;
}

// ═══════════════════════════════════════════════════════════════
// Geo-aware direct join: two nodes with different geo, verify
// that join on the "wrong" node returns redirect
// ═══════════════════════════════════════════════════════════════

static const int GEO_PORT_A = 14010;  // "杭州电信"
static const int GEO_PORT_B = 14011;  // "广州联通"

class GeoJoinTest : public ::testing::Test {
protected:
	pid_t pidA_ = -1, pidB_ = -1;
	std::string testRoom_;
	TestRedisServer redisServer_;

	static pid_t startSfu(int port, double lat, double lng, const std::string& isp, int redisPort) {
		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(port) +
			" --workers=1"
			" --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1"
			" --listenIp=127.0.0.1"
			" --lat=" + std::to_string(lat) +
			" --lng=" + std::to_string(lng) +
			" --isp=" + isp +
			" --redisHost=127.0.0.1"
			" --redisPort=" + std::to_string(redisPort) +
			" > /dev/null 2>&1 & echo $!";
		FILE* fp = popen(cmd.c_str(), "r");
		if (!fp) return -1;
		char buf[64]{};
		fgets(buf, sizeof(buf), fp);
		pclose(fp);
		return atoi(buf);
	}

	static bool waitForPort(int port, int timeoutMs = 5000) {
		for (int i = 0; i < timeoutMs / 100; ++i) {
			usleep(100000);
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
			if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0) {
				::close(fd);
				usleep(200000);
				return true;
			}
			::close(fd);
		}
		return false;
	}

	static void killAndWaitPort(pid_t pid, int port) {
		if (pid <= 0) return;
		terminateSfuProcess(pid);
		for (int i = 0; i < 20; ++i) {
			usleep(50000);
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			int opt = 1;
			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			bool free = (bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
			::close(fd);
			if (free) return;
		}
	}

	void SetUp() override {
		testRoom_ = "geo_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

		// Kill ALL stale test SFU processes to prevent Redis pollution
		system("pkill -f 'mediasoup-sfu.*--port=140' 2>/dev/null");
		for (int i = 0; i < 40; ++i) {
			if (system("pgrep -f 'mediasoup-sfu.*--port=140' >/dev/null 2>&1") != 0) break;
			usleep(50000);
		}
		usleep(200000);

		ASSERT_TRUE(redisServer_.start()) << redisServer_.failureMessage();

		// Clean stale room and node entries from Redis
		{
			redisContext* ctx = redisConnect("127.0.0.1", redisServer_.port());
			if (ctx && !ctx->err) {
				auto* r = (redisReply*)redisCommand(ctx, "KEYS sfu:room:geo_*");
				if (r && r->type == REDIS_REPLY_ARRAY)
					for (size_t i = 0; i < r->elements; i++)
						redisCommand(ctx, "DEL %s", r->element[i]->str);
				if (r) freeReplyObject(r);
				r = (redisReply*)redisCommand(ctx, "KEYS sfu:node:*");
				if (r && r->type == REDIS_REPLY_ARRAY)
					for (size_t i = 0; i < r->elements; i++)
						redisCommand(ctx, "DEL %s", r->element[i]->str);
				if (r) freeReplyObject(r);
				redisFree(ctx);
			}
		}

		// Node A: 杭州电信 (30.27, 120.15)
		pidA_ = startSfu(GEO_PORT_A, 30.27, 120.15, "电信", redisServer_.port());
		// Node B: 广州联通 (23.13, 113.26)
		pidB_ = startSfu(GEO_PORT_B, 23.13, 113.26, "联通", redisServer_.port());
		ASSERT_GT(pidA_, 0);
		ASSERT_GT(pidB_, 0);
		ASSERT_TRUE(waitForPort(GEO_PORT_A)) << "SFU-A (杭州) did not start";
		ASSERT_TRUE(waitForPort(GEO_PORT_B)) << "SFU-B (广州) did not start";
		// Wait for Redis heartbeat to register both nodes' geo info
		usleep(1500000);
	}

	void TearDown() override {
		killAndWaitPort(pidA_, GEO_PORT_A);
		killAndWaitPort(pidB_, GEO_PORT_B);
	}
};

// /api/resolve with Beijing Telecom clientIp should prefer 杭州电信 (Node A)
TEST_F(GeoJoinTest, ResolvePrefersSameIspNearest) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(GEO_PORT_A); // ask 杭州 node (has geo context)
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	ASSERT_EQ(::connect(fd, (sockaddr*)&addr, sizeof(addr)), 0);

	// Beijing Telecom IP → should route to 杭州电信 (port A), not 广州联通 (port B)
	std::string req = "GET /api/resolve?roomId=" + testRoom_ +
		"&clientIp=36.110.147.0 HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
	::send(fd, req.data(), req.size(), 0);

	std::string response = recvHttp(fd);
	::close(fd);
	ASSERT_FALSE(response.empty());

	EXPECT_NE(response.find("200"), std::string::npos) << response;
	bool hasPortA = response.find(std::to_string(GEO_PORT_A)) != std::string::npos;
	bool hasPortB = response.find(std::to_string(GEO_PORT_B)) != std::string::npos;
	EXPECT_TRUE(hasPortA) << "Beijing Telecom should route to 杭州电信, got: " << response;
	EXPECT_FALSE(hasPortB) << "Should NOT route to 广州联通, got: " << response;
}

// Direct join on 广州 node with Beijing Telecom peer should redirect to 杭州
TEST_F(GeoJoinTest, DirectJoinWithLoopbackIpSucceedsLocally) {
	TestWsClient ws;
	ASSERT_TRUE(ws.connect("127.0.0.1", GEO_PORT_B)); // connect to 广州

	json rtpCaps = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"kind", "audio"},
			{"clockRate", 48000}, {"channels", 2},
			{"preferredPayloadType", 100}
		}}},
		{"headerExtensions", json::array()}
	};

	auto resp = ws.request("join", {
		{"roomId", testRoom_ + "_direct"}, {"peerId", "loopback_user"},
		{"displayName", "test"}, {"rtpCapabilities", rtpCaps}
	});

	// Loopback IP has no valid geo → no geo redirect → must succeed locally
	EXPECT_TRUE(resp.value("ok", false))
		<< "Loopback IP should join locally without redirect, got: " << resp.dump();
	EXPECT_FALSE(resp.contains("redirect"))
		<< "Loopback IP should NOT trigger geo redirect, got: " << resp.dump();
}

// ═══════════════════════════════════════════════════════════════
// Country isolation: CN client should not be routed to US node
// ═══════════════════════════════════════════════════════════════

static const int ISO_PORT_CN = 18790;  // 中国节点
static const int ISO_PORT_US = 18791;  // 美国节点

class CountryIsolationTest : public ::testing::Test {
protected:
	pid_t pidCN_ = -1, pidUS_ = -1;
	std::string testRoom_;
	TestRedisServer redisServer_;

	static pid_t startSfu(int port, double lat, double lng,
		const std::string& isp, const std::string& country, bool isolation, int redisPort)
	{
		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(port) +
			" --workers=1"
			" --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1"
			" --listenIp=127.0.0.1"
			" --lat=" + std::to_string(lat) +
			" --lng=" + std::to_string(lng) +
			" --isp=" + isp +
			" --redisHost=127.0.0.1"
			" --redisPort=" + std::to_string(redisPort) +
			" '--country=" + country + "'" +
			(isolation ? " --countryIsolation" : " --noCountryIsolation") +
			" > /dev/null 2>&1 & echo $!";
		FILE* fp = popen(cmd.c_str(), "r");
		if (!fp) return -1;
		char buf[64]{};
		fgets(buf, sizeof(buf), fp);
		pclose(fp);
		return atoi(buf);
	}

	static bool waitForPort(int port, int timeoutMs = 5000) {
		for (int i = 0; i < timeoutMs / 100; ++i) {
			usleep(100000);
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
			if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0) {
				::close(fd);
				usleep(200000);
				return true;
			}
			::close(fd);
		}
		return false;
	}

	static void killAndWaitPort(pid_t pid, int port) {
		if (pid <= 0) return;
		terminateSfuProcess(pid);
		for (int i = 0; i < 20; ++i) {
			usleep(50000);
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			int opt = 1;
			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			bool free = (bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
			::close(fd);
			if (free) return;
		}
	}

	void SetUp() override {
		testRoom_ = "iso_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

		ASSERT_TRUE(redisServer_.start()) << redisServer_.failureMessage();

		// Clean stale room and node entries from other geo tests
		{
			redisContext* ctx = redisConnect("127.0.0.1", redisServer_.port());
			if (ctx && !ctx->err) {
				auto* r = (redisReply*)redisCommand(ctx, "KEYS sfu:room:iso_*");
				if (r && r->type == REDIS_REPLY_ARRAY) {
					for (size_t i = 0; i < r->elements; i++)
						redisCommand(ctx, "DEL %s", r->element[i]->str);
					freeReplyObject(r);
				}
				r = (redisReply*)redisCommand(ctx, "KEYS sfu:node:*");
				if (r && r->type == REDIS_REPLY_ARRAY) {
					for (size_t i = 0; i < r->elements; i++)
						redisCommand(ctx, "DEL %s", r->element[i]->str);
					freeReplyObject(r);
				}
				redisFree(ctx);
			}
		}

		pidCN_ = startSfu(ISO_PORT_CN, 30.27, 120.15, "电信", "中国", true, redisServer_.port());
		pidUS_ = startSfu(ISO_PORT_US, 37.39, -122.08, "Amazon", "United States", true, redisServer_.port());
		ASSERT_GT(pidCN_, 0);
		ASSERT_GT(pidUS_, 0);
		ASSERT_TRUE(waitForPort(ISO_PORT_CN)) << "CN node did not start";
		ASSERT_TRUE(waitForPort(ISO_PORT_US)) << "US node did not start";
		usleep(1500000); // wait for Redis heartbeat
	}

	void TearDown() override {
		killAndWaitPort(pidCN_, ISO_PORT_CN);
		killAndWaitPort(pidUS_, ISO_PORT_US);
	}
};

// Chinese client asking US node should be routed to CN node, not US
TEST_F(CountryIsolationTest, ChinaClientOnUsNodeRoutedToChinaNode) {
	// Connect to US node, but with Chinese IP
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(ISO_PORT_US);  // ask US node
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	ASSERT_EQ(::connect(fd, (sockaddr*)&addr, sizeof(addr)), 0);

	std::string req = "GET /api/resolve?roomId=" + testRoom_ +
		"&clientIp=36.110.147.0 HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
	::send(fd, req.data(), req.size(), 0);

	std::string response = recvHttp(fd);
	::close(fd);
	ASSERT_FALSE(response.empty());

	bool hasCN = response.find(std::to_string(ISO_PORT_CN)) != std::string::npos;
	bool hasUS = response.find(std::to_string(ISO_PORT_US)) != std::string::npos;
	EXPECT_TRUE(hasCN) << "CN client on US node should be redirected to CN node, got: " << response;
	EXPECT_FALSE(hasUS) << "CN client should NOT stay on US node, got: " << response;
}

// US client asking CN node should be routed to US node, not CN
TEST_F(CountryIsolationTest, UsClientOnCnNodeRoutedToUsNode) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(ISO_PORT_CN);  // ask CN node
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	ASSERT_EQ(::connect(fd, (sockaddr*)&addr, sizeof(addr)), 0);

	std::string req = "GET /api/resolve?roomId=" + testRoom_ + "_us2" +
		"&clientIp=8.8.8.8 HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
	::send(fd, req.data(), req.size(), 0);

	std::string response = recvHttp(fd);
	::close(fd);
	ASSERT_FALSE(response.empty());

	bool hasUS = response.find(std::to_string(ISO_PORT_US)) != std::string::npos;
	bool hasCN = response.find(std::to_string(ISO_PORT_CN)) != std::string::npos;
	EXPECT_TRUE(hasUS) << "US client on CN node should be redirected to US node, got: " << response;
	EXPECT_FALSE(hasCN) << "US client should NOT stay on CN node, got: " << response;
}

// Chinese client should be routed to CN node when asking CN node (baseline)
TEST_F(CountryIsolationTest, ChinaClientRoutedToChinaNode) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(ISO_PORT_CN);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	ASSERT_EQ(::connect(fd, (sockaddr*)&addr, sizeof(addr)), 0);

	std::string req = "GET /api/resolve?roomId=" + testRoom_ +
		"&clientIp=36.110.147.0 HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
	::send(fd, req.data(), req.size(), 0);

	std::string response = recvHttp(fd);
	::close(fd);
	ASSERT_FALSE(response.empty());

	bool hasCN = response.find(std::to_string(ISO_PORT_CN)) != std::string::npos;
	bool hasUS = response.find(std::to_string(ISO_PORT_US)) != std::string::npos;
	EXPECT_TRUE(hasCN) << "CN client should route to CN node, got: " << response;
	EXPECT_FALSE(hasUS) << "CN client should NOT route to US node, got: " << response;
}

// US client should only be routed to US node, not CN node
TEST_F(CountryIsolationTest, UsClientRoutedToUsNode) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(ISO_PORT_US);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	ASSERT_EQ(::connect(fd, (sockaddr*)&addr, sizeof(addr)), 0);

	std::string req = "GET /api/resolve?roomId=" + testRoom_ + "_us" +
		"&clientIp=8.8.8.8 HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
	::send(fd, req.data(), req.size(), 0);

	std::string response = recvHttp(fd);
	::close(fd);
	ASSERT_FALSE(response.empty());

	bool hasUS = response.find(std::to_string(ISO_PORT_US)) != std::string::npos;
	bool hasCN = response.find(std::to_string(ISO_PORT_CN)) != std::string::npos;
	EXPECT_TRUE(hasUS) << "US client should route to US node, got: " << response;
	EXPECT_FALSE(hasCN) << "US client should NOT route to CN node, got: " << response;
}

// ═══════════════════════════════════════════════════════════════
// Redis degradation: SFU with bad Redis should still work locally
// ═══════════════════════════════════════════════════════════════

static const int DEGRADE_PORT = 18795;

class RedisDegradeTest : public ::testing::Test {
protected:
	pid_t sfuPid_ = -1;
	std::string testRoom_;

	void SetUp() override {
		testRoom_ = "degrade_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

		// Start SFU pointing to a non-existent Redis (port 1 = unreachable)
		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(DEGRADE_PORT) +
			" --workers=1"
			" --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1"
			" --listenIp=127.0.0.1"
			" --redisHost=127.0.0.1 --redisPort=1"
			" > /dev/null 2>&1 & echo $!";
		FILE* fp = popen(cmd.c_str(), "r");
		ASSERT_NE(fp, nullptr);
		char buf[64]{};
		fgets(buf, sizeof(buf), fp);
		pclose(fp);
		sfuPid_ = atoi(buf);
		ASSERT_GT(sfuPid_, 0);

		for (int i = 0; i < 50; ++i) {
			usleep(100000);
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(DEGRADE_PORT);
			inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
			if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0) {
				::close(fd);
				usleep(200000);
				return;
			}
			::close(fd);
		}
		FAIL() << "SFU did not start";
	}

	void TearDown() override {
		if (sfuPid_ > 0) {
			terminateSfuProcess(sfuPid_);
			for (int i = 0; i < 20; ++i) {
				usleep(50000);
				int fd = socket(AF_INET, SOCK_STREAM, 0);
				sockaddr_in addr{};
				addr.sin_family = AF_INET;
				addr.sin_port = htons(DEGRADE_PORT);
				addr.sin_addr.s_addr = htonl(INADDR_ANY);
				int opt = 1;
				setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
				bool free = (bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
				::close(fd);
				if (free) return;
			}
		}
	}

	static json rtpCaps() {
		return {
			{"codecs", {{
				{"mimeType", "audio/opus"}, {"kind", "audio"},
				{"clockRate", 48000}, {"channels", 2},
				{"preferredPayloadType", 100}
			}}},
			{"headerExtensions", json::array()}
		};
	}
};

// /api/resolve should return a valid response even with Redis down
TEST_F(RedisDegradeTest, ResolveWorksWithoutRedis) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(DEGRADE_PORT);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	ASSERT_EQ(::connect(fd, (sockaddr*)&addr, sizeof(addr)), 0);

	std::string req = "GET /api/resolve?roomId=" + testRoom_ +
		" HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
	::send(fd, req.data(), req.size(), 0);

	std::string response = recvHttp(fd);
	::close(fd);
	ASSERT_FALSE(response.empty());

	EXPECT_NE(response.find("200"), std::string::npos) << "Should return 200, got: " << response;
	EXPECT_NE(response.find("wsUrl"), std::string::npos) << "Should contain wsUrl, got: " << response;
}

// Direct join should succeed locally even with Redis down
TEST_F(RedisDegradeTest, JoinWorksWithoutRedis) {
	TestWsClient ws;
	ASSERT_TRUE(ws.connect("127.0.0.1", DEGRADE_PORT));

	auto resp = ws.request("join", {
		{"roomId", testRoom_}, {"peerId", "alice"},
		{"displayName", "alice"}, {"rtpCapabilities", rtpCaps()}
	});
	ASSERT_TRUE(resp.value("ok", false))
		<< "Join should succeed locally when Redis is down, got: " << resp.dump();

	// Should also be able to create transport
	auto transport = ws.request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	EXPECT_TRUE(transport.value("ok", false))
		<< "createTransport should work after degraded join, got: " << transport.dump();
}

// ═══════════════════════════════════════════════════════════════
// Local cache + pub/sub: verify cache-based routing works
// ═══════════════════════════════════════════════════════════════

static const int CACHE_PORT_A = 18796;
static const int CACHE_PORT_B = 18797;

class CacheTest : public ::testing::Test {
protected:
	pid_t pidA_ = -1, pidB_ = -1;
	std::string testRoom_;
	TestRedisServer redisServer_;

	static pid_t startSfu(int port, int redisPort, const std::string& extraArgs = "") {
		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(port) +
			" --workers=1"
			" --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1"
			" --listenIp=127.0.0.1"
			" --redisHost=127.0.0.1"
			" --redisPort=" + std::to_string(redisPort) +
			" --noCountryIsolation"
			" " + extraArgs +
			" > /dev/null 2>&1 & echo $!";
		FILE* fp = popen(cmd.c_str(), "r");
		if (!fp) return -1;
		char buf[64]{};
		fgets(buf, sizeof(buf), fp);
		pclose(fp);
		return atoi(buf);
	}

	static bool waitForPort(int port, int timeoutMs = 5000) {
		for (int i = 0; i < timeoutMs / 100; ++i) {
			usleep(100000);
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
			if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0) {
				::close(fd);
				usleep(200000);
				return true;
			}
			::close(fd);
		}
		return false;
	}

	static void killAndWaitPort(pid_t pid, int port) {
		if (pid <= 0) return;
		terminateSfuProcess(pid);
		for (int i = 0; i < 20; ++i) {
			usleep(50000);
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			int opt = 1;
			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			bool free = (bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
			::close(fd);
			if (free) return;
		}
	}

	static json rtpCaps() {
		return {
			{"codecs", {{
				{"mimeType", "audio/opus"}, {"kind", "audio"},
				{"clockRate", 48000}, {"channels", 2},
				{"preferredPayloadType", 100}
			}}},
			{"headerExtensions", json::array()}
		};
	}

	void SetUp() override {
		testRoom_ = "cache_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

		ASSERT_TRUE(redisServer_.start()) << redisServer_.failureMessage();

		// Clean Redis
		{
			redisContext* ctx = redisConnect("127.0.0.1", redisServer_.port());
			if (ctx && !ctx->err) {
				auto* r = (redisReply*)redisCommand(ctx, "KEYS sfu:node:*");
				if (r && r->type == REDIS_REPLY_ARRAY)
					for (size_t i = 0; i < r->elements; i++)
						redisCommand(ctx, "DEL %s", r->element[i]->str);
				if (r) freeReplyObject(r);
				r = (redisReply*)redisCommand(ctx, "KEYS sfu:room:cache_*");
				if (r && r->type == REDIS_REPLY_ARRAY)
					for (size_t i = 0; i < r->elements; i++)
						redisCommand(ctx, "DEL %s", r->element[i]->str);
				if (r) freeReplyObject(r);
				redisFree(ctx);
			}
		}

		pidA_ = startSfu(CACHE_PORT_A, redisServer_.port(), "--lat=30.27 --lng=120.15 --isp=电信");
		pidB_ = startSfu(CACHE_PORT_B, redisServer_.port(), "--lat=23.13 --lng=113.26 --isp=联通");
		ASSERT_GT(pidA_, 0);
		ASSERT_GT(pidB_, 0);
		ASSERT_TRUE(waitForPort(CACHE_PORT_A));
		ASSERT_TRUE(waitForPort(CACHE_PORT_B));
		// Wait for heartbeat + pub/sub sync
		usleep(2000000);
	}

	void TearDown() override {
		killAndWaitPort(pidA_, CACHE_PORT_A);
		killAndWaitPort(pidB_, CACHE_PORT_B);
	}
};

// Room claimed on node A should be visible to node B via pub/sub cache
TEST_F(CacheTest, RoomClaimPropagatesViaCache) {
	// Alice joins on node A → claims room
	TestWsClient alice;
	ASSERT_TRUE(alice.connect("127.0.0.1", CACHE_PORT_A));
	auto joinResp = alice.request("join", {
		{"roomId", testRoom_}, {"peerId", "alice"},
		{"displayName", "alice"}, {"rtpCapabilities", rtpCaps()}
	});
	ASSERT_TRUE(joinResp.value("ok", false)) << joinResp.dump();

	// Wait for pub/sub propagation
	usleep(500000);

	// Node B should know about this room via cache → resolve returns node A
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CACHE_PORT_B);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	ASSERT_EQ(::connect(fd, (sockaddr*)&addr, sizeof(addr)), 0);

	std::string req = "GET /api/resolve?roomId=" + testRoom_ +
		" HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
	::send(fd, req.data(), req.size(), 0);

	std::string response = recvHttp(fd);
	::close(fd);
	ASSERT_FALSE(response.empty());

	// Should point to node A (where room was claimed)
	EXPECT_NE(response.find(std::to_string(CACHE_PORT_A)), std::string::npos)
		<< "Node B should resolve room to node A via cache, got: " << response;
}

// Second resolve for same room should be fast (cache hit, no Redis)
TEST_F(CacheTest, ResolveUsesCache) {
	// Claim room on node A
	TestWsClient alice;
	ASSERT_TRUE(alice.connect("127.0.0.1", CACHE_PORT_A));
	auto joinResp = alice.request("join", {
		{"roomId", testRoom_}, {"peerId", "alice"},
		{"displayName", "alice"}, {"rtpCapabilities", rtpCaps()}
	});
	ASSERT_TRUE(joinResp.value("ok", false));
	usleep(500000);

	// Resolve twice on node A — second should be cache hit
	auto resolve = [&]() {
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(CACHE_PORT_A);
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
		::connect(fd, (sockaddr*)&addr, sizeof(addr));
		std::string req = "GET /api/resolve?roomId=" + testRoom_ +
			" HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
		::send(fd, req.data(), req.size(), 0);
		std::string result = recvHttp(fd);
		::close(fd);
		return result;
	};

	auto start = std::chrono::steady_clock::now();
	std::string r1 = resolve();
	std::string r2 = resolve();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start).count();

	// Both should return node A
	EXPECT_NE(r1.find(std::to_string(CACHE_PORT_A)), std::string::npos);
	EXPECT_NE(r2.find(std::to_string(CACHE_PORT_A)), std::string::npos);
	// Two resolves should be fast (< 500ms total, cache hit)
	EXPECT_LT(elapsed, 500) << "Cached resolves should be fast";
}

// Node B sees node A via pub/sub — direct join on B redirects to A for existing room
TEST_F(CacheTest, DirectJoinRedirectsViaCachedRoom) {
	// Alice joins on node A
	TestWsClient alice;
	ASSERT_TRUE(alice.connect("127.0.0.1", CACHE_PORT_A));
	auto joinResp = alice.request("join", {
		{"roomId", testRoom_}, {"peerId", "alice"},
		{"displayName", "alice"}, {"rtpCapabilities", rtpCaps()}
	});
	ASSERT_TRUE(joinResp.value("ok", false));
	usleep(500000);

	// Bob tries to join same room on node B → should redirect to node A
	TestWsClient bob;
	ASSERT_TRUE(bob.connect("127.0.0.1", CACHE_PORT_B));
	auto bobResp = bob.request("join", {
		{"roomId", testRoom_}, {"peerId", "bob"},
		{"displayName", "bob"}, {"rtpCapabilities", rtpCaps()}
	});

	bool redirected = bobResp.contains("redirect");
	// Room is on node A, so join on node B MUST redirect (via cache or EVAL)
	EXPECT_TRUE(redirected)
		<< "Join on wrong node should redirect, got: " << bobResp.dump();
	if (redirected) {
		std::string redirect = bobResp["redirect"].get<std::string>();
		EXPECT_NE(redirect.find(std::to_string(CACHE_PORT_A)), std::string::npos)
			<< "Should redirect to node A, got: " << redirect;
	}
}

TEST_F(CacheTest, StaleCachedRedirectFallsBackToLocalClaimWhenNodeKeyMissing) {
	TestWsClient alice;
	ASSERT_TRUE(alice.connect("127.0.0.1", CACHE_PORT_A));
	auto joinResp = alice.request("join", {
		{"roomId", testRoom_}, {"peerId", "alice"},
		{"displayName", "alice"}, {"rtpCapabilities", rtpCaps()}
	});
	ASSERT_TRUE(joinResp.value("ok", false)) << joinResp.dump();
	usleep(500000);

	redisContext* ctx = redisConnect("127.0.0.1", redisServer_.port());
	ASSERT_NE(ctx, nullptr);
	ASSERT_FALSE(ctx->err);

	auto* owner = (redisReply*)redisCommand(
		ctx,
		"GET sfu:room:%s",
		testRoom_.c_str());
	ASSERT_NE(owner, nullptr);
	ASSERT_EQ(owner->type, REDIS_REPLY_STRING);
	ASSERT_NE(owner->str, nullptr);
	const std::string ownerNodeId(owner->str, owner->len);
	freeReplyObject(owner);

	auto* del = (redisReply*)redisCommand(
		ctx,
		"DEL sfu:node:%s",
		ownerNodeId.c_str());
	ASSERT_NE(del, nullptr);
	freeReplyObject(del);
	redisFree(ctx);

	TestWsClient bob;
	ASSERT_TRUE(bob.connect("127.0.0.1", CACHE_PORT_B));
	auto bobResp = bob.request("join", {
		{"roomId", testRoom_}, {"peerId", "bob"},
		{"displayName", "bob"}, {"rtpCapabilities", rtpCaps()}
	});

	EXPECT_TRUE(bobResp.value("ok", false))
		<< "Stale cached redirect should be revalidated instead of sent to a missing node: "
		<< bobResp.dump();
	EXPECT_FALSE(bobResp.contains("redirect")) << bobResp.dump();
}

// ═══════════════════════════════════════════════════════════════
// Full node can still redirect to existing rooms on other nodes
// ═══════════════════════════════════════════════════════════════

static const int FULL_PORT_A = 18798;
static const int FULL_PORT_B = 18799;

class FullNodeRedirectTest : public ::testing::Test {
protected:
	pid_t pidA_ = -1, pidB_ = -1;
	std::string testRoom_;
	TestRedisServer redisServer_;

	static pid_t startSfu(int port, int redisPort, int maxRouters = 0) {
		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(port) +
			" --workers=1"
			" --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1"
			" --listenIp=127.0.0.1"
			" --redisHost=127.0.0.1"
			" --redisPort=" + std::to_string(redisPort) +
			" --noCountryIsolation";
		if (maxRouters > 0)
			cmd += " --maxRoutersPerWorker=" + std::to_string(maxRouters);
		cmd += " > /dev/null 2>&1 & echo $!";
		FILE* fp = popen(cmd.c_str(), "r");
		if (!fp) return -1;
		char buf[64]{};
		fgets(buf, sizeof(buf), fp);
		pclose(fp);
		return atoi(buf);
	}

	static bool waitForPort(int port, int timeoutMs = 5000) {
		for (int i = 0; i < timeoutMs / 100; ++i) {
			usleep(100000);
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
			if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0) {
				::close(fd);
				usleep(200000);
				return true;
			}
			::close(fd);
		}
		return false;
	}

	static void killAndWaitPort(pid_t pid, int port) {
		if (pid <= 0) return;
		terminateSfuProcess(pid);
		for (int i = 0; i < 20; ++i) {
			usleep(50000);
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			int opt = 1;
			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			bool free = (bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
			::close(fd);
			if (free) return;
		}
	}

	static json rtpCaps() {
		return {
			{"codecs", {{
				{"mimeType", "audio/opus"}, {"kind", "audio"},
				{"clockRate", 48000}, {"channels", 2},
				{"preferredPayloadType", 100}
			}}},
			{"headerExtensions", json::array()}
		};
	}

	void SetUp() override {
		testRoom_ = "full_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
		ASSERT_TRUE(redisServer_.start()) << redisServer_.failureMessage();
		{
				redisContext* ctx = redisConnect("127.0.0.1", redisServer_.port());
				if (ctx && !ctx->err) {
					auto* r = (redisReply*)redisCommand(ctx, "KEYS sfu:node:*");
					if (r && r->type == REDIS_REPLY_ARRAY)
						for (size_t i = 0; i < r->elements; i++)
							redisCommand(ctx, "DEL %s", r->element[i]->str);
				if (r) freeReplyObject(r);
				redisFree(ctx);
			}
		}
		// Node A: normal capacity
		pidA_ = startSfu(FULL_PORT_A, redisServer_.port());
		// Node B: maxRoutersPerWorker=1 (will be full after 1 room)
		pidB_ = startSfu(FULL_PORT_B, redisServer_.port(), 1);
		ASSERT_GT(pidA_, 0);
		ASSERT_GT(pidB_, 0);
		ASSERT_TRUE(waitForPort(FULL_PORT_A));
		ASSERT_TRUE(waitForPort(FULL_PORT_B));
		usleep(2000000);
	}

	void TearDown() override {
		killAndWaitPort(pidA_, FULL_PORT_A);
		killAndWaitPort(pidB_, FULL_PORT_B);
	}
};

TEST_F(FullNodeRedirectTest, FullNodeRedirectsToExistingRoom) {
	// Create a room on node A
	TestWsClient alice;
	ASSERT_TRUE(alice.connect("127.0.0.1", FULL_PORT_A));
	auto aliceJoin = alice.request("join", {
		{"roomId", testRoom_}, {"peerId", "alice"},
		{"displayName", "alice"}, {"rtpCapabilities", rtpCaps()}
	});
	ASSERT_TRUE(aliceJoin.value("ok", false)) << aliceJoin.dump();
	usleep(500000);

	// Fill node B with a different room so it's at capacity
	TestWsClient filler;
	ASSERT_TRUE(filler.connect("127.0.0.1", FULL_PORT_B));
	auto fillerJoin = filler.request("join", {
		{"roomId", testRoom_ + "_filler"}, {"peerId", "filler"},
		{"displayName", "filler"}, {"rtpCapabilities", rtpCaps()}
	});
	// filler may succeed or redirect — either way node B should be full now
	usleep(500000);

	// Bob tries to join alice's room on full node B → should redirect to node A
	TestWsClient bob;
	ASSERT_TRUE(bob.connect("127.0.0.1", FULL_PORT_B));
	auto bobJoin = bob.request("join", {
		{"roomId", testRoom_}, {"peerId", "bob"},
		{"displayName", "bob"}, {"rtpCapabilities", rtpCaps()}
	});

	// Must redirect to node A (room exists there), not reject with "no capacity"
	EXPECT_TRUE(bobJoin.contains("redirect"))
		<< "Full node should redirect to existing room, not reject. Got: " << bobJoin.dump();
	if (bobJoin.contains("redirect")) {
		EXPECT_NE(bobJoin["redirect"].get<std::string>().find(std::to_string(FULL_PORT_A)),
			std::string::npos)
			<< "Should redirect to node A, got: " << bobJoin["redirect"].get<std::string>();
	}
}
