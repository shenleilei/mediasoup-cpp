// Integration (black-box) tests: start real SFU, drive via WebSocket signaling.
#include <gtest/gtest.h>
#include "TestWsClient.h"
#include "TestProcessUtils.h"
#include <signal.h>
#include <sys/wait.h>
#include <sstream>
#include <cmath>

static const int SFU_PORT = 14000;  // use high port to avoid conflicts
static const std::string HOST = "127.0.0.1";

class IntegrationTest : public ::testing::Test {
protected:
	TestSfuProcess sfu_;
	std::string testRoom_; // unique room per test to avoid Redis conflicts

	json defaultRtpCapabilities() const {
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

	void SetUp() override {
		// Generate unique room name per test
		testRoom_ = "room_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

		ASSERT_TRUE(sfu_.start(SFU_PORT, {}, makeTestSfuLogPath("sfu_integration", SFU_PORT)))
			<< "failed to start integration SFU on port " << SFU_PORT
			<< ", log: " << sfu_.logPath();
	}

	void TearDown() override {
		(void)sfu_.stop();
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
		EXPECT_TRUE(c.ws->connect(HOST, sfu_.port()));

		json rtpCaps = defaultRtpCapabilities();

		auto resp = c.ws->request("join", {
			{"roomId", roomId}, {"peerId", peerId},
			{"displayName", peerId}, {"rtpCapabilities", rtpCaps}
		});
		EXPECT_TRUE(resp.value("ok", false)) << "join failed: " << resp.dump();
		if (resp.contains("data") && resp["data"].contains("routerRtpCapabilities"))
			c.routerRtpCapabilities = resp["data"]["routerRtpCapabilities"];
		return c;
	}

	JoinedClient joinRoomWithoutRtpCapabilities(const std::string& roomId, const std::string& peerId) {
		JoinedClient c;
		c.roomId = roomId;
		c.peerId = peerId;
		c.ws = std::make_unique<TestWsClient>();
		EXPECT_TRUE(c.ws->connect(HOST, sfu_.port()));

		auto resp = c.ws->request("join", {
			{"roomId", roomId}, {"peerId", peerId},
			{"displayName", peerId}
		});
		EXPECT_TRUE(resp.value("ok", false)) << "join failed: " << resp.dump();
		if (resp.contains("data") && resp["data"].contains("routerRtpCapabilities"))
			c.routerRtpCapabilities = resp["data"]["routerRtpCapabilities"];
		return c;
	}

	bool waitForFreshRoomReady(int timeoutMs = 5000) {
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
		int attempt = 0;
		while (std::chrono::steady_clock::now() < deadline) {
			TestWsClient ws;
			if (!ws.connect(HOST, sfu_.port())) {
				usleep(100000);
				continue;
			}

			const std::string roomId = testRoom_ + "_respawn_" + std::to_string(++attempt);
			const std::string peerId = "probe_" + std::to_string(attempt);
			auto joinResp = ws.request("join", {
				{"roomId", roomId},
				{"peerId", peerId},
				{"displayName", peerId},
				{"rtpCapabilities", defaultRtpCapabilities()}
			});
			if (joinResp.value("ok", false)) {
				auto transportResp = ws.request("createWebRtcTransport", {
					{"producing", true},
					{"consuming", false}
				});
				if (transportResp.value("ok", false)) {
					ws.close();
					return true;
				}
			}
			ws.close();
			usleep(100000);
		}
		return false;
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

TEST_F(IntegrationTest, CreateTransportRejectsAmbiguousDirection) {
	auto alice = joinRoom(testRoom_, "alice");

	auto resp = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", true}
	});
	EXPECT_FALSE(resp.value("ok", false)) << resp.dump();

	resp = alice.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", false}
	});
	EXPECT_FALSE(resp.value("ok", false)) << resp.dump();
}

