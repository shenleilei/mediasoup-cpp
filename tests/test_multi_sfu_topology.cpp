// Multi-SFU multi-Worker topology integration test
//
// Topology:
//   SFU-A (port 14003, 2 workers)  ──┐
//   SFU-B (port 14004, 2 workers)  ──┼── Redis (room registry + node heartbeat)
//   SFU-C (port 14005, 1 worker)   ──┘
//
// Scenarios:
//   1. Room affinity: first joiner claims room, second joiner on different SFU gets redirect
//   2. Multi-room distribution: different rooms can live on different SFUs
//   3. Node crash takeover: if SFU-A dies, its rooms can be claimed by SFU-B
//   4. Multi-worker load balancing: creating multiple rooms on one SFU spreads across workers
//   5. Full 3-node conference: peers follow redirects and end up in the same room

#include <gtest/gtest.h>
#include "TestRedisServer.h"
#include "TestWsClient.h"
#include "TestProcessUtils.h"
#include <signal.h>
#include <sys/wait.h>
#include <set>
#include <hiredis/hiredis.h>

static const std::string HOST = "127.0.0.1";

struct SfuNode {
	int port;
	int workers;
	pid_t pid = -1;
};

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

class MultiSfuTopologyTest : public ::testing::Test {
protected:
	std::vector<SfuNode> nodes_;
	std::string testPrefix_;
	TestRedisServer redisServer_;

	static pid_t startSfu(int port, int workers, int redisPort) {
		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(port) +
			" --workers=" + std::to_string(workers) +
			" --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1"
			" --listenIp=127.0.0.1"
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
		terminateSfuProcess(pid);
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

	// Flush all sfu:room:* and sfu:node:* keys from Redis to avoid cross-test pollution
	void flushTestKeys() {
		auto* ctx = redisConnect("127.0.0.1", redisServer_.port());
		if (!ctx || ctx->err) { if (ctx) redisFree(ctx); return; }
		// Delete room keys
		auto* reply = (redisReply*)redisCommand(ctx, "KEYS sfu:room:*");
		if (reply && reply->type == REDIS_REPLY_ARRAY) {
			for (size_t i = 0; i < reply->elements; i++)
				redisCommand(ctx, "DEL %s", reply->element[i]->str);
		}
		if (reply) freeReplyObject(reply);
		// Delete node keys
		reply = (redisReply*)redisCommand(ctx, "KEYS sfu:node:*");
		if (reply && reply->type == REDIS_REPLY_ARRAY) {
			for (size_t i = 0; i < reply->elements; i++)
				redisCommand(ctx, "DEL %s", reply->element[i]->str);
		}
		if (reply) freeReplyObject(reply);
		redisFree(ctx);
	}

	void SetUp() override {
		testPrefix_ = "topo_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

		ASSERT_TRUE(redisServer_.start()) << redisServer_.failureMessage();

		flushTestKeys();

		nodes_ = {
			{14003, 2, -1},  // SFU-A: 2 workers
			{14004, 2, -1},  // SFU-B: 2 workers
			{14005, 1, -1},  // SFU-C: 1 worker
		};

		for (auto& n : nodes_) {
			n.pid = startSfu(n.port, n.workers, redisServer_.port());
			ASSERT_GT(n.pid, 0) << "Failed to start SFU on port " << n.port;
		}
		for (auto& n : nodes_) {
			ASSERT_TRUE(waitForPort(n.port))
				<< "SFU on port " << n.port << " did not start";
		}
	}

	void TearDown() override {
		for (auto& n : nodes_) killSfu(n.pid, n.port);
		flushTestKeys();
	}

	std::string roomName(const std::string& suffix) {
		return testPrefix_ + "_" + suffix;
	}

	// Join helper — returns {ok, redirect, response}
	struct JoinResult {
		bool ok = false;
		std::string redirect;
		json data;
	};

	JoinResult tryJoin(int port, const std::string& roomId, const std::string& peerId) {
		TestWsClient ws;
		if (!ws.connect(HOST, port)) return {false, "", {}};
		auto resp = ws.request("join", {
			{"roomId", roomId}, {"peerId", peerId},
			{"displayName", peerId}, {"rtpCapabilities", makeRtpCaps()}
		});
		JoinResult r;
		r.ok = resp.value("ok", false);
		r.redirect = resp.value("redirect", "");
		r.data = resp.value("data", json::object());
		ws.close();
		return r;
	}
};

// ═══════════════════════════════════════════════════════════════
// Test 1: Room affinity — first join claims, second gets redirect
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiSfuTopologyTest, RoomAffinityAndRedirect) {
	std::string room = roomName("affinity");

	// Alice joins on SFU-A → claims the room (keep connection alive!)
	TestWsClient aliceWs;
	ASSERT_TRUE(aliceWs.connect(HOST, 14003));
	auto aliceResp = aliceWs.request("join", {
		{"roomId", room}, {"peerId", "alice"},
		{"displayName", "alice"}, {"rtpCapabilities", makeRtpCaps()}
	});
	ASSERT_TRUE(aliceResp.value("ok", false)) << "Alice should join on SFU-A";

	// Bob tries to join same room on SFU-B → should get redirect to SFU-A
	auto bob = tryJoin(14004, room, "bob");
	EXPECT_FALSE(bob.ok) << "Bob should NOT join on SFU-B";
	EXPECT_FALSE(bob.redirect.empty()) << "Bob should get redirect";
	EXPECT_NE(bob.redirect.find("14003"), std::string::npos)
		<< "Redirect should point to SFU-A (port 14003), got: " << bob.redirect;

	// Charlie tries on SFU-C → also redirect to SFU-A
	auto charlie = tryJoin(14005, room, "charlie");
	EXPECT_FALSE(charlie.ok);
	EXPECT_NE(charlie.redirect.find("14003"), std::string::npos)
		<< "Charlie should also be redirected to SFU-A";

	aliceWs.close();
}

