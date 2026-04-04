// Integration (black-box) tests: start real SFU, drive via WebSocket signaling.
#include <gtest/gtest.h>
#include "TestWsClient.h"
#include <signal.h>
#include <sys/wait.h>

static const int SFU_PORT = 18765;  // use high port to avoid conflicts
static const std::string HOST = "127.0.0.1";

class IntegrationTest : public ::testing::Test {
protected:
	pid_t sfuPid_ = -1;
	std::string testRoom_; // unique room per test to avoid Redis conflicts

	void SetUp() override {
		// Generate unique room name per test
		testRoom_ = "room_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

		// Start SFU as a detached background process (avoids fork fd inheritance issues)
		std::string cmd = "./build/mediasoup-sfu"
			" --port=" + std::to_string(SFU_PORT) +
			" --workers=1"
			" --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1"
			" --listenIp=127.0.0.1"
			" > /dev/null 2>&1 & echo $!";
		FILE* fp = popen(cmd.c_str(), "r");
		ASSERT_NE(fp, nullptr);
		char buf[64]{};
		fgets(buf, sizeof(buf), fp);
		pclose(fp);
		sfuPid_ = atoi(buf);
		ASSERT_GT(sfuPid_, 0) << "Failed to start SFU";

		// Wait for SFU to accept WebSocket connections
		for (int i = 0; i < 50; ++i) {
			usleep(100000); // 100ms
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(SFU_PORT);
			inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
			if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0) {
				::close(fd);
				usleep(200000); // 200ms extra for uWS event loop to fully start
				return;
			}
			::close(fd);
		}
		FAIL() << "SFU did not start within 5 seconds";
	}

	void TearDown() override {
		if (sfuPid_ > 0) {
			kill(sfuPid_, SIGKILL); // SIGKILL for fast cleanup in tests
			for (int i = 0; i < 20; ++i) {
				usleep(50000); // 50ms
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

	// Helper: connect a client and join a room
	struct JoinedClient {
		std::unique_ptr<TestWsClient> ws;
		std::string peerId;
		std::string roomId;
		json routerRtpCapabilities;
	};

	JoinedClient joinRoom(const std::string& roomId, const std::string& peerId) {
		JoinedClient c;
		c.roomId = roomId;
		c.peerId = peerId;
		c.ws = std::make_unique<TestWsClient>();
		EXPECT_TRUE(c.ws->connect(HOST, 18765));

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
		if (resp.contains("data") && resp["data"].contains("routerRtpCapabilities"))
			c.routerRtpCapabilities = resp["data"]["routerRtpCapabilities"];
		return c;
	}
};

// ─── Test 1: Basic join and leave ───
TEST_F(IntegrationTest, JoinAndLeave) {
	auto alice = joinRoom(testRoom_, "alice");
	// Small delay to ensure alice's ws is fully registered in wsMap
	usleep(50000);

	// Bob joins, Alice should get peerJoined notification
	auto bob = joinRoom(testRoom_, "bob");

	auto notif = alice.ws->waitNotification("peerJoined", 3000);
	ASSERT_FALSE(notif.empty()) << "Alice did not receive peerJoined";
	EXPECT_EQ(notif["data"]["peerId"], "bob");

	// Bob disconnects, Alice should get peerLeft
	bob.ws->close();
	auto leftNotif = alice.ws->waitNotification("peerLeft", 3000);
	ASSERT_FALSE(leftNotif.empty()) << "Alice did not receive peerLeft";
	EXPECT_EQ(leftNotif["data"]["peerId"], "bob");
}

// ─── Test 2: Create transports ───
TEST_F(IntegrationTest, CreateTransports) {
	auto alice = joinRoom(testRoom_, "alice");

	// Create send transport
	auto sendResp = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(sendResp.value("ok", false)) << sendResp.dump();
	EXPECT_TRUE(sendResp["data"].contains("id"));
	EXPECT_TRUE(sendResp["data"].contains("iceParameters"));
	EXPECT_TRUE(sendResp["data"].contains("iceCandidates"));
	EXPECT_TRUE(sendResp["data"].contains("dtlsParameters"));

	// Create recv transport
	auto recvResp = alice.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true}
	});
	ASSERT_TRUE(recvResp.value("ok", false)) << recvResp.dump();
	EXPECT_NE(recvResp["data"]["id"], sendResp["data"]["id"]);
}

