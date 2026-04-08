// Multi-node integration tests: /api/resolve, node load, maxRoutersPerWorker, room TTL refresh
//
// Requires: built mediasoup-sfu + mediasoup-worker + Redis on 127.0.0.1:6379

#include <gtest/gtest.h>
#include "TestWsClient.h"
#include <signal.h>
#include <sys/wait.h>
#include <hiredis/hiredis.h>

static const std::string HOST = "127.0.0.1";

static json makeRtpCaps() {
	return {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"kind", "audio"},
			{"clockRate", 48000}, {"channels", 2}, {"preferredPayloadType", 100}
		}, {
			{"mimeType", "video/VP8"}, {"kind", "video"},
			{"clockRate", 90000}, {"preferredPayloadType", 101}
		}}},
		{"headerExtensions", json::array()}
	};
}

// Simple HTTP GET helper
static std::string httpGet(const std::string& host, int port, const std::string& path) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return "";
	struct timeval tv{5, 0};
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
	if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { ::close(fd); return ""; }
	std::string req = "GET " + path + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
	::send(fd, req.data(), req.size(), 0);
	std::string response;
	char buf[4096];
	while (true) {
		int n = ::recv(fd, buf, sizeof(buf), 0);
		if (n <= 0) break;
		response.append(buf, n);
	}
	::close(fd);
	auto pos = response.find("\r\n\r\n");
	if (pos == std::string::npos) return "";
	return response.substr(pos + 4);
}

class MultiNodeResolveTest : public ::testing::Test {
protected:
	struct SfuNode { int port; pid_t pid = -1; int maxRouters; };
	std::vector<SfuNode> nodes_;

	static pid_t startSfu(int port, int workers = 1, int maxRoutersPerWorker = 0) {
		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(port) +
			" --workers=" + std::to_string(workers) +
			" --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1"
			" --listenIp=127.0.0.1";
		if (maxRoutersPerWorker > 0)
			cmd += " --maxRoutersPerWorker=" + std::to_string(maxRoutersPerWorker);
		cmd += " > /dev/null 2>&1 & echo $!";
		FILE* fp = popen(cmd.c_str(), "r");
		if (!fp) return -1;
		char buf[64]{};
		fgets(buf, sizeof(buf), fp);
		pclose(fp);
		return atoi(buf);
	}

