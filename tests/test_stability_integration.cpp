// Integration tests for stability fixes:
// - WebSocket close handler exception safety
// - Signaling error responses on invalid operations
// - Rapid connect/disconnect doesn't crash
// - Unknown method returns error (not crash)
#include <gtest/gtest.h>
#include "TestWsClient.h"
#include <signal.h>
#include <sys/wait.h>

static const int SFU_PORT = 14003;
static const std::string HOST = "127.0.0.1";

class StabilityIntegrationTest : public ::testing::Test {
protected:
	pid_t sfuPid_ = -1;
	std::string testRoom_;

	void SetUp() override {
		testRoom_ = "stab_" + std::to_string(getpid()) + "_" +
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
			kill(sfuPid_, SIGTERM); for(int w_=0; w_<40 && kill(sfuPid_,0)==0; w_++) usleep(50000); kill(sfuPid_, SIGKILL); usleep(100000);
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

	struct JoinedClient {
		std::unique_ptr<TestWsClient> ws;
		std::string peerId;
		std::string roomId;
	};

	JoinedClient joinRoom(const std::string& roomId, const std::string& peerId) {
		JoinedClient c;
		c.roomId = roomId;
		c.peerId = peerId;
		c.ws = std::make_unique<TestWsClient>();
		EXPECT_TRUE(c.ws->connect(HOST, SFU_PORT));

		json rtpCaps = {
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

		auto resp = c.ws->request("join", {
			{"roomId", roomId}, {"peerId", peerId},
			{"displayName", peerId}, {"rtpCapabilities", rtpCaps}
		});
		EXPECT_TRUE(resp.value("ok", false)) << "join failed: " << resp.dump();
		return c;
	}
};

// ─── Test 1: Close after join doesn't crash server ───
TEST_F(StabilityIntegrationTest, CloseAfterJoinNoCrash) {
	auto alice = joinRoom(testRoom_, "alice");
	alice.ws->close();
	usleep(200000);

	// Server should still be alive — verify by joining again
	auto bob = joinRoom(testRoom_ + "_2", "bob");
	EXPECT_TRUE(bob.ws != nullptr);
}

// ─── Test 2: Close during produce doesn't crash ───
TEST_F(StabilityIntegrationTest, CloseWithActiveProducer) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	// Bob creates recv transport
	bob.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true}
	});

	// Alice creates send transport and produces
	auto sendResp = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(sendResp.value("ok", false));

	json rtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 11111111}}}},
		{"mid", "0"}
	};
	alice.ws->request("produce", {
		{"transportId", sendResp["data"]["id"]}, {"kind", "audio"},
		{"rtpParameters", rtpParams}
	});

	// Wait for Bob to get newConsumer
	bob.ws->waitNotification("newConsumer", 2000);

	// Alice abruptly disconnects while producing
	alice.ws->close();
	usleep(300000);

	// Bob should get peerLeft and server should still be alive
	auto leftNotif = bob.ws->waitNotification("peerLeft", 3000);
	EXPECT_FALSE(leftNotif.empty());

	// Server still alive
	auto charlie = joinRoom(testRoom_ + "_3", "charlie");
	EXPECT_TRUE(charlie.ws != nullptr);
}

// ─── Test 3: Unknown method returns error, not crash ───
TEST_F(StabilityIntegrationTest, UnknownMethodReturnsError) {
	auto alice = joinRoom(testRoom_, "alice");

	auto resp = alice.ws->request("nonExistentMethod", {{"foo", "bar"}});
	EXPECT_FALSE(resp.value("ok", true));
	EXPECT_TRUE(resp.contains("error"));
}

// ─── Test 4: Operations on non-existent room/peer return error ───
TEST_F(StabilityIntegrationTest, OperationsOnBadRoomReturnError) {
	auto alice = joinRoom(testRoom_, "alice");

	// Try to produce on non-existent transport
	json rtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 99999}}}},
		{"mid", "0"}
	};
	auto resp = alice.ws->request("produce", {
		{"transportId", "fake-transport-id"}, {"kind", "audio"},
		{"rtpParameters", rtpParams}
	});
	EXPECT_FALSE(resp.value("ok", true));
}

// ─── Test 5: Rapid connect/disconnect cycles ───
TEST_F(StabilityIntegrationTest, RapidConnectDisconnect) {
	for (int i = 0; i < 10; i++) {
		std::string peerId = "rapid_" + std::to_string(i);
		auto client = joinRoom(testRoom_, peerId);
		client.ws->close();
	}
	usleep(500000);

	// Server should still be alive
	auto final_client = joinRoom(testRoom_ + "_final", "survivor");
	EXPECT_TRUE(final_client.ws != nullptr);
}