TEST_F(IntegrationTest, MalformedWebSocketRequestDoesNotCrashServer) {
	TestWsClient ws;
	ASSERT_TRUE(ws.connect(HOST, sfu_.port()));

	ws.sendRawText(R"({"request":1,"id":7,"method":"join","data":{}})");
	usleep(200000);

	EXPECT_TRUE(isSfuProcessAlive(sfu_.pid())) << "SFU crashed after malformed request";

	auto joinResp = ws.request("join", {
		{"roomId", testRoom_},
		{"peerId", "alice"},
		{"displayName", "alice"},
		{"rtpCapabilities", defaultRtpCapabilities()}
	});
	ASSERT_TRUE(joinResp.value("ok", false)) << joinResp.dump();
}

TEST_F(IntegrationTest, ReplacingSendTransportClosesOldProducers) {
	auto alice = joinRoom(testRoom_, "alice");

	auto firstSend = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(firstSend.value("ok", false)) << firstSend.dump();

	json audioRtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 11111111}}}},
		{"mid", "0"}
	};
	auto firstProduce = alice.ws->request("produce", {
		{"transportId", firstSend["data"]["id"]},
		{"kind", "audio"},
		{"rtpParameters", audioRtpParams}
	});
	ASSERT_TRUE(firstProduce.value("ok", false)) << firstProduce.dump();

	auto secondSend = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(secondSend.value("ok", false)) << secondSend.dump();
	EXPECT_NE(secondSend["data"]["id"], firstSend["data"]["id"]);

	audioRtpParams["encodings"] = {{{"ssrc", 22222222}}};
	auto secondProduce = alice.ws->request("produce", {
		{"transportId", secondSend["data"]["id"]},
		{"kind", "audio"},
		{"rtpParameters", audioRtpParams}
	});
	ASSERT_TRUE(secondProduce.value("ok", false)) << secondProduce.dump();

	auto stats = alice.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(stats.value("ok", false)) << stats.dump();
	ASSERT_TRUE(stats["data"].contains("producers"));
	EXPECT_EQ(stats["data"]["producers"].size(), 1u);
}

TEST_F(IntegrationTest, ReplacingRecvTransportDoesNotDuplicateConsumersAndProducerCloseCleansThemUp) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	auto aliceSend = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(aliceSend.value("ok", false)) << aliceSend.dump();

	json audioRtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 33333333}}}},
		{"mid", "0"}
	};
	auto produceResp = alice.ws->request("produce", {
		{"transportId", aliceSend["data"]["id"]},
		{"kind", "audio"},
		{"rtpParameters", audioRtpParams}
	});
	ASSERT_TRUE(produceResp.value("ok", false)) << produceResp.dump();

	auto firstRecv = bob.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true}
	});
	ASSERT_TRUE(firstRecv.value("ok", false)) << firstRecv.dump();
	ASSERT_TRUE(firstRecv["data"].contains("consumers"));
	EXPECT_EQ(firstRecv["data"]["consumers"].size(), 1u);

	auto secondRecv = bob.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true}
	});
	ASSERT_TRUE(secondRecv.value("ok", false)) << secondRecv.dump();
	ASSERT_TRUE(secondRecv["data"].contains("consumers"));
	EXPECT_EQ(secondRecv["data"]["consumers"].size(), 1u);
	EXPECT_NE(secondRecv["data"]["id"], firstRecv["data"]["id"]);

	auto bobStats = bob.ws->request("getStats", {{"peerId", "bob"}});
	ASSERT_TRUE(bobStats.value("ok", false)) << bobStats.dump();
	ASSERT_TRUE(bobStats["data"].contains("consumers"));
	EXPECT_EQ(bobStats["data"]["consumers"].size(), 1u);

	alice.ws->close();
	auto leftNotif = bob.ws->waitNotification("peerLeft", 3000);
	ASSERT_FALSE(leftNotif.empty()) << "Bob did not receive peerLeft";
	EXPECT_EQ(leftNotif["data"]["peerId"], "alice");
	usleep(200000);

	bobStats = bob.ws->request("getStats", {{"peerId", "bob"}});
	ASSERT_TRUE(bobStats.value("ok", false)) << bobStats.dump();
	ASSERT_TRUE(bobStats["data"].contains("consumers"));
	EXPECT_EQ(bobStats["data"]["consumers"].size(), 0u);
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
		{"transportId", aliceSendId},
		{"kind", "audio"},
		{"rtpParameters", rtpParams},
		{"appData", {{"source", "vehicle-left-door"}}}
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
	ASSERT_TRUE(consumerNotif["data"].contains("appData")) << consumerNotif.dump();
	EXPECT_EQ(consumerNotif["data"]["appData"]["source"], "vehicle-left-door");
}