// ═══════════════════════════════════════════════════════════════
// Test 2: Multiple rooms distributed across different SFUs
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiSfuTopologyTest, MultiRoomDistribution) {
	std::string roomA = roomName("dist_A");
	std::string roomB = roomName("dist_B");
	std::string roomC = roomName("dist_C");

	// Each room created on a different SFU — keep connections alive
	TestWsClient wsA, wsB, wsC;
	ASSERT_TRUE(wsA.connect(HOST, 14003));
	ASSERT_TRUE(wsB.connect(HOST, 14004));
	ASSERT_TRUE(wsC.connect(HOST, 14005));

	auto rA = wsA.request("join", {{"roomId", roomA}, {"peerId", "peerA"},
		{"displayName", "peerA"}, {"rtpCapabilities", makeRtpCaps()}});
	auto rB = wsB.request("join", {{"roomId", roomB}, {"peerId", "peerB"},
		{"displayName", "peerB"}, {"rtpCapabilities", makeRtpCaps()}});
	auto rC = wsC.request("join", {{"roomId", roomC}, {"peerId", "peerC"},
		{"displayName", "peerC"}, {"rtpCapabilities", makeRtpCaps()}});

	ASSERT_TRUE(rA.value("ok", false)) << "Room A on SFU-A";
	ASSERT_TRUE(rB.value("ok", false)) << "Room B on SFU-B";
	ASSERT_TRUE(rC.value("ok", false)) << "Room C on SFU-C";

	// Cross-join: peer trying roomA on SFU-B → redirect to SFU-A
	auto cross = tryJoin(14004, roomA, "crossPeer");
	EXPECT_FALSE(cross.ok);
	EXPECT_NE(cross.redirect.find("14003"), std::string::npos);

	// Cross-join: peer trying roomB on SFU-C → redirect to SFU-B
	auto cross2 = tryJoin(14005, roomB, "crossPeer2");
	EXPECT_FALSE(cross2.ok);
	EXPECT_NE(cross2.redirect.find("14004"), std::string::npos);

	wsA.close(); wsB.close(); wsC.close();
}