// ─── Test 3: Produce → auto-subscribe notification ───
TEST_F(IntegrationTest, ProduceAndAutoSubscribe) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	// Bob creates recv transport first (so auto-subscribe can work)
	auto bobRecv = bob.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true}
	});
	ASSERT_TRUE(bobRecv.value("ok", false));

	// Alice creates send transport
	auto aliceSend = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(aliceSend.value("ok", false));
	std::string aliceSendId = aliceSend["data"]["id"];

	// Alice produces audio
	json rtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 11111111}}}},
		{"mid", "0"}
	};
	auto produceResp = alice.ws->request("produce", {
		{"transportId", aliceSendId}, {"kind", "audio"}, {"rtpParameters", rtpParams}
	});
	ASSERT_TRUE(produceResp.value("ok", false)) << "produce failed: " << produceResp.dump();
	EXPECT_TRUE(produceResp["data"].contains("id"));
	std::string producerId = produceResp["data"]["id"];

	// Bob should receive newConsumer notification (auto-subscribe)
	auto consumerNotif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(consumerNotif.empty()) << "Bob did not receive newConsumer";
	EXPECT_EQ(consumerNotif["data"]["peerId"], "alice");
	EXPECT_EQ(consumerNotif["data"]["producerId"], producerId);
	EXPECT_EQ(consumerNotif["data"]["kind"], "audio");
}

// ─── Test 4: Pause and resume producer ───
TEST_F(IntegrationTest, PauseResumeProducer) {
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
		{"encodings", {{{"ssrc", 22222222}}}},
		{"mid", "0"}
	};
	auto produceResp = alice.ws->request("produce", {
		{"transportId", sendResp["data"]["id"]}, {"kind", "audio"}, {"rtpParameters", rtpParams}
	});
	ASSERT_TRUE(produceResp.value("ok", false));
	std::string producerId = produceResp["data"]["id"];

	// Pause
	auto pauseResp = alice.ws->request("pauseProducer", {{"producerId", producerId}});
	EXPECT_TRUE(pauseResp.value("ok", false));

	// Resume
	auto resumeResp = alice.ws->request("resumeProducer", {{"producerId", producerId}});
	EXPECT_TRUE(resumeResp.value("ok", false));
}

// ─── Test 5: Multiple peers, verify participants list ───
TEST_F(IntegrationTest, ParticipantsList) {
	// Join 4 peers sequentially, each should see all previous peers
	TestWsClient ws1, ws2, ws3, ws4;
	ASSERT_TRUE(ws1.connect(HOST, 18765));
	ASSERT_TRUE(ws2.connect(HOST, 18765));
	ASSERT_TRUE(ws3.connect(HOST, 18765));
	ASSERT_TRUE(ws4.connect(HOST, 18765));

	auto r1 = ws1.request("join", {{"roomId", testRoom_}, {"peerId", "p1"}, {"displayName", "p1"}});
	ASSERT_TRUE(r1.value("ok", false)) << r1.dump();
	EXPECT_EQ(r1["data"]["participants"].size(), 1u);

	auto r2 = ws2.request("join", {{"roomId", testRoom_}, {"peerId", "p2"}, {"displayName", "p2"}});
	ASSERT_TRUE(r2.value("ok", false)) << r2.dump();
	EXPECT_EQ(r2["data"]["participants"].size(), 2u);

	auto r3 = ws3.request("join", {{"roomId", testRoom_}, {"peerId", "p3"}, {"displayName", "p3"}});
	ASSERT_TRUE(r3.value("ok", false)) << r3.dump();
	EXPECT_EQ(r3["data"]["participants"].size(), 3u);

	auto r4 = ws4.request("join", {{"roomId", testRoom_}, {"peerId", "p4"}, {"displayName", "p4"}});
	ASSERT_TRUE(r4.value("ok", false)) << r4.dump();
	EXPECT_EQ(r4["data"]["participants"].size(), 4u);
}

// ─── Test 6: Error handling - produce on non-existent transport ───
TEST_F(IntegrationTest, ProduceOnBadTransport) {
	auto alice = joinRoom(testRoom_, "alice");

	json rtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 33333333}}}},
		{"mid", "0"}
	};
	auto resp = alice.ws->request("produce", {
		{"transportId", "non-existent-id"}, {"kind", "audio"}, {"rtpParameters", rtpParams}
	});
	EXPECT_FALSE(resp.value("ok", true));
}

// ─── Test 7: Room isolation - peers in different rooms don't see each other ───
TEST_F(IntegrationTest, RoomIsolation) {
	auto alice = joinRoom(testRoom_ + "_A", "alice");
	auto bob = joinRoom(testRoom_ + "_B", "bob");

	// Bob creates recv transport
	bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}});

	// Alice creates send transport and produces
	auto aliceSend = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	json rtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 44444444}}}},
		{"mid", "0"}
	};
	alice.ws->request("produce", {
		{"transportId", aliceSend["data"]["id"]}, {"kind", "audio"}, {"rtpParameters", rtpParams}
	});

	// Bob should NOT get newConsumer (different room)
	auto notif = bob.ws->waitNotification("newConsumer", 1500);
	EXPECT_TRUE(notif.empty()) << "Bob should not receive newConsumer from different room";
}