TEST_F(IntegrationTest, ExplicitConsumeResponseIncludesProducerAppDataSource) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	auto aliceSend = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(aliceSend.value("ok", false)) << aliceSend.dump();

	json rtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 66666666}}}},
		{"mid", "0"}
	};
	auto produceResp = alice.ws->request("produce", {
		{"transportId", aliceSend["data"]["id"]},
		{"kind", "audio"},
		{"rtpParameters", rtpParams},
		{"appData", {{"source", "vehicle-explicit-consume"}}}
	});
	ASSERT_TRUE(produceResp.value("ok", false)) << produceResp.dump();
	ASSERT_TRUE(produceResp["data"].contains("id")) << produceResp.dump();

	auto bobRecv = bob.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true}
	});
	ASSERT_TRUE(bobRecv.value("ok", false)) << bobRecv.dump();
	ASSERT_TRUE(bobRecv["data"].contains("id")) << bobRecv.dump();

	auto consumeResp = bob.ws->request("consume", {
		{"transportId", bobRecv["data"]["id"]},
		{"producerId", produceResp["data"]["id"]},
		{"rtpCapabilities", defaultRtpCapabilities()}
	});
	ASSERT_TRUE(consumeResp.value("ok", false)) << consumeResp.dump();
	ASSERT_TRUE(consumeResp["data"].contains("appData")) << consumeResp.dump();
	EXPECT_EQ(consumeResp["data"]["peerId"], "alice");
	EXPECT_EQ(consumeResp["data"]["producerId"], produceResp["data"]["id"]);
	EXPECT_EQ(consumeResp["data"]["kind"], "audio");
	EXPECT_EQ(consumeResp["data"]["appData"]["source"], "vehicle-explicit-consume");
	EXPECT_TRUE(consumeResp["data"].contains("type")) << consumeResp.dump();
	EXPECT_TRUE(consumeResp["data"].contains("paused")) << consumeResp.dump();
	EXPECT_TRUE(consumeResp["data"].contains("producerPaused")) << consumeResp.dump();
	EXPECT_TRUE(consumeResp["data"].contains("priority")) << consumeResp.dump();
	ASSERT_TRUE(consumeResp["data"].contains("rtpParameters")) << consumeResp.dump();
}