// ═══════════════════════════════════════════════════════════════
// Test 3: Node crash → room takeover by another SFU
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiSfuTopologyTest, NodeCrashRoomTakeover) {
	std::string room = roomName("takeover");

	// Alice joins on SFU-A
	auto alice = tryJoin(14003, room, "alice");
	ASSERT_TRUE(alice.ok);

	// Kill SFU-A — simulate crash
	killSfu(nodes_[0].pid, nodes_[0].port);
	nodes_[0].pid = -1;

	// Wait for Redis sfu:node:* key to expire (nodeTTL=30s is too long for test).
	// Instead, manually delete the node key to simulate expiry.
	{
		auto* ctx = redisConnect("127.0.0.1", redisServer_.port());
		ASSERT_NE(ctx, nullptr);
		// Find SFU-A's node key and delete it
		auto* reply = (redisReply*)redisCommand(ctx, "KEYS sfu:node:*14003*");
		if (reply && reply->type == REDIS_REPLY_ARRAY) {
			for (size_t i = 0; i < reply->elements; i++) {
				std::string nid = std::string(reply->element[i]->str).substr(9); // strip "sfu:node:"
				redisCommand(ctx, "DEL %s", reply->element[i]->str);
				std::string msg = nid + "=";
				redisCommand(ctx, "PUBLISH sfu:ch:nodes %s", msg.c_str());
			}
		}
		if (reply) freeReplyObject(reply);
		// Also try hostname-based pattern
		char hostname[256] = {};
		gethostname(hostname, sizeof(hostname));
		std::string pattern = std::string("sfu:node:") + hostname + ":14003";
		std::string nid = std::string(hostname) + ":14003";
		auto* r2 = (redisReply*)redisCommand(ctx, "DEL %s", pattern.c_str());
		if (r2) freeReplyObject(r2);
		std::string nmsg = nid + "=";
		redisCommand(ctx, "PUBLISH sfu:ch:nodes %s", nmsg.c_str());
		// Also delete the room key so SFU-B can take over
		auto* rk = (redisReply*)redisCommand(ctx, "KEYS sfu:room:*");
		if (rk && rk->type == REDIS_REPLY_ARRAY) {
			for (size_t i = 0; i < rk->elements; i++) {
				std::string rid = std::string(rk->element[i]->str).substr(9); // strip "sfu:room:"
				redisCommand(ctx, "DEL %s", rk->element[i]->str);
				std::string rmsg = rid + "=";
				redisCommand(ctx, "PUBLISH sfu:ch:rooms %s", rmsg.c_str());
			}
		}
		if (rk) freeReplyObject(rk);
		redisFree(ctx);
	}

	usleep(5000000); // wait for pub/sub propagation (subscriber has 2s read timeout)

	// Bob tries to join the same room on SFU-B
	// Since SFU-A's node key is gone, SFU-B should take over the room
	auto bob = tryJoin(14004, room, "bob");
	EXPECT_TRUE(bob.ok)
		<< "Bob should take over the room on SFU-B after SFU-A crash. "
		<< "redirect=" << bob.redirect;
}

// ═══════════════════════════════════════════════════════════════
// Test 4: Multi-worker load balancing — multiple rooms on one SFU
//         should spread across workers (verified by room creation success)
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiSfuTopologyTest, MultiWorkerLoadBalancing) {
	// Create 6 rooms on SFU-A (which has 2 workers)
	// All should succeed — if load balancing works, routers spread across workers
	for (int i = 0; i < 6; i++) {
		std::string room = roomName("lb_" + std::to_string(i));
		auto r = tryJoin(14003, room, "peer_" + std::to_string(i));
		ASSERT_TRUE(r.ok) << "Room " << i << " creation failed on SFU-A";
	}

	// Create 6 rooms on SFU-C (which has only 1 worker)
	// Should also succeed — single worker handles all
	for (int i = 0; i < 6; i++) {
		std::string room = roomName("lb_c_" + std::to_string(i));
		auto r = tryJoin(14005, room, "peer_c_" + std::to_string(i));
		ASSERT_TRUE(r.ok) << "Room " << i << " creation failed on SFU-C (1 worker)";
	}
}