	static bool waitForPort(int port, int timeoutMs = 8000) {
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

	static void killSfu(pid_t pid, int port) {
		if (pid <= 0) return;
		kill(pid, SIGKILL);
		for (int i = 0; i < 30; ++i) {
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

	void flushRedis() {
		auto* ctx = redisConnect("127.0.0.1", 6379);
		if (!ctx || ctx->err) { if (ctx) redisFree(ctx); return; }
		auto* reply = (redisReply*)redisCommand(ctx, "KEYS room:*");
		if (reply && reply->type == REDIS_REPLY_ARRAY)
			for (size_t i = 0; i < reply->elements; i++)
				redisCommand(ctx, "DEL %s", reply->element[i]->str);
		if (reply) freeReplyObject(reply);
		reply = (redisReply*)redisCommand(ctx, "KEYS node:*");
		if (reply && reply->type == REDIS_REPLY_ARRAY)
			for (size_t i = 0; i < reply->elements; i++)
				redisCommand(ctx, "DEL %s", reply->element[i]->str);
		if (reply) freeReplyObject(reply);
		redisFree(ctx);
	}

	std::string testPrefix_;

	void SetUp() override {
		testPrefix_ = "resolve_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
		flushRedis();
	}

	void TearDown() override {
		for (auto& n : nodes_) killSfu(n.pid, n.port);
		flushRedis();
	}

	void startNodes(std::vector<std::pair<int,int>> portAndMaxRouters) {
		for (auto& [port, maxR] : portAndMaxRouters) {
			auto pid = startSfu(port, 1, maxR);
			ASSERT_GT(pid, 0);
			nodes_.push_back({port, pid, maxR});
		}
		for (auto& n : nodes_)
			ASSERT_TRUE(waitForPort(n.port)) << "SFU on port " << n.port << " did not start";
		// Wait for first heartbeat to register nodes in Redis
		usleep(1500000);
	}

	std::string roomName(const std::string& suffix) {
		return testPrefix_ + "_" + suffix;
	}
};

// ═══════════════════════════════════════════════════════════════
// Test 1: /api/resolve returns self for new room
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiNodeResolveTest, ResolveNewRoomReturnsSelf) {
	startNodes({{18780, 0}});
	std::string body = httpGet(HOST, 18780, "/api/resolve?roomId=" + roomName("new"));
	ASSERT_FALSE(body.empty());
	auto j = json::parse(body);
	EXPECT_TRUE(j.contains("wsUrl"));
	EXPECT_TRUE(j["isNew"].get<bool>());
	// Should contain the node's own address
	EXPECT_NE(j["wsUrl"].get<std::string>().find("18780"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════════
// Test 2: /api/resolve returns owner node for existing room
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiNodeResolveTest, ResolveExistingRoomReturnsOwner) {
	startNodes({{18780, 0}, {18781, 0}});
	std::string room = roomName("existing");

	// Alice joins on SFU-A → claims room
	TestWsClient alice;
	ASSERT_TRUE(alice.connect(HOST, 18780));
	auto resp = alice.request("join", {
		{"roomId", room}, {"peerId", "alice"},
		{"displayName", "alice"}, {"rtpCapabilities", makeRtpCaps()}
	});
	ASSERT_TRUE(resp.value("ok", false));

	// Resolve from SFU-B should point to SFU-A
	std::string body = httpGet(HOST, 18781, "/api/resolve?roomId=" + room);
	ASSERT_FALSE(body.empty());
	auto j = json::parse(body);
	EXPECT_FALSE(j["isNew"].get<bool>());
	EXPECT_NE(j["wsUrl"].get<std::string>().find("18780"), std::string::npos)
		<< "Should resolve to SFU-A, got: " << j["wsUrl"];

	alice.close();
}

// ═══════════════════════════════════════════════════════════════
// Test 3: /api/resolve missing roomId returns 400
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiNodeResolveTest, ResolveMissingRoomIdReturns400) {
	startNodes({{18780, 0}});
	// Raw HTTP to check status code
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(18780);
	inet_pton(AF_INET, HOST.c_str(), &addr.sin_addr);
	ASSERT_EQ(::connect(fd, (sockaddr*)&addr, sizeof(addr)), 0);
	std::string req = "GET /api/resolve HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
	::send(fd, req.data(), req.size(), 0);
	char buf[4096]{};
	::recv(fd, buf, sizeof(buf), 0);
	::close(fd);
	EXPECT_NE(std::string(buf).find("400"), std::string::npos);
}

// ═══════════════════════════════════════════════════════════════
// Test 4: /api/node-load returns room count
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiNodeResolveTest, NodeLoadEndpoint) {
	startNodes({{18780, 10}});

	// Before any rooms
	std::string body = httpGet(HOST, 18780, "/api/node-load");
	auto j = json::parse(body);
	EXPECT_EQ(j["rooms"].get<int>(), 0);
	EXPECT_EQ(j["maxRooms"].get<int>(), 10); // 1 worker * 10 maxRoutersPerWorker

	// Create a room
	TestWsClient ws;
	ASSERT_TRUE(ws.connect(HOST, 18780));
	auto resp = ws.request("join", {
		{"roomId", roomName("load")}, {"peerId", "p1"},
		{"displayName", "p1"}, {"rtpCapabilities", makeRtpCaps()}
	});
	ASSERT_TRUE(resp.value("ok", false));

	body = httpGet(HOST, 18780, "/api/node-load");
	j = json::parse(body);
	EXPECT_EQ(j["rooms"].get<int>(), 1);

	ws.close();
}

// ═══════════════════════════════════════════════════════════════
// Test 5: maxRoutersPerWorker — node rejects room when full
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiNodeResolveTest, MaxRoutersPerWorkerRejectsWhenFull) {
	// 1 worker, max 2 routers per worker
	startNodes({{18780, 2}});

	std::vector<std::unique_ptr<TestWsClient>> clients;

	// Create 2 rooms — should succeed
	for (int i = 0; i < 2; i++) {
		auto ws = std::make_unique<TestWsClient>();
		ASSERT_TRUE(ws->connect(HOST, 18780));
		auto resp = ws->request("join", {
			{"roomId", roomName("full_" + std::to_string(i))},
			{"peerId", "p" + std::to_string(i)},
			{"displayName", "p"}, {"rtpCapabilities", makeRtpCaps()}
		});
		ASSERT_TRUE(resp.value("ok", false)) << "Room " << i << " should succeed";
		clients.push_back(std::move(ws));
	}

	// 3rd room — should fail (no available worker)
	TestWsClient ws3;
	ASSERT_TRUE(ws3.connect(HOST, 18780));
	auto resp3 = ws3.request("join", {
		{"roomId", roomName("full_overflow")}, {"peerId", "overflow"},
		{"displayName", "overflow"}, {"rtpCapabilities", makeRtpCaps()}
	});
	EXPECT_FALSE(resp3.value("ok", true)) << "3rd room should be rejected";

	for (auto& c : clients) c->close();
	ws3.close();
}

// ═══════════════════════════════════════════════════════════════
// Test 6: Resolve picks least-loaded node for new room
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiNodeResolveTest, ResolveLeastLoadedNode) {
	startNodes({{18780, 10}, {18781, 10}});

	// Create 3 rooms on SFU-A to make it more loaded
	std::vector<std::unique_ptr<TestWsClient>> clients;
	for (int i = 0; i < 3; i++) {
		auto ws = std::make_unique<TestWsClient>();
		ASSERT_TRUE(ws->connect(HOST, 18780));
		auto resp = ws->request("join", {
			{"roomId", roomName("load_a_" + std::to_string(i))},
			{"peerId", "pa" + std::to_string(i)},
			{"displayName", "p"}, {"rtpCapabilities", makeRtpCaps()}
		});
		ASSERT_TRUE(resp.value("ok", false));
		clients.push_back(std::move(ws));
	}

	// Wait for heartbeat to propagate load to Redis
	usleep(1500000);

	// Resolve a new room from SFU-A — should pick SFU-B (less loaded)
	std::string body = httpGet(HOST, 18780, "/api/resolve?roomId=" + roomName("new_balanced"));
	auto j = json::parse(body);
	EXPECT_TRUE(j["isNew"].get<bool>());
	EXPECT_NE(j["wsUrl"].get<std::string>().find("18781"), std::string::npos)
		<< "Should resolve to less-loaded SFU-B, got: " << j["wsUrl"];

	for (auto& c : clients) c->close();
}