TEST_F(IntegrationTest, AutoSubscribeStillNotifiesWhenSubscriberPrecreatedSendAndRecvWithoutExistingProducers) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoomWithoutRtpCapabilities(testRoom_, "bob");

	auto bobRecv = bob.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true},
		{"rtpCapabilities", defaultRtpCapabilities()}
	});
	ASSERT_TRUE(bobRecv.value("ok", false)) << bobRecv.dump();
	ASSERT_TRUE(bobRecv.contains("data")) << bobRecv.dump();
	ASSERT_TRUE(bobRecv["data"].contains("consumers")) << bobRecv.dump();
	EXPECT_TRUE(bobRecv["data"]["consumers"].is_array()) << bobRecv.dump();
	EXPECT_TRUE(bobRecv["data"]["consumers"].empty()) << bobRecv.dump();

	auto bobSend = bob.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(bobSend.value("ok", false)) << bobSend.dump();

	auto aliceSend = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(aliceSend.value("ok", false)) << aliceSend.dump();

	json audioRtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 77777771}}}},
		{"mid", "0"}
	};
	auto produceResp = alice.ws->request("produce", {
		{"transportId", aliceSend["data"]["id"]},
		{"kind", "audio"},
		{"rtpParameters", audioRtpParams}
	});
	ASSERT_TRUE(produceResp.value("ok", false)) << produceResp.dump();
	ASSERT_TRUE(produceResp["data"].contains("id")) << produceResp.dump();

	auto consumerNotif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(consumerNotif.empty()) << "Bob did not receive newConsumer after precreating send/recv";
	EXPECT_EQ(consumerNotif["data"]["peerId"], "alice");
	EXPECT_EQ(consumerNotif["data"]["producerId"], produceResp["data"]["id"]);
	EXPECT_EQ(consumerNotif["data"]["kind"], "audio");
}

TEST_F(IntegrationTest, AutoSubscribeNotifiesExistingPeerWhenPublisherJoinsLaterAndProduces) {
	auto alice = joinRoomWithoutRtpCapabilities(testRoom_, "alice");

	auto aliceRecv = alice.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true},
		{"rtpCapabilities", defaultRtpCapabilities()}
	});
	ASSERT_TRUE(aliceRecv.value("ok", false)) << aliceRecv.dump();
	ASSERT_TRUE(aliceRecv.contains("data")) << aliceRecv.dump();
	ASSERT_TRUE(aliceRecv["data"].contains("consumers")) << aliceRecv.dump();
	EXPECT_TRUE(aliceRecv["data"]["consumers"].is_array()) << aliceRecv.dump();
	EXPECT_TRUE(aliceRecv["data"]["consumers"].empty()) << aliceRecv.dump();

	auto aliceSend = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(aliceSend.value("ok", false)) << aliceSend.dump();

	auto bob = joinRoom(testRoom_, "bob");
	auto bobSend = bob.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(bobSend.value("ok", false)) << bobSend.dump();

	json audioRtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 77777772}}}},
		{"mid", "0"}
	};
	auto produceResp = bob.ws->request("produce", {
		{"transportId", bobSend["data"]["id"]},
		{"kind", "audio"},
		{"rtpParameters", audioRtpParams}
	});
	ASSERT_TRUE(produceResp.value("ok", false)) << produceResp.dump();
	ASSERT_TRUE(produceResp["data"].contains("id")) << produceResp.dump();

	auto consumerNotif = alice.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(consumerNotif.empty()) << "Alice did not receive newConsumer after Bob joined later and produced";
	EXPECT_EQ(consumerNotif["data"]["peerId"], "bob");
	EXPECT_EQ(consumerNotif["data"]["producerId"], produceResp["data"]["id"]);
	EXPECT_EQ(consumerNotif["data"]["kind"], "audio");
}