// ─── Test 8: Auto-subscribe on recvTransport creation (late joiner) ───
TEST_F(IntegrationTest, LateJoinerAutoSubscribe) {
	auto alice = joinRoom(testRoom_, "alice");

	// Alice produces first
	auto aliceSend = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	json rtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 55555555}}}},
		{"mid", "0"}
	};
	auto produceResp = alice.ws->request("produce", {
		{"transportId", aliceSend["data"]["id"]}, {"kind", "audio"}, {"rtpParameters", rtpParams}
	});
	ASSERT_TRUE(produceResp.value("ok", false));

	// Bob joins late
	auto bob = joinRoom(testRoom_, "bob");

	// Bob should see existingProducers in join response
	// (we need to re-check the join response - joinRoom already did it)
	// Instead, Bob creates recvTransport and should get consumers in response
	auto bobRecv = bob.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true}
	});
	ASSERT_TRUE(bobRecv.value("ok", false));
	// The response should contain consumers array with Alice's producer
	ASSERT_TRUE(bobRecv["data"].contains("consumers"));
	auto consumers = bobRecv["data"]["consumers"];
	EXPECT_GE(consumers.size(), 1u);
	if (!consumers.empty()) {
		EXPECT_EQ(consumers[0]["peerId"], "alice");
		EXPECT_EQ(consumers[0]["kind"], "audio");
	}
}

// ═══════════════════════════════════════════════════════════════
// Multi-node tests: two SFU instances sharing Redis
// ═══════════════════════════════════════════════════════════════

static const int SFU_PORT_A = 18765;
static const int SFU_PORT_B = 18766;

class MultiNodeTest : public ::testing::Test {
protected:
	pid_t pidA_ = -1, pidB_ = -1;
	std::string testRoom_;

	static pid_t startSfu(int port) {
		std::string cmd = "./build/mediasoup-sfu"
			" --port=" + std::to_string(port) +
			" --workers=1"
			" --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1"
			" --listenIp=127.0.0.1"
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
		testRoom_ = "route_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

		pidA_ = startSfu(SFU_PORT_A);
		pidB_ = startSfu(SFU_PORT_B);
		ASSERT_GT(pidA_, 0);
		ASSERT_GT(pidB_, 0);
		ASSERT_TRUE(waitForPort(SFU_PORT_A)) << "SFU-A did not start";
		ASSERT_TRUE(waitForPort(SFU_PORT_B)) << "SFU-B did not start";
	}

	void TearDown() override {
		killAndWaitPort(pidA_, SFU_PORT_A);
		killAndWaitPort(pidB_, SFU_PORT_B);
	}
};

// ─── Test 9: Signaling redirect across nodes ───
TEST_F(MultiNodeTest, RedirectToCorrectNode) {
	// Alice joins roomX on SFU-A → claims the room in Redis
	TestWsClient alice;
	ASSERT_TRUE(alice.connect(HOST, SFU_PORT_A));
	auto aliceJoin = alice.request("join", {
		{"roomId", testRoom_}, {"peerId", "alice"}, {"displayName", "alice"}
	});
	ASSERT_TRUE(aliceJoin.value("ok", false)) << "alice join failed: " << aliceJoin.dump();

	// Bob tries to join the SAME room on SFU-B → should get redirect
	TestWsClient bob;
	ASSERT_TRUE(bob.connect(HOST, SFU_PORT_B));
	auto bobJoin = bob.request("join", {
		{"roomId", testRoom_}, {"peerId", "bob"}, {"displayName", "bob"}
	});
	ASSERT_FALSE(bobJoin.value("ok", true)) << "bob should not join on SFU-B";
	ASSERT_TRUE(bobJoin.contains("redirect")) << "expected redirect, got: " << bobJoin.dump();
	std::string redirectAddr = bobJoin["redirect"].get<std::string>();
	EXPECT_NE(redirectAddr.find(std::to_string(SFU_PORT_A)), std::string::npos)
		<< "redirect should point to SFU-A, got: " << redirectAddr;

	// Bob follows redirect: connect to SFU-A and join
	bob.close();
	TestWsClient bob2;
	ASSERT_TRUE(bob2.connect(HOST, SFU_PORT_A));
	auto bob2Join = bob2.request("join", {
		{"roomId", testRoom_}, {"peerId", "bob"}, {"displayName", "bob"}
	});
	ASSERT_TRUE(bob2Join.value("ok", false)) << "bob join on SFU-A failed: " << bob2Join.dump();

	// Alice should receive peerJoined notification
	auto notif = alice.waitNotification("peerJoined", 3000);
	ASSERT_FALSE(notif.empty()) << "Alice did not receive peerJoined after redirect";
	EXPECT_EQ(notif["data"]["peerId"], "bob");
}
