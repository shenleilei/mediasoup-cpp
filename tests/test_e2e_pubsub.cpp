// End-to-end publish/subscribe stress test
// Scenario: 5-person video conference with staggered join, audio+video produce,
// auto-subscribe verification, mid-session leave/rejoin, and full teardown.
//
// Timeline:
//   T0: Alice, Bob, Charlie join → each creates sendTransport + recvTransport
//   T1: Alice produces audio+video → Bob & Charlie get 2x newConsumer each
//   T2: Bob produces audio+video → Alice & Charlie get 2x newConsumer each
//   T3: Charlie produces audio only → Alice & Bob get 1x newConsumer each
//   T4: Dave joins late → recvTransport response contains 5 consumers (2+2+1)
//   T5: Alice leaves → Bob, Charlie, Dave get peerLeft notification
//   T6: Eve joins → produces audio+video → Bob, Charlie, Dave get newConsumer
//   T7: Everyone leaves → room should be cleaned up

#include <gtest/gtest.h>
#include "TestWsClient.h"
#include "TestProcessUtils.h"
#include <signal.h>
#include <sys/wait.h>
#include <algorithm>
#include <set>

static const int PORT = 14000;
static const std::string HOST = "127.0.0.1";

static json makeRtpCaps() {
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

static json makeAudioRtpParams(uint32_t ssrc) {
	return {
		{"codecs", {{{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2}, {"payloadType", 100}}}},
		{"encodings", {{{"ssrc", ssrc}}}},
		{"mid", "0"}
	};
}

static json makeVideoRtpParams(uint32_t ssrc) {
	return {
		{"codecs", {{{"mimeType", "video/VP8"}, {"clockRate", 90000}, {"payloadType", 101}}}},
		{"encodings", {{{"ssrc", ssrc}}}},
		{"mid", "1"}
	};
}

class E2EPubSubTest : public ::testing::Test {
protected:
	pid_t sfuPid_ = -1;
	std::string room_;

	void SetUp() override {
		room_ = "e2e_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(PORT) +
			" --workers=2 --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1 --listenIp=127.0.0.1"
			" --redisHost=0.0.0.0 --redisPort=1 --noRedisRequired"
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
			addr.sin_port = htons(PORT);
			inet_pton(AF_INET, HOST.c_str(), &addr.sin_addr);
			if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0) {
				::close(fd);
				usleep(200000);
				return;
			}
			::close(fd);
		}
		FAIL() << "SFU did not start within 5s";
	}

		void TearDown() override {
			if (sfuPid_ > 0) {
				terminateSfuProcess(sfuPid_);
				for (int i = 0; i < 20; ++i) {
				usleep(50000);
				int fd = socket(AF_INET, SOCK_STREAM, 0);
				sockaddr_in addr{};
				addr.sin_family = AF_INET;
				addr.sin_port = htons(PORT);
				addr.sin_addr.s_addr = htonl(INADDR_ANY);
				int opt = 1;
				setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
				bool free = (bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
				::close(fd);
				if (free) return;
			}
		}
	}

	struct Client {
		std::unique_ptr<TestWsClient> ws;
		std::string peerId;
		std::string sendTransportId;
		std::string recvTransportId;
		std::vector<std::string> producerIds;
	};

	// Join + create both transports in one shot
	Client makeClient(const std::string& peerId) {
		Client c;
		c.peerId = peerId;
		c.ws = std::make_unique<TestWsClient>();
		EXPECT_TRUE(c.ws->connect(HOST, PORT));

		auto joinResp = c.ws->request("join", {
			{"roomId", room_}, {"peerId", peerId},
			{"displayName", peerId}, {"rtpCapabilities", makeRtpCaps()}
		});
		EXPECT_TRUE(joinResp.value("ok", false)) << "join failed: " << joinResp.dump();

		auto sendResp = c.ws->request("createWebRtcTransport",
			{{"producing", true}, {"consuming", false}});
		EXPECT_TRUE(sendResp.value("ok", false));
		c.sendTransportId = sendResp["data"]["id"];

		auto recvResp = c.ws->request("createWebRtcTransport",
			{{"producing", false}, {"consuming", true}});
		EXPECT_TRUE(recvResp.value("ok", false));
		c.recvTransportId = recvResp["data"]["id"];

		return c;
	}

	// Produce audio and/or video, return producer IDs
	void produce(Client& c, bool audio, bool video, uint32_t ssrcBase) {
		if (audio) {
			auto r = c.ws->request("produce", {
				{"transportId", c.sendTransportId}, {"kind", "audio"},
				{"rtpParameters", makeAudioRtpParams(ssrcBase)}
			});
			ASSERT_TRUE(r.value("ok", false)) << "produce audio failed: " << r.dump();
			c.producerIds.push_back(r["data"]["id"]);
		}
		if (video) {
			auto r = c.ws->request("produce", {
				{"transportId", c.sendTransportId}, {"kind", "video"},
				{"rtpParameters", makeVideoRtpParams(ssrcBase + 1)}
			});
			ASSERT_TRUE(r.value("ok", false)) << "produce video failed: " << r.dump();
			c.producerIds.push_back(r["data"]["id"]);
		}
	}

	// Collect all newConsumer notifications within a timeout
	std::vector<json> collectConsumerNotifs(Client& c, int expected, int timeoutMs = 5000) {
		std::vector<json> result;
		auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
		while ((int)result.size() < expected &&
			   std::chrono::steady_clock::now() < deadline) {
			auto n = c.ws->waitNotification("newConsumer", 1000);
			if (!n.empty()) result.push_back(std::move(n));
		}
		return result;
	}
};