TEST_F(IntegrationTest, JoinAndRecvTransportWithoutCapabilitiesStillReceiveNewConsumer) {
	auto alice = joinRoomWithoutRtpCapabilities(testRoom_, "alice");

	auto aliceRecv = alice.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true}
	});
	ASSERT_TRUE(aliceRecv.value("ok", false)) << aliceRecv.dump();
	ASSERT_TRUE(aliceRecv.contains("data")) << aliceRecv.dump();
	ASSERT_TRUE(aliceRecv["data"].contains("consumers")) << aliceRecv.dump();
	EXPECT_TRUE(aliceRecv["data"]["consumers"].is_array()) << aliceRecv.dump();
	EXPECT_TRUE(aliceRecv["data"]["consumers"].empty()) << aliceRecv.dump();

	auto bob = joinRoom(testRoom_, "bob");
	auto bobSend = bob.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(bobSend.value("ok", false)) << bobSend.dump();

	json audioRtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 77777773}}}},
		{"mid", "0"}
	};
	auto produceResp = bob.ws->request("produce", {
		{"transportId", bobSend["data"]["id"]},
		{"kind", "audio"},
		{"rtpParameters", audioRtpParams}
	});
	ASSERT_TRUE(produceResp.value("ok", false)) << produceResp.dump();
	ASSERT_TRUE(produceResp["data"].contains("id")) << produceResp.dump();

	auto consumerNotif = alice.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(consumerNotif.empty()) << "Alice did not receive newConsumer without join/createTransport rtpCapabilities";
	EXPECT_EQ(consumerNotif["data"]["peerId"], "bob");
	EXPECT_EQ(consumerNotif["data"]["producerId"], produceResp["data"]["id"]);
	EXPECT_EQ(consumerNotif["data"]["kind"], "audio");
}

TEST_F(IntegrationTest, RecvTransportRequestCanPopulateRtpCapabilitiesForLaterAutoSubscribe) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoomWithoutRtpCapabilities(testRoom_, "bob");

	auto bobRecv = bob.ws->request("createWebRtcTransport", {
		{"producing", false},
		{"consuming", true},
		{"rtpCapabilities", defaultRtpCapabilities()}
	});
	ASSERT_TRUE(bobRecv.value("ok", false)) << bobRecv.dump();

	auto aliceSend = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(aliceSend.value("ok", false)) << aliceSend.dump();

	json audioRtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2},
			{"payloadType", 100}
		}}},
		{"encodings", {{{"ssrc", 44444444}}}},
		{"mid", "0"}
	};
	auto produceResp = alice.ws->request("produce", {
		{"transportId", aliceSend["data"]["id"]},
		{"kind", "audio"},
		{"rtpParameters", audioRtpParams}
	});
	ASSERT_TRUE(produceResp.value("ok", false)) << produceResp.dump();

	auto consumerNotif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(consumerNotif.empty()) << "Bob did not receive newConsumer";
	EXPECT_EQ(consumerNotif["data"]["peerId"], "alice");
	EXPECT_EQ(consumerNotif["data"]["producerId"], produceResp["data"]["id"]);
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

TEST_F(IntegrationTest, PauseResumeMissingProducerFailsExplicitly) {
	auto alice = joinRoom(testRoom_, "alice");

	auto pauseResp = alice.ws->request("pauseProducer", {{"producerId", "missing-producer"}});
	EXPECT_FALSE(pauseResp.value("ok", true)) << pauseResp.dump();
	EXPECT_EQ(pauseResp.value("error", ""), "producer not found");

	auto resumeResp = alice.ws->request("resumeProducer", {{"producerId", "missing-producer"}});
	EXPECT_FALSE(resumeResp.value("ok", true)) << resumeResp.dump();
	EXPECT_EQ(resumeResp.value("error", ""), "producer not found");
}