// ─── Test 6: Connect without join, then disconnect ───
TEST_F(StabilityIntegrationTest, DisconnectWithoutJoin) {
	// Connect WebSocket but never send join
	TestWsClient ws;
	ASSERT_TRUE(ws.connect(HOST, SFU_PORT));
	ws.close();
	usleep(200000);

	// Server should still be alive
	auto alice = joinRoom(testRoom_, "alice");
	EXPECT_TRUE(alice.ws != nullptr);
}

// ─── Test 7: Double leave (close handler idempotency) ───
TEST_F(StabilityIntegrationTest, DoubleCloseNoCrash) {
	auto alice = joinRoom(testRoom_, "alice");
	alice.ws->close();
	usleep(100000);
	// close() again — should be idempotent
	alice.ws->close();
	usleep(200000);

	// Server still alive
	auto bob = joinRoom(testRoom_ + "_2", "bob");
	EXPECT_TRUE(bob.ws != nullptr);
}

// ─── Test 8: Event loop stays responsive during concurrent signaling ───
// Verifies the worker thread offloading: multiple clients can join/produce
// concurrently without blocking each other's responses.
TEST_F(StabilityIntegrationTest, EventLoopNotBlockedDuringIPC) {
	// Join 3 clients in parallel threads — if the event loop were blocked,
	// sequential IPC would cause later joins to timeout.
	constexpr int N = 3;
	std::vector<std::thread> threads;
	std::atomic<int> successCount{0};
	std::atomic<int> maxLatencyMs{0};

	for (int i = 0; i < N; ++i) {
		threads.emplace_back([&, i]{
			auto start = std::chrono::steady_clock::now();
			std::string peerId = "parallel_" + std::to_string(i);
			TestWsClient ws;
			if (!ws.connect(HOST, SFU_PORT)) return;

			json rtpCaps = {
				{"codecs", {{
					{"mimeType", "audio/opus"}, {"kind", "audio"},
					{"clockRate", 48000}, {"channels", 2},
					{"preferredPayloadType", 100}
				}}},
				{"headerExtensions", json::array()}
			};

			auto resp = ws.request("join", {
				{"roomId", testRoom_}, {"peerId", peerId},
				{"displayName", peerId}, {"rtpCapabilities", rtpCaps}
			}, 10000);

			if (resp.value("ok", false)) successCount++;

			auto elapsed = std::chrono::steady_clock::now() - start;
			int ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
			int prev = maxLatencyMs.load();
			while (ms > prev && !maxLatencyMs.compare_exchange_weak(prev, ms));

			ws.close();
		});
	}

	for (auto& t : threads) t.join();

	EXPECT_EQ(successCount.load(), N) << "Not all parallel joins succeeded";
	// With worker thread offloading, all joins should complete within a few seconds
	// even though each involves Channel IPC. Without offloading, they'd serialize
	// and the last one could take N * IPC_latency.
	EXPECT_LT(maxLatencyMs.load(), 8000) << "Parallel joins took too long: " << maxLatencyMs.load() << "ms";
}

// ─── Test 9: Responses arrive even after slow IPC ───
// Verifies that a client can still receive responses after another client's
// request triggers a slow Channel IPC call.
TEST_F(StabilityIntegrationTest, ResponseAfterSlowRequest) {
	auto alice = joinRoom(testRoom_, "alice");
	usleep(50000);

	// Alice creates transport (triggers Channel IPC)
	auto sendResp = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(sendResp.value("ok", false));

	// Immediately after, Bob joins — should not be blocked by Alice's IPC
	auto start = std::chrono::steady_clock::now();
	auto bob = joinRoom(testRoom_, "bob");
	auto elapsed = std::chrono::steady_clock::now() - start;
	int ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

	// Bob's join should complete reasonably fast
	EXPECT_LT(ms, 5000) << "Bob's join was blocked for " << ms << "ms";
}

// ─── Test 10: Disconnect during pending IPC doesn't crash ───
// Client sends a request then immediately disconnects before response arrives.
TEST_F(StabilityIntegrationTest, DisconnectDuringPendingRequest) {
	for (int i = 0; i < 5; ++i) {
		TestWsClient ws;
		ASSERT_TRUE(ws.connect(HOST, SFU_PORT));

		// Send join but don't wait for response — close immediately
		json msg = {{"request", true}, {"id", 1}, {"method", "join"},
			{"data", {{"roomId", testRoom_}, {"peerId", "ghost_" + std::to_string(i)},
				{"displayName", "ghost"}, {"rtpCapabilities", json::object()}}}};
		// Use raw send to avoid waiting for response
		ws.request("join", {{"roomId", testRoom_}, {"peerId", "ghost_" + std::to_string(i)},
			{"displayName", "ghost"}, {"rtpCapabilities", json::object()}}, 100);
		ws.close();
	}

	usleep(500000);

	// Server should still be alive
	auto survivor = joinRoom(testRoom_ + "_survive", "survivor");
	EXPECT_TRUE(survivor.ws != nullptr);
}