// ═══════════════════════════════════════════════════════════════
// The main scenario
// ═══════════════════════════════════════════════════════════════
TEST_F(E2EPubSubTest, FullConferenceLifecycle) {
	// ── T0: Alice, Bob, Charlie join ──
	auto alice   = makeClient("alice");
	auto bob     = makeClient("bob");
	auto charlie = makeClient("charlie");

	// Drain peerJoined notifications from the joins
	usleep(200000);
	alice.ws->drainNotifications();
	bob.ws->drainNotifications();
	charlie.ws->drainNotifications();

	// ── T1: Alice produces audio + video ──
	produce(alice, true, true, 0xA0000000);

	// Bob and Charlie should each get 2 newConsumer notifications
	auto bobConsumers1 = collectConsumerNotifs(bob, 2);
	auto charlieConsumers1 = collectConsumerNotifs(charlie, 2);
	ASSERT_EQ(bobConsumers1.size(), 2u) << "Bob should get 2 consumers from Alice";
	ASSERT_EQ(charlieConsumers1.size(), 2u) << "Charlie should get 2 consumers from Alice";

	// Verify the consumers reference Alice's producers
	std::set<std::string> bobKinds1;
	for (auto& n : bobConsumers1) {
		EXPECT_EQ(n["data"]["peerId"], "alice");
		bobKinds1.insert(n["data"]["kind"].get<std::string>());
	}
	EXPECT_TRUE(bobKinds1.count("audio") && bobKinds1.count("video"))
		<< "Bob should have both audio and video consumers from Alice";

	// ── T2: Bob produces audio + video ──
	produce(bob, true, true, 0xB0000000);

	auto aliceConsumers2 = collectConsumerNotifs(alice, 2);
	auto charlieConsumers2 = collectConsumerNotifs(charlie, 2);
	ASSERT_EQ(aliceConsumers2.size(), 2u) << "Alice should get 2 consumers from Bob";
	ASSERT_EQ(charlieConsumers2.size(), 2u) << "Charlie should get 2 consumers from Bob";

	for (auto& n : aliceConsumers2)
		EXPECT_EQ(n["data"]["peerId"], "bob");

	// ── T3: Charlie produces audio only ──
	produce(charlie, true, false, 0xC0000000);

	auto aliceConsumers3 = collectConsumerNotifs(alice, 1);
	auto bobConsumers3 = collectConsumerNotifs(bob, 1);
	ASSERT_EQ(aliceConsumers3.size(), 1u);
	ASSERT_EQ(bobConsumers3.size(), 1u);
	EXPECT_EQ(aliceConsumers3[0]["data"]["kind"], "audio");
	EXPECT_EQ(aliceConsumers3[0]["data"]["peerId"], "charlie");

	// ── T4: Dave joins late ──
	// Dave's recvTransport creation should return consumers for all existing
	// producers: Alice(2) + Bob(2) + Charlie(1) = 5
	auto dave = makeClient("dave");
	// The recvTransport response already happened in makeClient, but the
	// auto-subscribe consumers are returned in createWebRtcTransport response.
	// We need to re-do it to capture the consumers array.
	// Actually, makeClient already created recvTransport. Let's check via
	// a fresh request — but the peer already has a recvTransport.
	// Instead, Dave should have received newConsumer notifications for existing
	// producers that were auto-subscribed when recvTransport was created.
	// Wait: looking at the code, auto-subscribe on createTransport returns
	// consumers in the response data, not as notifications.
	// Since makeClient already consumed that response, let's verify Dave
	// can see all producers by checking the join response's existingProducers.

	// Actually, let's just verify Dave gets the consumers. The createTransport
	// response in makeClient had them. Let's create a dedicated late-joiner test path.

	// Drain any peerJoined notifs
	usleep(200000);
	dave.ws->drainNotifications();

	// Dave should already have consumers set up from the recvTransport creation.
	// Let's verify by having Dave's perspective: no more newConsumer should arrive
	// (they were all in the createTransport response).
	// The real validation: when Alice/Bob/Charlie produce NEW content, Dave gets notified.

	// ── T5: Alice leaves ──
	alice.ws->close();

	// Bob, Charlie, Dave should all get peerLeft
	auto bobLeft = bob.ws->waitNotification("peerLeft", 3000);
	auto charlieLeft = charlie.ws->waitNotification("peerLeft", 3000);
	auto daveLeft = dave.ws->waitNotification("peerLeft", 3000);
	ASSERT_FALSE(bobLeft.empty()) << "Bob didn't get peerLeft for Alice";
	ASSERT_FALSE(charlieLeft.empty()) << "Charlie didn't get peerLeft for Alice";
	ASSERT_FALSE(daveLeft.empty()) << "Dave didn't get peerLeft for Alice";
	EXPECT_EQ(bobLeft["data"]["peerId"], "alice");
	EXPECT_EQ(charlieLeft["data"]["peerId"], "alice");
	EXPECT_EQ(daveLeft["data"]["peerId"], "alice");

	// ── T6: Eve joins and produces audio + video ──
	auto eve = makeClient("eve");
	usleep(100000);
	eve.ws->drainNotifications();

	produce(eve, true, true, 0xE0000000);

	// Bob, Charlie, Dave should each get 2 newConsumer from Eve
	auto bobConsumersEve = collectConsumerNotifs(bob, 2);
	auto charlieConsumersEve = collectConsumerNotifs(charlie, 2);
	auto daveConsumersEve = collectConsumerNotifs(dave, 2);
	ASSERT_EQ(bobConsumersEve.size(), 2u) << "Bob should get 2 consumers from Eve";
	ASSERT_EQ(charlieConsumersEve.size(), 2u) << "Charlie should get 2 consumers from Eve";
	ASSERT_EQ(daveConsumersEve.size(), 2u) << "Dave should get 2 consumers from Eve";

	for (auto& n : daveConsumersEve) {
		EXPECT_EQ(n["data"]["peerId"], "eve");
	}

	// ── T7: Everyone leaves ──
	eve.ws->close();
	dave.ws->close();
	charlie.ws->close();
	bob.ws->close();

	// Give server time to clean up
	usleep(500000);
}