// ─── Test 5: Multiple peers, verify participants list ───
TEST_F(IntegrationTest, ParticipantsList) {
	// Join 4 peers sequentially, each should see all previous peers
	TestWsClient ws1, ws2, ws3, ws4;
	ASSERT_TRUE(ws1.connect(HOST, sfu_.port()));
	ASSERT_TRUE(ws2.connect(HOST, sfu_.port()));
	ASSERT_TRUE(ws3.connect(HOST, sfu_.port()));
	ASSERT_TRUE(ws4.connect(HOST, sfu_.port()));

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
		{"transportId", aliceSend["data"]["id"]},
		{"kind", "audio"},
		{"rtpParameters", rtpParams},
		{"appData", {{"source", "vehicle-rear-seat"}}}
	});
	ASSERT_TRUE(produceResp.value("ok", false));

	// Bob joins late
	JoinedClient bob;
	bob.roomId = testRoom_;
	bob.peerId = "bob";
	bob.ws = std::make_unique<TestWsClient>();
	ASSERT_TRUE(bob.ws->connect(HOST, sfu_.port()));
	auto bobJoinResp = bob.ws->request("join", {
		{"roomId", testRoom_},
		{"peerId", "bob"},
		{"displayName", "bob"},
		{"rtpCapabilities", defaultRtpCapabilities()}
	});
	ASSERT_TRUE(bobJoinResp.value("ok", false)) << bobJoinResp.dump();
	ASSERT_TRUE(bobJoinResp["data"].contains("existingProducers")) << bobJoinResp.dump();
	ASSERT_FALSE(bobJoinResp["data"]["existingProducers"].empty()) << bobJoinResp.dump();
	EXPECT_EQ(bobJoinResp["data"]["existingProducers"][0]["producerId"], produceResp["data"]["id"]);
	ASSERT_TRUE(bobJoinResp["data"]["existingProducers"][0].contains("appData")) << bobJoinResp.dump();
	EXPECT_EQ(bobJoinResp["data"]["existingProducers"][0]["appData"]["source"], "vehicle-rear-seat");

	// Bob creates recvTransport and should get existing producer consumers in response.
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
		ASSERT_TRUE(consumers[0].contains("appData")) << bobRecv.dump();
		EXPECT_EQ(consumers[0]["appData"]["source"], "vehicle-rear-seat");
	}
}

// ═══════════════════════════════════════════════════════════════
// Multi-node tests: two SFU instances sharing Redis


// ═══════════════════════════════════════════════════════════════
// Worker crash recovery: kill worker process, verify serverRestart
// ═══════════════════════════════════════════════════════════════

TEST_F(IntegrationTest, WorkerCrashSendsServerRestart) {
	auto alice = joinRoom(testRoom_, "alice");

	auto sendResp = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(sendResp.value("ok", false));
	usleep(500000);

	// Kill only mediasoup-worker children of our test SFU (avoid killing production workers)
	{
		std::string cmd = "pgrep -P " + std::to_string(sfu_.pid()) + " 2>/dev/null";
		FILE* fp = popen(cmd.c_str(), "r");
		char buf[64]{};
		while (fgets(buf, sizeof(buf), fp)) {
			pid_t p = atoi(buf);
			if (p > 0) kill(p, SIGKILL);
		}
		pclose(fp);
	}

	// Alice should receive serverRestart notification (checkRoomHealth runs every 2s)
	auto notif = alice.ws->waitNotification("serverRestart", 10000);
	ASSERT_FALSE(notif.empty()) << "Alice did not receive serverRestart after worker crash";
	EXPECT_EQ(notif["data"]["roomId"], testRoom_);

	ASSERT_TRUE(waitForFreshRoomReady(5000)) << "worker did not become ready after crash";
	auto alice2 = joinRoom(testRoom_ + "_new", "alice2");
}

// ═══════════════════════════════════════════════════════════════
// Worker respawn: after crash, new rooms can be created
// ═══════════════════════════════════════════════════════════════

TEST_F(IntegrationTest, WorkerRespawnAllowsNewRooms) {
	auto alice = joinRoom(testRoom_, "alice");

	// Kill only mediasoup-worker children of our test SFU
	{
		std::string cmd = "pgrep -P " + std::to_string(sfu_.pid()) + " 2>/dev/null";
		FILE* fp = popen(cmd.c_str(), "r");
		char buf[64]{};
		while (fgets(buf, sizeof(buf), fp)) {
			pid_t p = atoi(buf);
			if (p > 0) kill(p, SIGKILL);
		}
		pclose(fp);
	}

	ASSERT_TRUE(waitForFreshRoomReady(6000)) << "worker did not respawn in time";

	// New room should work on the respawned worker
	std::string newRoom = testRoom_ + "_after_crash";
	auto bob = joinRoom(newRoom, "bob");
	auto sendResp = bob.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	EXPECT_TRUE(sendResp.value("ok", false))
		<< "createTransport failed after worker respawn: " << sendResp.dump();
}