// ═══════════════════════════════════════════════════════════════
// Test 7: Room TTL refresh — room stays alive during long session
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiNodeResolveTest, RoomTtlRefresh) {
	startNodes({{18780, 0}});
	std::string room = roomName("ttl");

	TestWsClient ws;
	ASSERT_TRUE(ws.connect(HOST, 18780));
	auto resp = ws.request("join", {
		{"roomId", room}, {"peerId", "p1"},
		{"displayName", "p1"}, {"rtpCapabilities", makeRtpCaps()}
	});
	ASSERT_TRUE(resp.value("ok", false));

	// Check Redis TTL — should be refreshed by heartbeat
	auto* ctx = redisConnect("127.0.0.1", 6379);
	ASSERT_NE(ctx, nullptr);
	ASSERT_FALSE(ctx->err);

	// Wait for at least one heartbeat cycle
	usleep(1500000);

	auto* reply = (redisReply*)redisCommand(ctx, "TTL room:%s", room.c_str());
	ASSERT_NE(reply, nullptr);
	ASSERT_EQ(reply->type, REDIS_REPLY_INTEGER);
	// TTL should be close to kRedisRoomTtlSec (300), not decreasing toward 0
	EXPECT_GT(reply->integer, 200) << "Room TTL should be refreshed, got: " << reply->integer;
	freeReplyObject(reply);
	redisFree(ctx);

	ws.close();
}

// ═══════════════════════════════════════════════════════════════
// Test 8: Resolve + redirect workflow — client uses resolve then direct connects
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiNodeResolveTest, ResolveAndDirectConnect) {
	startNodes({{18780, 0}, {18781, 0}});
	std::string room = roomName("resolve_flow");

	// Alice joins on SFU-A
	TestWsClient alice;
	ASSERT_TRUE(alice.connect(HOST, 18780));
	auto aliceResp = alice.request("join", {
		{"roomId", room}, {"peerId", "alice"},
		{"displayName", "alice"}, {"rtpCapabilities", makeRtpCaps()}
	});
	ASSERT_TRUE(aliceResp.value("ok", false));

	// Bob resolves from SFU-B
	std::string body = httpGet(HOST, 18781, "/api/resolve?roomId=" + room);
	auto resolved = json::parse(body);
	std::string wsUrl = resolved["wsUrl"].get<std::string>();
	EXPECT_NE(wsUrl.find("18780"), std::string::npos);

	// Bob connects directly to SFU-A (the resolved node)
	TestWsClient bob;
	ASSERT_TRUE(bob.connect(HOST, 18780));
	auto bobResp = bob.request("join", {
		{"roomId", room}, {"peerId", "bob"},
		{"displayName", "bob"}, {"rtpCapabilities", makeRtpCaps()}
	});
	ASSERT_TRUE(bobResp.value("ok", false));

	// Alice should get peerJoined
	auto notif = alice.waitNotification("peerJoined", 3000);
	ASSERT_FALSE(notif.empty());
	EXPECT_EQ(notif["data"]["peerId"], "bob");

	alice.close();
	bob.close();
}