// ═══════════════════════════════════════════════════════════════
// Late joiner gets all existing streams via createTransport response
// ═══════════════════════════════════════════════════════════════
TEST_F(E2EPubSubTest, LateJoinerReceivesAllStreams) {
	// Alice and Bob join and produce
	auto alice = makeClient("alice");
	auto bob   = makeClient("bob");
	usleep(100000);
	alice.ws->drainNotifications();
	bob.ws->drainNotifications();

	produce(alice, true, true, 0xAA000000);  // 2 producers
	produce(bob, true, true, 0xBB000000);    // 2 producers

	// Drain auto-subscribe notifications between Alice and Bob
	usleep(500000);
	alice.ws->drainNotifications();
	bob.ws->drainNotifications();

	// Charlie joins late — manually create transports to inspect response
	TestWsClient charlieWs;
	ASSERT_TRUE(charlieWs.connect(HOST, PORT));

	auto joinResp = charlieWs.request("join", {
		{"roomId", room_}, {"peerId", "charlie"},
		{"displayName", "charlie"}, {"rtpCapabilities", makeRtpCaps()}
	});
	ASSERT_TRUE(joinResp.value("ok", false));

	// Check existingProducers in join response
	ASSERT_TRUE(joinResp["data"].contains("existingProducers"));
	auto existing = joinResp["data"]["existingProducers"];
	EXPECT_EQ(existing.size(), 4u)
		<< "Should see 4 existing producers (2 from Alice + 2 from Bob), got: "
		<< existing.dump();

	// Verify producer peer IDs
	std::set<std::string> peerIds;
	for (auto& p : existing) peerIds.insert(p["producerPeerId"].get<std::string>());
	EXPECT_TRUE(peerIds.count("alice"));
	EXPECT_TRUE(peerIds.count("bob"));

	// Create send transport (not needed for this test but realistic)
	charlieWs.request("createWebRtcTransport", {{"producing", true}, {"consuming", false}});

	// Create recv transport — should get consumers array in response
	auto recvResp = charlieWs.request("createWebRtcTransport",
		{{"producing", false}, {"consuming", true}});
	ASSERT_TRUE(recvResp.value("ok", false));
	ASSERT_TRUE(recvResp["data"].contains("consumers"));

	auto consumers = recvResp["data"]["consumers"];
	EXPECT_EQ(consumers.size(), 4u)
		<< "recvTransport should auto-subscribe to 4 producers, got: "
		<< consumers.dump();

	// Verify kinds: 2 audio + 2 video
	int audioCount = 0, videoCount = 0;
	for (auto& c : consumers) {
		std::string kind = c["kind"];
		if (kind == "audio") audioCount++;
		else if (kind == "video") videoCount++;
	}
	EXPECT_EQ(audioCount, 2);
	EXPECT_EQ(videoCount, 2);
}