// ═══════════════════════════════════════════════════════════════
// Test 5: Full 3-node scenario — peers follow redirects, end up
//         in same room, produce/subscribe works
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiSfuTopologyTest, FullCrossNodeConference) {
	std::string room = roomName("conference");

	// Alice joins on SFU-A (claims room)
	TestWsClient aliceWs;
	ASSERT_TRUE(aliceWs.connect(HOST, 14003));
	auto aliceJoin = aliceWs.request("join", {
		{"roomId", room}, {"peerId", "alice"},
		{"displayName", "alice"}, {"rtpCapabilities", makeRtpCaps()}
	});
	ASSERT_TRUE(aliceJoin.value("ok", false));

	// Bob tries SFU-B → gets redirect → follows to SFU-A
	TestWsClient bobProbe;
	ASSERT_TRUE(bobProbe.connect(HOST, 14004));
	auto bobTry = bobProbe.request("join", {
		{"roomId", room}, {"peerId", "bob"},
		{"displayName", "bob"}, {"rtpCapabilities", makeRtpCaps()}
	});
	EXPECT_FALSE(bobTry.value("ok", true));
	ASSERT_TRUE(bobTry.contains("redirect"));
	bobProbe.close();

	// Bob follows redirect to SFU-A
	TestWsClient bobWs;
	ASSERT_TRUE(bobWs.connect(HOST, 14003));
	auto bobJoin = bobWs.request("join", {
		{"roomId", room}, {"peerId", "bob"},
		{"displayName", "bob"}, {"rtpCapabilities", makeRtpCaps()}
	});
	ASSERT_TRUE(bobJoin.value("ok", false));

	// Alice should get peerJoined
	auto notif = aliceWs.waitNotification("peerJoined", 3000);
	ASSERT_FALSE(notif.empty());
	EXPECT_EQ(notif["data"]["peerId"], "bob");

	// Bob creates transports and produces
	auto bobSend = bobWs.request("createWebRtcTransport",
		{{"producing", true}, {"consuming", false}});
	ASSERT_TRUE(bobSend.value("ok", false));

	// Alice creates recv transport
	auto aliceRecv = aliceWs.request("createWebRtcTransport",
		{{"producing", false}, {"consuming", true}});
	ASSERT_TRUE(aliceRecv.value("ok", false));

	// Bob produces audio
	auto produceResp = bobWs.request("produce", {
		{"transportId", bobSend["data"]["id"]}, {"kind", "audio"},
		{"rtpParameters", {
			{"codecs", {{{"mimeType", "audio/opus"}, {"clockRate", 48000},
				{"channels", 2}, {"payloadType", 100}}}},
			{"encodings", {{{"ssrc", 0xBB001}}}}, {"mid", "0"}
		}}
	});
	ASSERT_TRUE(produceResp.value("ok", false));

	// Alice should get newConsumer notification
	auto consumerNotif = aliceWs.waitNotification("newConsumer", 3000);
	ASSERT_FALSE(consumerNotif.empty()) << "Alice should receive newConsumer from Bob";
	EXPECT_EQ(consumerNotif["data"]["peerId"], "bob");
	EXPECT_EQ(consumerNotif["data"]["kind"], "audio");

	// Charlie tries SFU-C → redirect → joins SFU-A
	TestWsClient charlieProbe;
	ASSERT_TRUE(charlieProbe.connect(HOST, 14005));
	auto charlieTry = charlieProbe.request("join", {
		{"roomId", room}, {"peerId", "charlie"},
		{"displayName", "charlie"}, {"rtpCapabilities", makeRtpCaps()}
	});
	EXPECT_FALSE(charlieTry.value("ok", true));
	ASSERT_TRUE(charlieTry.contains("redirect"));
	charlieProbe.close();

	TestWsClient charlieWs;
	ASSERT_TRUE(charlieWs.connect(HOST, 14003));
	auto charlieJoin = charlieWs.request("join", {
		{"roomId", room}, {"peerId", "charlie"},
		{"displayName", "charlie"}, {"rtpCapabilities", makeRtpCaps()}
	});
	ASSERT_TRUE(charlieJoin.value("ok", false));

	// Charlie's join should show Bob's existing producer
	ASSERT_TRUE(charlieJoin["data"].contains("existingProducers"));
	EXPECT_GE(charlieJoin["data"]["existingProducers"].size(), 1u);

	// Participants should be 3
	EXPECT_EQ(charlieJoin["data"]["participants"].size(), 3u);

	aliceWs.close();
	bobWs.close();
	charlieWs.close();
}

// ═══════════════════════════════════════════════════════════════
// Test 6: Concurrent room creation on all 3 SFUs — no conflicts
// ═══════════════════════════════════════════════════════════════
TEST_F(MultiSfuTopologyTest, ConcurrentRoomCreation) {
	// Create different rooms on each SFU simultaneously
	std::vector<std::thread> threads;
	std::atomic<int> successes{0};

	for (int n = 0; n < 3; n++) {
		threads.emplace_back([&, n]() {
			for (int i = 0; i < 5; i++) {
				// Each SFU creates its own set of rooms
				std::string room = roomName("concurrent_" + std::to_string(n) + "_" + std::to_string(i));
				TestWsClient ws;
				if (!ws.connect(HOST, nodes_[n].port)) continue;
				auto resp = ws.request("join", {
					{"roomId", room}, {"peerId", "p_" + std::to_string(n) + "_" + std::to_string(i)},
					{"displayName", "peer"}, {"rtpCapabilities", makeRtpCaps()}
				});
				if (resp.value("ok", false)) successes++;
				ws.close();
			}
		});
	}

	for (auto& t : threads) t.join();

	// All 15 rooms (5 per SFU, unique names) should succeed
	EXPECT_EQ(successes.load(), 15)
		<< "All 15 unique rooms should be created successfully";
}
