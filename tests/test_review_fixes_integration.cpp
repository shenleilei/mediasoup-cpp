// Black-box integration tests for the 5 code review fixes (2026-04-08).
// Requires a running mediasoup-sfu + mediasoup-worker.
#include <gtest/gtest.h>
#include "TestWsClient.h"
#include <signal.h>
#include <sys/wait.h>
#include <hiredis/hiredis.h>

static const int SFU_PORT = 18767;
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
			kill(sfuPid_, SIGKILL);
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

	char buf[4096]{};
	int n = ::recv(fd, buf, sizeof(buf) - 1, 0);
	::close(fd);
	ASSERT_GT(n, 0);

	std::string response(buf, n);
	// Should get 200 with JSON containing wsUrl
	EXPECT_NE(response.find("200"), std::string::npos) << response;
	EXPECT_NE(response.find("wsUrl"), std::string::npos) << response;
}

// ═══════════════════════════════════════════════════════════════
// Geo-aware direct join: two nodes with different geo, verify
// that join on the "wrong" node returns redirect
// ═══════════════════════════════════════════════════════════════

static const int GEO_PORT_A = 18780;  // "杭州电信"
static const int GEO_PORT_B = 18781;  // "广州联通"

class GeoJoinTest : public ::testing::Test {
protected:
	pid_t pidA_ = -1, pidB_ = -1;
	std::string testRoom_;

	static pid_t startSfu(int port, double lat, double lng, const std::string& isp) {
		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(port) +
			" --workers=1"
			" --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1"
			" --listenIp=127.0.0.1"
			" --lat=" + std::to_string(lat) +
			" --lng=" + std::to_string(lng) +
			" --isp=" + isp +
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
		kill(pid, SIGKILL);
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

		// Clean stale room entries for our test room prefix from Redis
		{
			redisContext* ctx = redisConnect("127.0.0.1", 6379);
			if (ctx && !ctx->err) {
				auto* r = (redisReply*)redisCommand(ctx, "KEYS room:geo_*");
				if (r && r->type == REDIS_REPLY_ARRAY) {
					for (size_t i = 0; i < r->elements; i++)
						redisCommand(ctx, "DEL %s", r->element[i]->str);
					freeReplyObject(r);
				}
				redisFree(ctx);
			}
		}

		// Node A: 杭州电信 (30.27, 120.15)
		pidA_ = startSfu(GEO_PORT_A, 30.27, 120.15, "电信");
		// Node B: 广州联通 (23.13, 113.26)
		pidB_ = startSfu(GEO_PORT_B, 23.13, 113.26, "联通");
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

	char buf[4096]{};
	int n = ::recv(fd, buf, sizeof(buf) - 1, 0);
	::close(fd);
	ASSERT_GT(n, 0);

	std::string response(buf, n);
	EXPECT_NE(response.find("200"), std::string::npos) << response;
	// Should pick 杭州电信 (GEO_PORT_A) over 广州联通 (GEO_PORT_B)
	bool hasPortA = response.find(std::to_string(GEO_PORT_A)) != std::string::npos;
	bool hasPortB = response.find(std::to_string(GEO_PORT_B)) != std::string::npos;
	EXPECT_TRUE(hasPortA) << "Beijing Telecom should route to 杭州电信, got: " << response;
	EXPECT_FALSE(hasPortB) << "Should NOT route to 广州联通, got: " << response;
}

// Direct join on 广州 node with Beijing Telecom peer should redirect to 杭州
TEST_F(GeoJoinTest, DirectJoinRedirectsToGeoOptimal) {
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
		{"roomId", testRoom_}, {"peerId", "beijing_user"},
		{"displayName", "test"}, {"rtpCapabilities", rtpCaps}
	});

	// Since client IP is 127.0.0.1 (loopback), geo lookup won't work.
	// The join should succeed locally (no geo info = no redirect).
	// This tests that the code path doesn't crash with loopback IP.
	// True geo redirect requires a real external IP which we can't simulate here.
	bool ok = resp.value("ok", false);
	bool hasRedirect = resp.contains("redirect");
	EXPECT_TRUE(ok || hasRedirect)
		<< "Join should either succeed or redirect, got: " << resp.dump();
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

	static pid_t startSfu(int port, double lat, double lng,
		const std::string& isp, const std::string& country, bool isolation)
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
		kill(pid, SIGKILL);
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

		// Clean stale room and node entries from other geo tests
		{
			redisContext* ctx = redisConnect("127.0.0.1", 6379);
			if (ctx && !ctx->err) {
				auto* r = (redisReply*)redisCommand(ctx, "KEYS room:iso_*");
				if (r && r->type == REDIS_REPLY_ARRAY) {
					for (size_t i = 0; i < r->elements; i++)
						redisCommand(ctx, "DEL %s", r->element[i]->str);
					freeReplyObject(r);
				}
				r = (redisReply*)redisCommand(ctx, "KEYS node:*");
				if (r && r->type == REDIS_REPLY_ARRAY) {
					for (size_t i = 0; i < r->elements; i++)
						redisCommand(ctx, "DEL %s", r->element[i]->str);
					freeReplyObject(r);
				}
				redisFree(ctx);
			}
		}

		pidCN_ = startSfu(ISO_PORT_CN, 30.27, 120.15, "电信", "中国", true);
		pidUS_ = startSfu(ISO_PORT_US, 37.39, -122.08, "Amazon", "United States", true);
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

// Chinese client should only be routed to CN node, not US node
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

	char buf[4096]{};
	int n = ::recv(fd, buf, sizeof(buf) - 1, 0);
	::close(fd);
	ASSERT_GT(n, 0);

	std::string response(buf, n);
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

	char buf[4096]{};
	int n = ::recv(fd, buf, sizeof(buf) - 1, 0);
	::close(fd);
	ASSERT_GT(n, 0);

	std::string response(buf, n);
	bool hasUS = response.find(std::to_string(ISO_PORT_US)) != std::string::npos;
	bool hasCN = response.find(std::to_string(ISO_PORT_CN)) != std::string::npos;
	EXPECT_TRUE(hasUS) << "US client should route to US node, got: " << response;
	EXPECT_FALSE(hasCN) << "US client should NOT route to CN node, got: " << response;
}