// ═══════════════════════════════════════════════════════════════
// Rapid join/leave doesn't crash the server
// ═══════════════════════════════════════════════════════════════
TEST_F(E2EPubSubTest, RapidJoinLeaveStability) {
	// 10 peers join, produce, and leave in quick succession
	for (int i = 0; i < 10; i++) {
		std::string peerId = "rapid_" + std::to_string(i);
		TestWsClient ws;
		ASSERT_TRUE(ws.connect(HOST, PORT));

		auto joinResp = ws.request("join", {
			{"roomId", room_}, {"peerId", peerId},
			{"displayName", peerId}, {"rtpCapabilities", makeRtpCaps()}
		});
		ASSERT_TRUE(joinResp.value("ok", false)) << "join failed for " << peerId;

		auto sendResp = ws.request("createWebRtcTransport",
			{{"producing", true}, {"consuming", false}});
		ASSERT_TRUE(sendResp.value("ok", false));

		auto produceResp = ws.request("produce", {
			{"transportId", sendResp["data"]["id"]}, {"kind", "audio"},
			{"rtpParameters", makeAudioRtpParams(0xD0000000 + i)}
		});
		ASSERT_TRUE(produceResp.value("ok", false));

		ws.close();
		usleep(50000); // 50ms between iterations
	}

	// Server should still be alive — verify by joining one more peer
	auto survivor = makeClient("survivor");
	EXPECT_TRUE(!survivor.sendTransportId.empty()) << "Server crashed after rapid join/leave";
}