// ─── Downlink QoS Phase 1: Consumer control tests ───

TEST_F(IntegrationTest, PauseResumeConsumerControl) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	auto bobRecv = bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}});
	ASSERT_TRUE(bobRecv.value("ok", false));

	auto aliceSend = alice.ws->request("createWebRtcTransport", {{"producing", true}, {"consuming", false}});
	ASSERT_TRUE(aliceSend.value("ok", false));

	json rtpParams = {{"codecs", {{{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2}, {"payloadType", 100}}}},
		{"encodings", {{{"ssrc", 55550001}}}}, {"mid", "0"}};
	auto prod = alice.ws->request("produce", {{"transportId", aliceSend["data"]["id"]}, {"kind", "audio"}, {"rtpParameters", rtpParams}});
	ASSERT_TRUE(prod.value("ok", false));

	auto notif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(notif.empty());
	std::string consumerId = notif["data"]["id"];

	auto pauseResp = bob.ws->request("pauseConsumer", {{"consumerId", consumerId}});
	ASSERT_TRUE(pauseResp.value("ok", false)) << pauseResp.dump();
	EXPECT_TRUE(pauseResp["data"]["paused"].get<bool>());

	auto resumeResp = bob.ws->request("resumeConsumer", {{"consumerId", consumerId}});
	ASSERT_TRUE(resumeResp.value("ok", false)) << resumeResp.dump();
	EXPECT_FALSE(resumeResp["data"]["paused"].get<bool>());
}

TEST_F(IntegrationTest, SetConsumerPriorityControl) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	auto bobRecv = bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}});
	ASSERT_TRUE(bobRecv.value("ok", false));

	auto aliceSend = alice.ws->request("createWebRtcTransport", {{"producing", true}, {"consuming", false}});
	ASSERT_TRUE(aliceSend.value("ok", false));

	json rtpParams = {{"codecs", {{{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2}, {"payloadType", 100}}}},
		{"encodings", {{{"ssrc", 55550002}}}}, {"mid", "0"}};
	auto prod = alice.ws->request("produce", {{"transportId", aliceSend["data"]["id"]}, {"kind", "audio"}, {"rtpParameters", rtpParams}});
	ASSERT_TRUE(prod.value("ok", false));

	auto notif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(notif.empty());
	std::string consumerId = notif["data"]["id"];

	auto resp = bob.ws->request("setConsumerPriority", {{"consumerId", consumerId}, {"priority", 200}});
	ASSERT_TRUE(resp.value("ok", false)) << resp.dump();
	EXPECT_EQ(resp["data"]["priority"].get<uint8_t>(), 200);
}

TEST_F(IntegrationTest, RequestConsumerKeyFrameControl) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	auto bobRecv = bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}});
	ASSERT_TRUE(bobRecv.value("ok", false));

	auto aliceSend = alice.ws->request("createWebRtcTransport", {{"producing", true}, {"consuming", false}});
	ASSERT_TRUE(aliceSend.value("ok", false));

	json rtpParams = {{"codecs", {{{"mimeType", "video/VP8"}, {"clockRate", 90000}, {"payloadType", 101}}}},
		{"encodings", {{{"ssrc", 55550003}}}}, {"mid", "0"}};
	auto prod = alice.ws->request("produce", {{"transportId", aliceSend["data"]["id"]}, {"kind", "video"}, {"rtpParameters", rtpParams}});
	ASSERT_TRUE(prod.value("ok", false));

	auto notif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(notif.empty());
	std::string consumerId = notif["data"]["id"];

	auto resp = bob.ws->request("requestConsumerKeyFrame", {{"consumerId", consumerId}});
	ASSERT_TRUE(resp.value("ok", false)) << resp.dump();
}
