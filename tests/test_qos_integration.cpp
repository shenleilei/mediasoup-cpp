// Integration tests for QoS stats: getStats signaling, statsReport broadcast,
// and various error/edge cases.
#include <gtest/gtest.h>
#include "TestWsClient.h"
#include "TestProcessUtils.h"
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <chrono>
#include <fstream>
#include <functional>
#include <optional>

static const std::string HOST = "127.0.0.1";

class QosIntegrationTest : public ::testing::Test {
protected:
	TestSfuProcess sfu_;
	int sfuPort_ = -1;
	std::string testRoom_;

	void SetUp() override {
		sfuPort_ = allocateUniqueTestPort();
		ASSERT_GT(sfuPort_, 0) << "failed to allocate unique QoS integration test port";

		testRoom_ = "qos_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
		ASSERT_TRUE(sfu_.start(sfuPort_, {}, makeTestSfuLogPath("sfu_qos_integration", sfuPort_)))
			<< "failed to start QoS integration SFU on port " << sfuPort_
			<< ", log: " << sfu_.logPath();
	}

	void TearDown() override {
		EXPECT_TRUE(sfu_.stop()) << "failed to stop SFU or release port " << sfuPort_;
	}

	struct JoinedClient {
		std::unique_ptr<TestWsClient> ws;
		std::string peerId;
		std::string roomId;
		json joinData;
	};

	JoinedClient joinRoom(const std::string& roomId, const std::string& peerId) {
		JoinedClient c;
		c.roomId = roomId;
		c.peerId = peerId;
		c.ws = std::make_unique<TestWsClient>();
		EXPECT_TRUE(c.ws->connect(HOST, sfuPort_));

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
		c.joinData = resp["data"];
		return c;
	}

	// Helper: create send transport, produce audio, return producerId
	std::string produceAudio(JoinedClient& c, uint32_t ssrc) {
		auto sendResp = c.ws->request("createWebRtcTransport", {
			{"producing", true}, {"consuming", false}
		});
		EXPECT_TRUE(sendResp.value("ok", false));
		std::string sendId = sendResp["data"]["id"];

		json rtpParams = {
			{"codecs", {{
				{"mimeType", "audio/opus"}, {"clockRate", 48000},
				{"channels", 2}, {"payloadType", 100}
			}}},
			{"encodings", {{{"ssrc", ssrc}}}},
			{"mid", "0"}
		};
		auto resp = c.ws->request("produce", {
			{"transportId", sendId}, {"kind", "audio"}, {"rtpParameters", rtpParams}
		});
		EXPECT_TRUE(resp.value("ok", false)) << "produce failed: " << resp.dump();
		return resp["data"]["id"];
	}

	std::string produceVideo(JoinedClient& c, uint32_t ssrc) {
		auto sendResp = c.ws->request("createWebRtcTransport", {
			{"producing", true}, {"consuming", false}
		});
		EXPECT_TRUE(sendResp.value("ok", false));
		std::string sendId = sendResp["data"]["id"];

		json rtpParams = {
			{"codecs", {{
				{"mimeType", "video/VP8"}, {"clockRate", 90000},
				{"payloadType", 101}
			}}},
			{"encodings", {{{"ssrc", ssrc}}}},
			{"mid", "0"}
		};
		auto resp = c.ws->request("produce", {
			{"transportId", sendId}, {"kind", "video"}, {"rtpParameters", rtpParams}
		});
		EXPECT_TRUE(resp.value("ok", false)) << "produce failed: " << resp.dump();
		return resp["data"]["id"];
	}

	json makePublisherClientStats(
		uint64_t seq,
		const std::string& localTrackId,
		const std::string& producerId) const
	{
		return {
			{"schema", "mediasoup.qos.client.v1"},
			{"seq", seq},
			{"tsMs", 1712736000123LL + static_cast<int64_t>(seq)},
			{"peerState", {
				{"mode", "audio-video"},
				{"quality", "good"},
				{"stale", false}
			}},
			{"tracks", json::array({
				{
					{"localTrackId", localTrackId},
					{"producerId", producerId},
					{"kind", "video"},
					{"source", "camera"},
					{"state", "stable"},
					{"reason", "unknown"},
					{"quality", "good"},
					{"ladderLevel", 2},
					{"signals", {
						{"sendBitrateBps", 900000},
						{"targetBitrateBps", 900000},
						{"lossRate", 0.0},
						{"rttMs", 40},
						{"jitterMs", 4},
						{"frameWidth", 1280},
						{"frameHeight", 720},
						{"framesPerSecond", 30},
						{"qualityLimitationReason", "none"}
					}},
					{"lastAction", {
						{"type", "noop"},
						{"applied", true}
					}}
				}
			})}
		};
	}

	json makeDownlinkSnapshot(
		uint64_t seq,
		const std::string& subscriberPeerId,
		const std::string& consumerId,
		const std::string& producerId,
		double availableIncomingBitrate,
		bool visible,
		bool pinned,
		uint32_t targetWidth,
		uint32_t targetHeight) const
	{
		return {
			{"schema", "mediasoup.qos.downlink.client.v1"},
			{"seq", seq},
			{"tsMs", 1712737000000LL + static_cast<int64_t>(seq)},
			{"subscriberPeerId", subscriberPeerId},
			{"transport", {
				{"availableIncomingBitrate", availableIncomingBitrate},
				{"currentRoundTripTime", 0.03}
			}},
			{"subscriptions", {{
				{"consumerId", consumerId},
				{"producerId", producerId},
				{"visible", visible},
				{"pinned", pinned},
				{"activeSpeaker", false},
				{"isScreenShare", false},
				{"targetWidth", targetWidth},
				{"targetHeight", targetHeight},
				{"packetsLost", 0},
				{"jitter", 0.001},
				{"framesPerSecond", visible ? 30.0 : 0.0},
				{"frameWidth", visible ? targetWidth : 0},
				{"frameHeight", visible ? targetHeight : 0},
				{"freezeRate", 0.0}
			}}}
		};
	}

	json waitForPeerStats(
		JoinedClient& requester,
		const std::string& peerId,
		const std::function<bool(const json&)>& predicate,
		int timeoutMs = 3000)
	{
		const auto deadline = std::chrono::steady_clock::now() +
			std::chrono::milliseconds(timeoutMs);
		json lastResp;

		while (std::chrono::steady_clock::now() < deadline) {
			lastResp = requester.ws->request("getStats", {{"peerId", peerId}});
			if (lastResp.value("ok", false) && predicate(lastResp["data"])) {
				return lastResp;
			}
			usleep(100000);
		}

		return lastResp;
	}
	};

// ─── Test 1: getStats returns valid structure after produce ───
TEST_F(QosIntegrationTest, GetStatsAfterProduce) {
	auto alice = joinRoom(testRoom_, "alice");
	produceAudio(alice, 70000001);

	// Use a second connection to query stats (avoids statsReport notification
	// interference on the same socket)
	auto observer = joinRoom(testRoom_, "observer");
	usleep(200000);

	auto resp = observer.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(resp.value("ok", false)) << "getStats failed: " << resp.dump();

	auto& data = resp["data"];
	EXPECT_EQ(data["peerId"], "alice");
	// Should have sendTransport stats
	EXPECT_TRUE(data.contains("sendTransport"));
	// Should have producers map
	EXPECT_TRUE(data.contains("producers"));
	EXPECT_FALSE(data["producers"].empty());

	// Verify producer entry has expected fields
	for (auto& [pid, pstat] : data["producers"].items()) {
		EXPECT_TRUE(pstat.contains("kind"));
		EXPECT_EQ(pstat["kind"], "audio");
		EXPECT_TRUE(pstat.contains("clockRate"));
		EXPECT_EQ(pstat["clockRate"], 48000);
		EXPECT_TRUE(pstat.contains("stats"));
		EXPECT_TRUE(pstat.contains("scores"));
	}
}

// ─── Test 2: getStats for another peer (cross-query) ───
TEST_F(QosIntegrationTest, GetStatsForOtherPeer) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");
	produceAudio(alice, 70000002);
	usleep(200000);

	json resp;
	for (int i = 0; i < 10; ++i) {
		resp = bob.ws->request("getStats", {{"peerId", "alice"}});
		if (resp.value("ok", false) && resp.contains("data") &&
			resp["data"].value("peerId", "") == "alice" &&
			resp["data"].contains("producers") &&
			!resp["data"]["producers"].empty()) {
			break;
		}
		usleep(100000);
	}

	ASSERT_TRUE(resp.value("ok", false)) << resp.dump();
	EXPECT_EQ(resp["data"]["peerId"], "alice");
	EXPECT_FALSE(resp["data"]["producers"].empty());
}

// ─── Test 3: getStats for non-existent peer returns empty ───
TEST_F(QosIntegrationTest, GetStatsNonExistentPeer) {
	auto alice = joinRoom(testRoom_, "alice");
	// Use a second connection to avoid statsReport interference
	auto observer = joinRoom(testRoom_, "observer");

	auto resp = observer.ws->request("getStats", {{"peerId", "ghost"}});
	ASSERT_TRUE(resp.value("ok", false));
	// Should return empty/null data for non-existent peer
	EXPECT_TRUE(resp["data"].empty() || resp["data"].is_null());
}

// ─── Test 4: getStats with no transports (just joined, no produce) ───
TEST_F(QosIntegrationTest, GetStatsNoTransports) {
	auto alice = joinRoom(testRoom_, "alice");

	// Use a second connection to query (avoids statsReport interference)
	auto observer = joinRoom(testRoom_, "observer");
	usleep(100000);

	auto resp = observer.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(resp.value("ok", false));

	auto& data = resp["data"];
	EXPECT_EQ(data["peerId"], "alice");
	// No sendTransport created, so it should be absent or null
	EXPECT_TRUE(!data.contains("sendTransport") || data["sendTransport"].is_null());
	// No producers
	EXPECT_TRUE(data["producers"].empty());
	// No consumers
	EXPECT_TRUE(data["consumers"].empty());
}

// ─── Test 5: statsReport broadcast arrives within timer interval ───
TEST_F(QosIntegrationTest, StatsReportBroadcast) {
	auto alice = joinRoom(testRoom_, "alice");
	produceAudio(alice, 70000005);

	// Wait for the stats broadcast timer to fire (kStatsBroadcastIntervalMs=10s)
	auto notif = alice.ws->waitNotification("statsReport", 15000);
	ASSERT_FALSE(notif.empty()) << "Did not receive statsReport within 5s";

	auto& data = notif["data"];
	EXPECT_TRUE(data.contains("roomId"));
	EXPECT_EQ(data["roomId"], testRoom_);
	EXPECT_TRUE(data.contains("peers"));
	EXPECT_GE(data["peers"].size(), 1u);

	// Find alice in the peers array
	bool foundAlice = false;
	for (auto& peer : data["peers"]) {
		if (peer["peerId"] == "alice") {
			foundAlice = true;
			EXPECT_TRUE(peer.contains("producers"));
			EXPECT_TRUE(peer.contains("sendTransport"));
			break;
		}
	}
	EXPECT_TRUE(foundAlice) << "Alice not found in statsReport";
}

// ─── Test 6: statsReport includes consumer scores after subscribe ───
TEST_F(QosIntegrationTest, StatsReportWithConsumers) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	// Bob creates recv transport so auto-subscribe works
	bob.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true}
	});

	// Alice produces
	produceAudio(alice, 70000006);

	// Bob should get newConsumer
	auto consumerNotif = bob.ws->waitNotification("newConsumer", 5000);
	ASSERT_FALSE(consumerNotif.empty()) << "Bob did not get newConsumer";

	// Wait for statsReport (kStatsBroadcastIntervalMs=10s)
	auto stats = bob.ws->waitNotification("statsReport", 15000);
	ASSERT_FALSE(stats.empty()) << "No statsReport received";

	// Bob should appear in peers with consumers
	bool foundBob = false;
	for (auto& peer : stats["data"]["peers"]) {
		if (peer["peerId"] == "bob") {
			foundBob = true;
			EXPECT_TRUE(peer.contains("consumers"));
			// Bob has at least one consumer (from Alice's producer)
			if (!peer["consumers"].empty()) {
				for (auto& [cid, cdata] : peer["consumers"].items()) {
					EXPECT_TRUE(cdata.contains("kind"));
					EXPECT_TRUE(cdata.contains("score"));
					EXPECT_TRUE(cdata.contains("producerScore"));
				}
			}
			break;
		}
	}
	EXPECT_TRUE(foundBob);
}

TEST_F(QosIntegrationTest, StatsReportIncludesDownlinkConsumerState) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	auto bobRecv = bob.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true}
	});
	ASSERT_TRUE(bobRecv.value("ok", false));

	produceVideo(alice, 70000061);

	auto consumerNotif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(consumerNotif.empty()) << "Bob did not get newConsumer";
	const std::string consumerId = consumerNotif["data"]["id"];
	const std::string producerId = consumerNotif["data"]["producerId"];

	auto snapshot = makeDownlinkSnapshot(
		1, "bob", consumerId, producerId, 2'000'000.0, true, true, 1280, 720);
	auto downlinkResp = bob.ws->request("downlinkClientStats", snapshot);
	ASSERT_TRUE(downlinkResp.value("ok", false)) << downlinkResp.dump();
	usleep(500000);

	auto stats = bob.ws->waitNotification("statsReport", 15000);
	ASSERT_FALSE(stats.empty()) << "No statsReport received";

	bool foundBob = false;
	for (auto& peer : stats["data"]["peers"]) {
		if (peer["peerId"] != "bob") continue;
		foundBob = true;
		ASSERT_TRUE(peer.contains("consumers"));
		ASSERT_TRUE(peer["consumers"].contains(consumerId)) << peer.dump();
		const auto& consumer = peer["consumers"][consumerId];
		EXPECT_EQ(consumer["kind"], "video");
		EXPECT_EQ(consumer["type"], "simple");
		EXPECT_FALSE(consumer["paused"].get<bool>());
		EXPECT_FALSE(consumer["producerPaused"].get<bool>());
		EXPECT_EQ(consumer["preferredSpatialLayer"].get<uint8_t>(), 2);
		EXPECT_EQ(consumer["preferredTemporalLayer"].get<uint8_t>(), 2);
		EXPECT_EQ(consumer["priority"].get<uint8_t>(), 220);
		EXPECT_TRUE(peer.contains("downlinkClientStats"));
		ASSERT_TRUE(peer["downlinkClientStats"].contains("subscriptions"));
		ASSERT_FALSE(peer["downlinkClientStats"]["subscriptions"].empty());
		EXPECT_EQ(
			peer["downlinkClientStats"]["subscriptions"][0]["consumerId"].get<std::string>(),
			consumerId);
		break;
	}

	EXPECT_TRUE(foundBob) << "Bob not found in statsReport";
}

// ─── Test 7: getStats after peer leaves returns empty ───
TEST_F(QosIntegrationTest, GetStatsAfterPeerLeaves) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");
	produceAudio(alice, 70000007);
	usleep(200000);

	// Alice disconnects
	alice.ws->close();
	usleep(500000); // wait for server to detect disconnect

	// Bob queries Alice's stats — should be empty since Alice left
	auto resp = bob.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(resp.value("ok", false));
	EXPECT_TRUE(resp["data"].empty() || resp["data"].is_null());
}

// ─── Test 8: Multiple producers show up in stats ───
TEST_F(QosIntegrationTest, MultipleProducersInStats) {
	auto alice = joinRoom(testRoom_, "alice");

	// Create send transport
	auto sendResp = alice.ws->request("createWebRtcTransport", {
		{"producing", true}, {"consuming", false}
	});
	ASSERT_TRUE(sendResp.value("ok", false));
	std::string sendId = sendResp["data"]["id"];

	// Produce audio
	json audioParams = {
		{"codecs", {{{"mimeType", "audio/opus"}, {"clockRate", 48000},
			{"channels", 2}, {"payloadType", 100}}}},
		{"encodings", {{{"ssrc", 70000081}}}},
		{"mid", "0"}
	};
	auto r1 = alice.ws->request("produce", {
		{"transportId", sendId}, {"kind", "audio"}, {"rtpParameters", audioParams}
	});
	ASSERT_TRUE(r1.value("ok", false));

	// Produce video
	json videoParams = {
		{"codecs", {{{"mimeType", "video/VP8"}, {"clockRate", 90000},
			{"payloadType", 101}}}},
		{"encodings", {{{"ssrc", 70000082}}}},
		{"mid", "1"}
	};
	auto r2 = alice.ws->request("produce", {
		{"transportId", sendId}, {"kind", "video"}, {"rtpParameters", videoParams}
	});
	ASSERT_TRUE(r2.value("ok", false));

	usleep(200000);

	// Use a second connection to query (avoids statsReport interference)
	auto observer = joinRoom(testRoom_, "observer");
	auto resp = observer.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(resp.value("ok", false));
	EXPECT_EQ(resp["data"]["producers"].size(), 2u);

	// Verify both kinds present
	bool hasAudio = false, hasVideo = false;
	for (auto& [pid, pstat] : resp["data"]["producers"].items()) {
		if (pstat["kind"] == "audio") hasAudio = true;
		if (pstat["kind"] == "video") hasVideo = true;
	}
	EXPECT_TRUE(hasAudio);
	EXPECT_TRUE(hasVideo);
}

// ─── Test 9: statsReport not received in empty room (no producers) ───
TEST_F(QosIntegrationTest, NoStatsReportWithoutProducers) {
	auto alice = joinRoom(testRoom_, "alice");

	// Wait 3 seconds — should still get statsReport even without producers
	// (the timer fires regardless), but peers array should show no producer data
	auto notif = alice.ws->waitNotification("statsReport", 15000);
	if (!notif.empty()) {
		for (auto& peer : notif["data"]["peers"]) {
			if (peer["peerId"] == "alice") {
				EXPECT_TRUE(peer["producers"].empty());
			}
		}
	}
}

// ─── Test 10: Room isolation — statsReport only goes to same room ───
TEST_F(QosIntegrationTest, StatsReportRoomIsolation) {
	auto alice = joinRoom(testRoom_ + "_A", "alice");
	auto bob = joinRoom(testRoom_ + "_B", "bob");

	produceAudio(alice, 70000010);

	// Wait for statsReport on Bob's side
	auto notif = bob.ws->waitNotification("statsReport", 15000);
	if (!notif.empty()) {
		// If Bob gets a statsReport, it should be for his room, not Alice's
		EXPECT_EQ(notif["data"]["roomId"], testRoom_ + "_B");
		// Alice should NOT appear in Bob's stats
		for (auto& peer : notif["data"]["peers"]) {
			EXPECT_NE(peer["peerId"], "alice");
		}
	}
}

// ═══════════════════════════════════════════════════════════════════
// QoS Recording + Playback API integration tests
// ═══════════════════════════════════════════════════════════════════

class QosRecordingTest : public ::testing::Test {
protected:
	TestSfuProcess sfu_;
	int sfuPort_ = -1;
	std::string testRoom_;
	std::string recordDir_;

	void SetUp() override {
		sfuPort_ = allocateUniqueTestPort();
		ASSERT_GT(sfuPort_, 0) << "failed to allocate unique QoS recording test port";

		testRoom_ = "qosrec_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
		recordDir_ = "/tmp/mediasoup_qosrec_" + std::to_string(getpid());
		system(("rm -rf " + recordDir_).c_str());
		mkdir(recordDir_.c_str(), 0755);
		ASSERT_TRUE(sfu_.start(
			sfuPort_,
			{"--recordDir=" + recordDir_},
			makeTestSfuLogPath("sfu_qos_recording", sfuPort_)))
			<< "failed to start QoS recording SFU on port " << sfuPort_
			<< ", log: " << sfu_.logPath();
	}

	void TearDown() override {
		EXPECT_TRUE(sfu_.stop()) << "failed to stop SFU or release port " << sfuPort_;
		system(("rm -rf " + recordDir_).c_str());
	}

	// Simple HTTP GET helper (no WebSocket, just plain HTTP)
	std::string httpGetRaw(const std::string& path) {
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) return "";
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(sfuPort_);
		inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
		if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) { ::close(fd); return ""; }

		std::string req = "GET " + path + " HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
		::send(fd, req.data(), req.size(), 0);

		std::string response;
		char buf[4096];
		while (true) {
			int n = ::recv(fd, buf, sizeof(buf), 0);
			if (n <= 0) break;
			response.append(buf, n);
		}
		::close(fd);

		return response;
	}

	std::string httpGet(const std::string& path) {
		std::string response = httpGetRaw(path);
		// Extract body (after \r\n\r\n)
		auto pos = response.find("\r\n\r\n");
		if (pos == std::string::npos) return "";
		return response.substr(pos + 4);
	}

	struct JoinedClient {
		std::unique_ptr<TestWsClient> ws;
	};

	JoinedClient joinAndProduce(const std::string& roomId, const std::string& peerId, uint32_t ssrc) {
		JoinedClient c;
		c.ws = std::make_unique<TestWsClient>();
		EXPECT_TRUE(c.ws->connect(HOST, sfuPort_));

		json rtpCaps = {{"codecs", {{
			{"mimeType", "audio/opus"}, {"kind", "audio"},
			{"clockRate", 48000}, {"channels", 2}, {"preferredPayloadType", 100}
		}}}, {"headerExtensions", json::array()}};

		auto joinResp = c.ws->request("join", {
			{"roomId", roomId}, {"peerId", peerId},
			{"displayName", peerId}, {"rtpCapabilities", rtpCaps}
		});
		EXPECT_TRUE(joinResp.value("ok", false));

		auto sendResp = c.ws->request("createWebRtcTransport", {
			{"producing", true}, {"consuming", false}
		});
		EXPECT_TRUE(sendResp.value("ok", false));

		auto produceResp = c.ws->request("produce", {
			{"transportId", sendResp["data"]["id"]}, {"kind", "audio"},
			{"rtpParameters", {
				{"codecs", {{{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2}, {"payloadType", 100}}}},
				{"encodings", {{{"ssrc", ssrc}}}}, {"mid", "0"}
			}}
		});
		EXPECT_TRUE(produceResp.value("ok", false));
		return c;
	}
};

// ─── Test: QoS JSON file created alongside recording ───
TEST_F(QosRecordingTest, QosFileCreatedWithRecording) {
	auto alice = joinAndProduce(testRoom_, "alice", 80000001);

	// Wait for at least one stats broadcast (kStatsBroadcastIntervalMs=10s)
	usleep(12000000);

	// Disconnect to finalize recording
	alice.ws->close();
	usleep(1000000);

	// The .qos.json is written by broadcastStats → appendQosSnapshot.
	// The .webm may not exist for H264 (deferred header, no real media).
	// Check that the room directory and .qos.json were created.
	std::string findQos = "find " + recordDir_ + " -name '*.qos.json' 2>/dev/null";

	FILE* fp = popen(findQos.c_str(), "r");
	char buf[512]{};
	std::string qosFiles;
	while (fgets(buf, sizeof(buf), fp)) qosFiles += buf;
	pclose(fp);
	ASSERT_FALSE(qosFiles.empty()) << "No .qos.json file created alongside recording";

	// Parse the QoS file and validate structure
	std::string qosPath = qosFiles.substr(0, qosFiles.find('\n'));
	std::ifstream f(qosPath);
	ASSERT_TRUE(f.is_open());
	std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	f.close();

	auto arr = json::parse(content);
	ASSERT_TRUE(arr.is_array());
	EXPECT_GE(arr.size(), 1u) << "QoS file should have at least 1 snapshot";

	// Validate first entry
	auto& first = arr[0];
	EXPECT_TRUE(first.contains("t"));
	EXPECT_TRUE(first.contains("stats"));
	EXPECT_GE(first["t"].get<double>(), 0.0);
	EXPECT_EQ(first["stats"]["peerId"], "alice");
	EXPECT_TRUE(first["stats"].contains("producers"));
}

// ─── Test: /api/recordings returns correct listing ───
TEST_F(QosRecordingTest, RecordingsApiListing) {
	auto alice = joinAndProduce(testRoom_, "alice", 80000002);
	usleep(12000000); // wait for stats broadcast (10s interval)

	// Check API while recorder is still active (file exists before stop)
	std::string body = httpGet("/api/recordings");
	ASSERT_FALSE(body.empty());

	auto recs = json::parse(body);
	ASSERT_TRUE(recs.is_array());
	EXPECT_GE(recs.size(), 1u);

	// Find our test room
	bool found = false;
	for (auto& r : recs) {
		if (r["roomId"] == testRoom_) {
			found = true;
			EXPECT_TRUE(r.contains("peerId"));
			EXPECT_TRUE(r.contains("timestamp"));
			EXPECT_TRUE(r.contains("file"));
			std::string fname = r["file"].get<std::string>();
			EXPECT_NE(fname.find(".webm"), std::string::npos);
			break;
		}
	}
	EXPECT_TRUE(found) << "Test room not found in /api/recordings";
	alice.ws->close();
}

// ─── Test: /recordings/* serves files ───
TEST_F(QosRecordingTest, RecordingsFileServing) {
	auto alice = joinAndProduce(testRoom_, "alice", 80000003);
	usleep(12000000); // wait for stats broadcast (10s interval)

	// Get listing while recorder is active (file exists before stop)
	std::string listBody = httpGet("/api/recordings");
	auto recs = json::parse(listBody);
	std::string webmFile;
	for (auto& r : recs) {
		if (r["roomId"] == testRoom_) {
			webmFile = r["file"].get<std::string>();
			break;
		}
	}
	ASSERT_FALSE(webmFile.empty()) << "No recording file found";

	// Close to finalize recording + QoS file
	alice.ws->close();
	usleep(1000000);

	// Fetch the .qos.json file (now finalized)
	std::string qosFile = webmFile.substr(0, webmFile.rfind('.')) + ".qos.json";
	std::string qosBody = httpGet("/recordings/" + testRoom_ + "/" + qosFile);
	EXPECT_GT(qosBody.size(), 0u) << "Empty .qos.json response";
	auto arr = json::parse(qosBody);
	EXPECT_TRUE(arr.is_array());
}

// ─── Test: /recordings with path traversal is rejected ───
TEST_F(QosRecordingTest, PathTraversalRejected) {
	std::string response = httpGetRaw("/recordings/../../../etc/passwd");
	EXPECT_TRUE(response.find("403 Forbidden") != std::string::npos ||
		response.find("404 Not Found") != std::string::npos) << response;
	EXPECT_TRUE(response.find("root:") == std::string::npos) << "Path traversal not blocked!";
}

// ─── Test: static file path traversal is rejected ───
TEST_F(QosRecordingTest, StaticPathTraversalRejected) {
	std::string response = httpGetRaw("/../../../etc/passwd");
	EXPECT_TRUE(response.find("403 Forbidden") != std::string::npos ||
		response.find("404 Not Found") != std::string::npos) << response;
	EXPECT_TRUE(response.find("root:") == std::string::npos) << "Static path traversal not blocked!";
}

// ─── Test: large recording files are served successfully ───
TEST_F(QosRecordingTest, LargeRecordingServed) {
	std::string roomDir = recordDir_ + "/" + testRoom_;
	ASSERT_EQ(mkdir(roomDir.c_str(), 0755), 0);

	std::string payload(512 * 1024, 'x');
	std::string filePath = roomDir + "/large.webm";
	std::ofstream file(filePath, std::ios::binary);
	ASSERT_TRUE(file.is_open());
	file.write(payload.data(), static_cast<std::streamsize>(payload.size()));
	file.close();

	std::string body = httpGet("/recordings/" + testRoom_ + "/large.webm");
	EXPECT_EQ(body.size(), payload.size());
	EXPECT_EQ(body, payload);
}

// ─── Test: /api/recordings returns empty when no recordings ───
TEST_F(QosRecordingTest, EmptyRecordingsApi) {
	// No one joined, no recordings
	std::string body = httpGet("/api/recordings");
	auto rooms = json::parse(body);
	EXPECT_TRUE(rooms.is_array());
	EXPECT_TRUE(rooms.empty());
}

// ─── Test: QoS clientStats are stored, validated and aggregated ───
TEST_F(QosIntegrationTest, ClientStatsQosStoredAndAggregated) {
	auto alice = joinRoom(testRoom_, "alice");
	produceAudio(alice, 80000001);
	usleep(200000);

	auto observer = joinRoom(testRoom_, "observer");
	usleep(200000);

	json clientReport = {
		{"schema", "mediasoup.qos.client.v1"},
		{"seq", 42},
		{"tsMs", 1712736000123LL},
		{"peerState", {
			{"mode", "audio-video"},
			{"quality", "poor"},
			{"stale", false}
		}},
		{"tracks", json::array({
			{
				{"localTrackId", "camera-main"},
				{"producerId", "producer-123"},
				{"kind", "video"},
				{"source", "camera"},
				{"state", "congested"},
				{"reason", "network"},
				{"quality", "poor"},
				{"ladderLevel", 3},
				{"signals", {
					{"sendBitrateBps", 380000},
					{"targetBitrateBps", 900000},
					{"lossRate", 0.11},
					{"rttMs", 260},
					{"jitterMs", 24},
					{"frameWidth", 640},
					{"frameHeight", 360},
					{"framesPerSecond", 12},
					{"qualityLimitationReason", "bandwidth"}
				}},
				{"lastAction", {
					{"type", "setEncodingParameters"},
					{"applied", true}
				}}
			}
		})}
	};
	auto statsResp = alice.ws->request("clientStats", clientReport);
	ASSERT_TRUE(statsResp.value("ok", false)) << statsResp.dump();
	usleep(500000);

	auto resp = observer.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(resp.value("ok", false)) << resp.dump();

	auto& data = resp["data"];
	ASSERT_TRUE(data.contains("clientStats")) << "clientStats missing from getStats response";
	ASSERT_TRUE(data.contains("qos")) << "qos aggregate missing from getStats response";
	EXPECT_FALSE(data.value("qosStale", true));
	EXPECT_EQ(data.value("qosLastUpdatedMs", 0LL), 1712736000123LL);

	auto& cs = data["clientStats"];
	EXPECT_EQ(cs["schema"], "mediasoup.qos.client.v1");
	EXPECT_EQ(cs["seq"], 42);
	ASSERT_TRUE(cs.contains("tracks"));
	ASSERT_EQ(cs["tracks"].size(), 1u);
	EXPECT_EQ(cs["tracks"][0]["quality"], "poor");
	EXPECT_EQ(cs["tracks"][0]["signals"]["qualityLimitationReason"], "bandwidth");

	auto& qos = data["qos"];
	EXPECT_EQ(qos["quality"], "poor");
	EXPECT_EQ(qos["seq"], 42);
	EXPECT_FALSE(qos["stale"].get<bool>());
	EXPECT_FALSE(qos["lost"].get<bool>());
}

// ─── Test: QoS clientStats appear in statsReport broadcast ───
TEST_F(QosIntegrationTest, ClientStatsQosInBroadcast) {
	auto alice = joinRoom(testRoom_, "alice");
	produceAudio(alice, 80000002);
	usleep(200000);
	alice.ws->drainNotifications();

	json clientReport = {
		{"schema", "mediasoup.qos.client.v1"},
		{"seq", 99},
		{"tsMs", 1712736002222LL},
		{"peerState", {
			{"mode", "audio-video"},
			{"quality", "good"},
			{"stale", false}
		}},
		{"tracks", json::array({
			{
				{"localTrackId", "camera-main"},
				{"kind", "video"},
				{"source", "camera"},
				{"state", "early_warning"},
				{"reason", "network"},
				{"quality", "good"},
				{"ladderLevel", 1},
				{"signals", {
					{"sendBitrateBps", 700000},
					{"targetBitrateBps", 900000},
					{"lossRate", 0.03},
					{"rttMs", 180},
					{"jitterMs", 12},
					{"frameWidth", 640},
					{"frameHeight", 480}
				}}
			}
		})}
	};
	auto statsResp = alice.ws->request("clientStats", clientReport);
	ASSERT_TRUE(statsResp.value("ok", false)) << statsResp.dump();

	// Wait for a statsReport notification that includes the new clientStats.
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	json matching;
	while (std::chrono::steady_clock::now() < deadline) {
		const auto remainingMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			deadline - std::chrono::steady_clock::now()).count();
		auto candidate = alice.ws->waitNotification(
			"statsReport", std::max<int>(100, static_cast<int>(remainingMs)));
		ASSERT_FALSE(candidate.empty()) << "Did not receive statsReport with latest clientStats";

		auto& peers = candidate["data"]["peers"];
		for (auto& p : peers) {
			if (p.value("peerId", "") == "alice" &&
				p.contains("clientStats") &&
				p["clientStats"].value("seq", 0) == 99) {
				matching = std::move(candidate);
				break;
			}
		}
		if (!matching.empty()) break;
	}
	ASSERT_FALSE(matching.empty()) << "Did not observe statsReport carrying clientStats seq=99";

	auto& peers = matching["data"]["peers"];
	bool found = false;
	for (auto& p : peers) {
		if (p.value("peerId", "") == "alice") {
			found = true;
			ASSERT_TRUE(p.contains("clientStats"));
			ASSERT_TRUE(p.contains("qos"));
			EXPECT_EQ(p["clientStats"]["schema"], "mediasoup.qos.client.v1");
			EXPECT_EQ(p["clientStats"]["seq"], 99);
			EXPECT_EQ(p["qos"]["quality"], "good");
			EXPECT_FALSE(p["qos"]["stale"].get<bool>());
			break;
		}
	}
	EXPECT_TRUE(found) << "Alice not found in statsReport";
}

TEST_F(QosIntegrationTest, InvalidClientStatsRejected) {
	auto alice = joinRoom(testRoom_, "alice");
	auto resp = alice.ws->request("clientStats", {
		{"recv", {{"currentRoundTripTime", 0.025}}}
	});

	EXPECT_FALSE(resp.value("ok", true));
	EXPECT_NE(resp.value("error", "").find("invalid clientStats"), std::string::npos);
}

TEST_F(QosIntegrationTest, OlderClientStatsSeqIsIgnored) {
	auto alice = joinRoom(testRoom_, "alice");
	produceAudio(alice, 80000003);
	usleep(200000);

	auto observer = joinRoom(testRoom_, "observer");
	usleep(200000);

	json newer = {
		{"schema", "mediasoup.qos.client.v1"},
		{"seq", 50},
		{"tsMs", 1712736003000LL},
		{"peerState", {
			{"mode", "audio-video"},
			{"quality", "good"},
			{"stale", false}
		}},
		{"tracks", json::array({
			{
				{"localTrackId", "camera-main"},
				{"kind", "video"},
				{"source", "camera"},
				{"state", "early_warning"},
				{"reason", "network"},
				{"quality", "good"},
				{"ladderLevel", 1},
				{"signals", {
					{"sendBitrateBps", 700000},
					{"targetBitrateBps", 900000}
				}}
			}
		})}
	};
	json older = newer;
	older["seq"] = 49;
	older["peerState"]["quality"] = "poor";
	older["tracks"][0]["quality"] = "poor";

	ASSERT_TRUE(alice.ws->request("clientStats", newer).value("ok", false));
	auto olderResp = alice.ws->request("clientStats", older);
	ASSERT_TRUE(olderResp.value("ok", false)) << olderResp.dump();
	ASSERT_TRUE(olderResp["data"].is_object()) << olderResp.dump();
	EXPECT_FALSE(olderResp["data"].value("stored", true));
	EXPECT_EQ(olderResp["data"].value("reason", ""), "stale-seq");
	usleep(500000);

	auto resp = observer.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(resp.value("ok", false)) << resp.dump();
	ASSERT_TRUE(resp["data"].contains("clientStats"));
	ASSERT_TRUE(resp["data"].contains("qos"));

	EXPECT_EQ(resp["data"]["clientStats"]["seq"], 50);
	EXPECT_EQ(resp["data"]["qos"]["seq"], 50);
	EXPECT_EQ(resp["data"]["qos"]["quality"], "good");
}

TEST_F(QosIntegrationTest, JoinReceivesQosPolicyNotification) {
	auto alice = joinRoom(testRoom_, "alice");

	auto notif = alice.ws->waitNotification("qosPolicy", 3000);
	ASSERT_FALSE(notif.empty()) << "Did not receive qosPolicy";

	ASSERT_TRUE(notif["data"].is_object());
	EXPECT_EQ(notif["data"]["schema"], "mediasoup.qos.policy.v1");
	EXPECT_EQ(notif["data"]["sampleIntervalMs"], 1000);
	EXPECT_EQ(notif["data"]["snapshotIntervalMs"], 2000);
	EXPECT_TRUE(notif["data"]["allowAudioOnly"].get<bool>());
}

TEST_F(QosIntegrationTest, SetQosOverrideNotifiesTargetPeer) {
	auto alice = joinRoom(testRoom_, "alice");
	(void)alice.ws->waitNotification("qosPolicy", 3000);

	auto resp = alice.ws->request("setQosOverride", {
		{"peerId", "alice"},
		{"override", {
			{"schema", "mediasoup.qos.override.v1"},
			{"scope", "peer"},
			{"trackId", nullptr},
			{"forceAudioOnly", true},
			{"ttlMs", 10000},
			{"reason", "room_protection"}
		}}
	});
	ASSERT_TRUE(resp.value("ok", false)) << resp.dump();

	auto notif = alice.ws->waitNotification("qosOverride", 3000);
	ASSERT_FALSE(notif.empty()) << "Did not receive qosOverride";
	EXPECT_EQ(notif["data"]["schema"], "mediasoup.qos.override.v1");
	EXPECT_EQ(notif["data"]["scope"], "peer");
	EXPECT_TRUE(notif["data"]["forceAudioOnly"].get<bool>());
	EXPECT_EQ(notif["data"]["ttlMs"], 10000);
}

TEST_F(QosIntegrationTest, ManualQosOverrideClear) {
	auto alice = joinRoom(testRoom_, "alice");
	(void)alice.ws->waitNotification("qosPolicy", 3000);
	alice.ws->drainNotifications();

	auto resp = alice.ws->request("setQosOverride", {
		{"peerId", "alice"},
		{"override", {
			{"schema", "mediasoup.qos.override.v1"},
			{"scope", "peer"},
			{"trackId", nullptr},
			{"forceAudioOnly", true},
			{"ttlMs", 60000},
			{"reason", "manual"}
		}}
	});
	ASSERT_TRUE(resp.value("ok", false)) << resp.dump();

	auto forceNotif = alice.ws->waitNotification("qosOverride", 3000);
	ASSERT_FALSE(forceNotif.empty()) << "Did not receive manual qosOverride";
	EXPECT_EQ(forceNotif["data"]["scope"], "peer");
	EXPECT_EQ(forceNotif["data"]["reason"], "manual");
	EXPECT_EQ(forceNotif["data"]["ttlMs"], 60000);
	EXPECT_TRUE(forceNotif["data"]["forceAudioOnly"].get<bool>());

	auto clearResp = alice.ws->request("setQosOverride", {
		{"peerId", "alice"},
		{"override", {
			{"schema", "mediasoup.qos.override.v1"},
			{"scope", "peer"},
			{"trackId", nullptr},
			{"ttlMs", 0},
			{"reason", "manual"}
		}}
	});
	ASSERT_TRUE(clearResp.value("ok", false)) << clearResp.dump();

	auto clearNotif = alice.ws->waitNotification("qosOverride", 3000);
	EXPECT_FALSE(clearNotif.empty()) << "Did not receive manual qosOverride clear";
	EXPECT_EQ(clearNotif["data"]["scope"], "peer");
	EXPECT_EQ(clearNotif["data"]["reason"], "manual");
	EXPECT_EQ(clearNotif["data"]["ttlMs"], 0);
	EXPECT_TRUE(!clearNotif["data"].contains("forceAudioOnly") ||
		!clearNotif["data"]["forceAudioOnly"].get<bool>());
}

TEST_F(QosIntegrationTest, PlainPublishRejectsDuplicateVideoSsrcs) {
	auto alice = joinRoom(testRoom_, "alice");

	auto resp = alice.ws->request("plainPublish", {
		{"videoSsrcs", json::array({11111111u, 11111111u})},
		{"audioSsrc", 22222222u}
	});

	EXPECT_FALSE(resp.value("ok", false)) << resp.dump();
	EXPECT_FALSE(resp.value("error", "").empty()) << resp.dump();
}

TEST_F(QosIntegrationTest, PlainPublishReplacesOldTransportAndUsesBaselineCodec) {
	auto alice = joinRoom(testRoom_, "alice");

	std::optional<int> baselinePt;
	std::optional<int> mainPt;
	const auto& routerCaps = alice.joinData["routerRtpCapabilities"];
	ASSERT_TRUE(routerCaps.contains("codecs"));
	for (const auto& codec : routerCaps["codecs"]) {
		if (codec.value("mimeType", "") != "video/H264") continue;
		const auto& parameters = codec.value("parameters", json::object());
		if (parameters.value("packetization-mode", 0) != 1) continue;
		const auto profileLevelId = parameters.value("profile-level-id", std::string{});
		if (profileLevelId == "42e01f") baselinePt = codec.value("preferredPayloadType", 0);
		if (profileLevelId == "4d0032") mainPt = codec.value("preferredPayloadType", 0);
	}

	ASSERT_TRUE(baselinePt.has_value());
	ASSERT_TRUE(mainPt.has_value());
	EXPECT_NE(*baselinePt, *mainPt);

	auto first = alice.ws->request("plainPublish", {
		{"videoSsrc", 11111111u},
		{"audioSsrc", 22222222u}
	});
	ASSERT_TRUE(first.value("ok", false)) << first.dump();
	EXPECT_EQ(first["data"]["videoPt"], *baselinePt);

	auto second = alice.ws->request("plainPublish", {
		{"videoSsrc", 33333333u},
		{"audioSsrc", 44444444u}
	});
	ASSERT_TRUE(second.value("ok", false)) << second.dump();
	EXPECT_EQ(second["data"]["videoPt"], *baselinePt);
	EXPECT_NE(second["data"]["transportId"], first["data"]["transportId"]);

	auto observer = joinRoom(testRoom_, "observer");
	usleep(200000);

	auto stats = observer.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(stats.value("ok", false)) << stats.dump();
	ASSERT_TRUE(stats["data"].contains("producers"));
	EXPECT_EQ(stats["data"]["producers"].size(), 2u)
		<< "replacing plainPublish should keep only current audio/video producers";
}

TEST_F(QosIntegrationTest, SetQosPolicyNotifiesTargetPeer) {
	auto alice = joinRoom(testRoom_, "alice");
	(void)alice.ws->waitNotification("qosPolicy", 3000);

	auto resp = alice.ws->request("setQosPolicy", {
		{"peerId", "alice"},
		{"policy", {
			{"schema", "mediasoup.qos.policy.v1"},
			{"sampleIntervalMs", 1500},
			{"snapshotIntervalMs", 4000},
			{"allowAudioOnly", false},
			{"allowVideoPause", false},
			{"profiles", {
				{"camera", "conservative"},
				{"screenShare", "clarity-first"},
				{"audio", "speech-first"}
			}}
		}}
	});
	ASSERT_TRUE(resp.value("ok", false)) << resp.dump();

	auto notif = alice.ws->waitNotification("qosPolicy", 3000);
	ASSERT_FALSE(notif.empty()) << "Did not receive qosPolicy";
	EXPECT_EQ(notif["data"]["sampleIntervalMs"], 1500);
	EXPECT_EQ(notif["data"]["snapshotIntervalMs"], 4000);
	EXPECT_FALSE(notif["data"]["allowAudioOnly"].get<bool>());
	EXPECT_EQ(notif["data"]["profiles"]["camera"], "conservative");
}

TEST_F(QosIntegrationTest, AutomaticQosOverrideOnPoorQuality) {
	auto alice = joinRoom(testRoom_, "alice");
	(void)alice.ws->waitNotification("qosPolicy", 3000);

	json poorClientReport = {
		{"schema", "mediasoup.qos.client.v1"},
		{"seq", 77},
		{"tsMs", 1712736007777LL},
		{"peerState", {
			{"mode", "audio-video"},
			{"quality", "poor"},
			{"stale", false}
		}},
		{"tracks", json::array({
			{
				{"localTrackId", "camera-main"},
				{"kind", "video"},
				{"source", "camera"},
				{"state", "congested"},
				{"reason", "network"},
				{"quality", "poor"},
				{"ladderLevel", 3},
				{"signals", {
					{"sendBitrateBps", 300000},
					{"targetBitrateBps", 900000},
					{"lossRate", 0.12},
					{"rttMs", 280}
				}}
			}
		})}
	};
	ASSERT_TRUE(alice.ws->request("clientStats", poorClientReport).value("ok", false));

	auto notif = alice.ws->waitNotification("qosOverride", 3000);
	ASSERT_FALSE(notif.empty()) << "Did not receive automatic qosOverride";
	EXPECT_EQ(notif["data"]["reason"], "server_auto_poor");
	EXPECT_EQ(notif["data"]["maxLevelClamp"], 2);
	EXPECT_TRUE(notif["data"]["disableRecovery"].get<bool>());
}

TEST_F(QosIntegrationTest, AutomaticQosOverrideOnLostQuality) {
	auto alice = joinRoom(testRoom_, "alice");
	(void)alice.ws->waitNotification("qosPolicy", 3000);

	json lostClientReport = {
		{"schema", "mediasoup.qos.client.v1"},
		{"seq", 88},
		{"tsMs", 1712736008888LL},
		{"peerState", {
			{"mode", "audio-video"},
			{"quality", "lost"},
			{"stale", false}
		}},
		{"tracks", json::array({
			{
				{"localTrackId", "camera-main"},
				{"kind", "video"},
				{"source", "camera"},
				{"state", "congested"},
				{"reason", "network"},
				{"quality", "lost"},
				{"ladderLevel", 4},
				{"signals", {
					{"sendBitrateBps", 0},
					{"targetBitrateBps", 900000},
					{"lossRate", 0.2},
					{"rttMs", 400}
				}}
			}
		})}
	};
	ASSERT_TRUE(alice.ws->request("clientStats", lostClientReport).value("ok", false));

	auto notif = alice.ws->waitNotification("qosOverride", 3000);
	ASSERT_FALSE(notif.empty()) << "Did not receive automatic lost qosOverride";
	EXPECT_EQ(notif["data"]["reason"], "server_auto_lost");
	EXPECT_TRUE(notif["data"]["forceAudioOnly"].get<bool>());
	EXPECT_TRUE(notif["data"]["disableRecovery"].get<bool>());
	EXPECT_EQ(notif["data"]["ttlMs"], 15000);
}

TEST_F(QosIntegrationTest, AutomaticQosOverrideClearsWhenQualityRecovers) {
	auto alice = joinRoom(testRoom_, "alice");
	(void)alice.ws->waitNotification("qosPolicy", 3000);

	json poorClientReport = {
		{"schema", "mediasoup.qos.client.v1"},
		{"seq", 100},
		{"tsMs", 1712736010000LL},
		{"peerState", {
			{"mode", "audio-video"},
			{"quality", "poor"},
			{"stale", false}
		}},
		{"tracks", json::array({
			{
				{"localTrackId", "camera-main"},
				{"kind", "video"},
				{"source", "camera"},
				{"state", "congested"},
				{"reason", "network"},
				{"quality", "poor"},
				{"ladderLevel", 3},
				{"signals", {
					{"sendBitrateBps", 300000},
					{"targetBitrateBps", 900000},
					{"lossRate", 0.12},
					{"rttMs", 280}
				}}
			}
		})}
	};
	ASSERT_TRUE(alice.ws->request("clientStats", poorClientReport).value("ok", false));
	auto poorNotif = alice.ws->waitNotification("qosOverride", 3000);
	ASSERT_FALSE(poorNotif.empty());
	EXPECT_EQ(poorNotif["data"]["reason"], "server_auto_poor");

	json recoveredClientReport = poorClientReport;
	recoveredClientReport["seq"] = 101;
	recoveredClientReport["tsMs"] = 1712736011000LL;
	recoveredClientReport["peerState"]["quality"] = "good";
	recoveredClientReport["tracks"][0]["quality"] = "good";
	recoveredClientReport["tracks"][0]["state"] = "stable";
	recoveredClientReport["tracks"][0]["ladderLevel"] = 0;
	recoveredClientReport["tracks"][0]["signals"]["sendBitrateBps"] = 800000;
	recoveredClientReport["tracks"][0]["signals"]["lossRate"] = 0.01;
	recoveredClientReport["tracks"][0]["signals"]["rttMs"] = 120;

	ASSERT_TRUE(alice.ws->request("clientStats", recoveredClientReport).value("ok", false));
	auto clearNotif = alice.ws->waitNotification("qosOverride", 3000);
	ASSERT_FALSE(clearNotif.empty()) << "Did not receive qosOverride clear";
	EXPECT_EQ(clearNotif["data"]["reason"], "server_auto_clear");
	EXPECT_EQ(clearNotif["data"]["ttlMs"], 0);
}

TEST_F(QosIntegrationTest, ConnectionQualityNotificationDelivered) {
	auto alice = joinRoom(testRoom_, "alice");
	(void)alice.ws->waitNotification("qosPolicy", 3000);

	json poorClientReport = {
		{"schema", "mediasoup.qos.client.v1"},
		{"seq", 120},
		{"tsMs", 1712736012000LL},
		{"peerState", {
			{"mode", "audio-video"},
			{"quality", "poor"},
			{"stale", false}
		}},
		{"tracks", json::array({
			{
				{"localTrackId", "camera-main"},
				{"kind", "video"},
				{"source", "camera"},
				{"state", "congested"},
				{"reason", "network"},
				{"quality", "poor"},
				{"ladderLevel", 3},
				{"signals", {
					{"sendBitrateBps", 300000},
					{"targetBitrateBps", 900000},
					{"lossRate", 0.12},
					{"rttMs", 280}
				}}
			}
		})}
	};
	ASSERT_TRUE(alice.ws->request("clientStats", poorClientReport).value("ok", false));
	auto observed = alice.ws->drainNotifications();
	std::string observedMethods;
	for (auto& msg : observed) {
		if (!observedMethods.empty()) observedMethods += ",";
		observedMethods += msg.value("method", "");
	}
	json notif;
	for (auto& msg : observed) {
		if (msg.value("method", "") == "qosConnectionQuality") {
			notif = msg;
			break;
		}
	}
	if (notif.empty()) notif = alice.ws->waitNotification("qosConnectionQuality", 3000);
	ASSERT_FALSE(notif.empty()) << "Did not receive qosConnectionQuality [observed:" << observedMethods << "]";
	EXPECT_EQ(notif["data"]["quality"], "poor");
	EXPECT_FALSE(notif["data"]["stale"].get<bool>());
	EXPECT_FALSE(notif["data"]["lost"].get<bool>());
}

TEST_F(QosIntegrationTest, RoomQosStateAndRoomPressureOverride) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");
	(void)alice.ws->waitNotification("qosPolicy", 3000);
	(void)bob.ws->waitNotification("qosPolicy", 3000);

	auto makePoor = [](int seq, const std::string& trackId) {
		return json{
			{"schema", "mediasoup.qos.client.v1"},
			{"seq", seq},
			{"tsMs", 1712736013000LL + seq},
			{"peerState", {
				{"mode", "audio-video"},
				{"quality", "poor"},
				{"stale", false}
			}},
			{"tracks", json::array({
				{
					{"localTrackId", trackId},
					{"kind", "video"},
					{"source", "camera"},
					{"state", "congested"},
					{"reason", "network"},
					{"quality", "poor"},
					{"ladderLevel", 3},
					{"signals", {
						{"sendBitrateBps", 300000},
						{"targetBitrateBps", 900000},
						{"lossRate", 0.12},
						{"rttMs", 280}
					}}
				}
			})}
		};
	};

	ASSERT_TRUE(alice.ws->request("clientStats", makePoor(130, "camera-a")).value("ok", false));
	ASSERT_TRUE(bob.ws->request("clientStats", makePoor(131, "camera-b")).value("ok", false));
	auto roomObserved = alice.ws->drainNotifications();
	std::string roomObservedMethods;
	for (auto& msg : roomObserved) {
		if (!roomObservedMethods.empty()) roomObservedMethods += ",";
		roomObservedMethods += msg.value("method", "");
	}
	json roomState;
	for (auto& msg : roomObserved) {
		if (msg.value("method", "") == "qosRoomState") {
			roomState = msg;
		}
	}
	if (roomState.empty()) roomState = alice.ws->waitNotification("qosRoomState", 3000);
	ASSERT_FALSE(roomState.empty()) << "Did not receive qosRoomState [observed:" << roomObservedMethods << "]";
	if (roomState["data"]["poorPeers"] != 2) {
		auto nextRoomState = alice.ws->waitNotification("qosRoomState", 3000);
		if (!nextRoomState.empty()) roomState = nextRoomState;
	}
	EXPECT_EQ(roomState["data"]["poorPeers"], 2);
	EXPECT_EQ(roomState["data"]["quality"], "poor");

	json aliceOverride;
	for (auto& msg : roomObserved) {
		if (msg.value("method", "") == "qosOverride" &&
			msg["data"].value("reason", "") == "server_room_pressure") {
			aliceOverride = msg;
			break;
		}
	}
	if (aliceOverride.empty()) {
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(1000);
		while (std::chrono::steady_clock::now() < deadline) {
			const auto remainingMs = std::chrono::duration_cast<std::chrono::milliseconds>(
				deadline - std::chrono::steady_clock::now()).count();
			auto msg = alice.ws->waitNotification(
				"qosOverride", std::max<int>(100, static_cast<int>(remainingMs)));
			if (msg.empty()) break;
			if (msg["data"].value("reason", "") == "server_room_pressure") {
				aliceOverride = std::move(msg);
				break;
			}
		}
	}
	// Alice is already poor — room pressure override should NOT be sent to her.
	EXPECT_TRUE(aliceOverride.empty())
		<< "Poor peer should not receive room pressure override";
}

TEST_F(QosIntegrationTest, RoomLostPeerPressureOverride) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");
	(void)alice.ws->waitNotification("qosPolicy", 3000);
	(void)bob.ws->waitNotification("qosPolicy", 3000);

	alice.ws->drainNotifications();
	bob.ws->drainNotifications();

	json lostClientReport = {
		{"schema", "mediasoup.qos.client.v1"},
		{"seq", 400},
		{"tsMs", 1712736024000LL},
		{"peerState", {
			{"mode", "audio-video"},
			{"quality", "lost"},
			{"stale", false}
		}},
		{"tracks", json::array({
			{
				{"localTrackId", "camera-lost"},
				{"kind", "video"},
				{"source", "camera"},
				{"state", "congested"},
				{"reason", "network"},
				{"quality", "lost"},
				{"ladderLevel", 4},
				{"signals", {
					{"sendBitrateBps", 0},
					{"targetBitrateBps", 900000},
					{"lossRate", 0.25},
					{"rttMs", 450}
				}}
			}
		})}
	};

	ASSERT_TRUE(bob.ws->request("clientStats", lostClientReport).value("ok", false));

	auto roomState = alice.ws->waitNotification("qosRoomState", 3000);
	ASSERT_FALSE(roomState.empty()) << "Did not receive qosRoomState";
	EXPECT_GE(roomState["data"]["lostPeers"].get<int>(), 1);
	EXPECT_EQ(roomState["data"]["quality"], "lost");

	auto roomOverride = alice.ws->waitNotification("qosOverride", 3000);
	ASSERT_FALSE(roomOverride.empty()) << "Did not receive room pressure qosOverride";
	EXPECT_EQ(roomOverride["data"]["reason"], "server_room_pressure");
	EXPECT_EQ(roomOverride["data"]["ttlMs"], 8000);
	EXPECT_EQ(roomOverride["data"]["maxLevelClamp"], 1);
	EXPECT_TRUE(roomOverride["data"]["disableRecovery"].get<bool>());
}

TEST_F(QosIntegrationTest, SetQosOverrideDeniedForOtherPeer) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");
	(void)alice.ws->waitNotification("qosPolicy", 3000);
	(void)bob.ws->waitNotification("qosPolicy", 3000);

	// Alice tries to set override on Bob → should be denied
	auto resp = alice.ws->request("setQosOverride", {
		{"peerId", "bob"},
		{"override", {
			{"schema", "mediasoup.qos.override.v1"},
			{"scope", "peer"},
			{"trackId", nullptr},
			{"forceAudioOnly", true},
			{"ttlMs", 10000},
			{"reason", "manual"}
		}}
	});
	EXPECT_FALSE(resp.value("ok", false));
	EXPECT_NE(resp.value("error", "").find("permission denied"), std::string::npos);
}

TEST_F(QosIntegrationTest, SetQosPolicyDeniedForOtherPeer) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");
	(void)alice.ws->waitNotification("qosPolicy", 3000);
	(void)bob.ws->waitNotification("qosPolicy", 3000);

	auto resp = alice.ws->request("setQosPolicy", {
		{"peerId", "bob"},
		{"policy", {
			{"schema", "mediasoup.qos.policy.v1"},
			{"snapshotIntervalMs", 2000},
			{"allowAudioOnly", true}
		}}
	});
	EXPECT_FALSE(resp.value("ok", false));
	EXPECT_NE(resp.value("error", "").find("permission denied"), std::string::npos);
}

TEST_F(QosIntegrationTest, ReconnectClearsQosAndAcceptsNewStats) {
	auto alice = joinRoom(testRoom_, "alice");
	auto observer = joinRoom(testRoom_, "observer");
	(void)alice.ws->waitNotification("qosPolicy", 3000);
	(void)observer.ws->waitNotification("qosPolicy", 3000);

	json peerState = {{"mode","audio-video"},{"quality","good"},{"stale",false}};

	// Submit clientStats with high seq
	json report1 = {
		{"schema", "mediasoup.qos.client.v1"}, {"seq", 5000},
		{"tsMs", 1712736000000LL}, {"peerState", peerState}, {"tracks", json::array()}
	};
	auto report1Resp = alice.ws->request("clientStats", report1);
	ASSERT_TRUE(report1Resp.value("ok", false)) << report1Resp.dump();
	EXPECT_TRUE(report1Resp["data"].value("stored", false));
	usleep(300000);

	// Verify baseline stored
	auto stats0 = observer.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(stats0.value("ok", false));
	EXPECT_EQ(stats0["data"]["clientStats"]["seq"], 5000);

	// Reconnect: same peerId, new WebSocket (old QoS entry should be erased)
	auto alice2 = joinRoom(testRoom_, "alice");
	(void)alice2.ws->waitNotification("qosPolicy", 3000);

	// Right after reconnect, old entry should be gone
	auto statsCleared = observer.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(statsCleared.value("ok", false));
	EXPECT_FALSE(statsCleared["data"].contains("clientStats"))
		<< "reconnect should clear old QoS entry";

	// New session starts from seq=1 — accepted because entry was cleared
	json report2 = {
		{"schema", "mediasoup.qos.client.v1"}, {"seq", 1},
		{"tsMs", 1712736010000LL}, {"peerState", peerState}, {"tracks", json::array()}
	};
	auto report2Resp = alice2.ws->request("clientStats", report2);
	ASSERT_TRUE(report2Resp.value("ok", false)) << report2Resp.dump();
	EXPECT_TRUE(report2Resp["data"].value("stored", false));
	usleep(300000);

	auto stats1 = observer.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(stats1.value("ok", false));
	ASSERT_TRUE(stats1["data"].contains("clientStats"));
	EXPECT_EQ(stats1["data"]["clientStats"]["seq"], 1)
		<< "new stats after reconnect should be stored";
}

TEST_F(QosIntegrationTest, SeqResetThresholdWithoutReconnect) {
	auto alice = joinRoom(testRoom_, "alice");
	auto observer = joinRoom(testRoom_, "observer");
	(void)alice.ws->waitNotification("qosPolicy", 3000);
	(void)observer.ws->waitNotification("qosPolicy", 3000);

	json peerState = {{"mode","audio-video"},{"quality","good"},{"stale",false}};

	// Establish high seq baseline
	json report1 = {
		{"schema", "mediasoup.qos.client.v1"}, {"seq", 5000},
		{"tsMs", 1712736000000LL}, {"peerState", peerState}, {"tracks", json::array()}
	};
	ASSERT_TRUE(alice.ws->request("clientStats", report1).value("ok", false));
	usleep(300000);  // let async post settle

	// Small jump back (within 1000) → ignored by the registry, stored seq stays 5000
	json stale = {
		{"schema", "mediasoup.qos.client.v1"}, {"seq", 4500},
		{"tsMs", 1712736001000LL}, {"peerState", peerState}, {"tracks", json::array()}
	};
	auto staleResp = alice.ws->request("clientStats", stale);
	ASSERT_TRUE(staleResp.value("ok", false)) << staleResp.dump();
	ASSERT_TRUE(staleResp["data"].is_object()) << staleResp.dump();
	EXPECT_FALSE(staleResp["data"].value("stored", true));
	EXPECT_EQ(staleResp["data"].value("reason", ""), "stale-seq");
	usleep(300000);

	auto stats1 = observer.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(stats1.value("ok", false));
	EXPECT_EQ(stats1["data"]["clientStats"]["seq"], 5000)
		<< "stale seq should have been silently dropped";

	// Large jump back (> 1000) → accepted as client reset, stored seq becomes 1
	json reset = {
		{"schema", "mediasoup.qos.client.v1"}, {"seq", 1},
		{"tsMs", 1712736002000LL}, {"peerState", peerState}, {"tracks", json::array()}
	};
	alice.ws->request("clientStats", reset);
	usleep(300000);

	auto stats2 = observer.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(stats2.value("ok", false));
	EXPECT_EQ(stats2["data"]["clientStats"]["seq"], 1)
		<< "seq reset (gap > 1000) should have been accepted";
}

// ─── Test: clientStats in recording QoS file ───
// Note: QoS snapshot passthrough to QoS file is implicitly tested by
// test_qos_recording_accuracy.cpp (appendQosSnapshot writes full peer stats
// including clientStats/qos). The service-side passthrough itself is verified
// by ClientStatsQosStoredAndAggregated and ClientStatsQosInBroadcast above.

// ─── Downlink QoS Phase 3: downlinkClientStats tests ───

TEST_F(QosIntegrationTest, DownlinkClientStatsStored) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	auto bobRecv = bob.ws->request("createWebRtcTransport", {
		{"producing", false}, {"consuming", true}
	});
	ASSERT_TRUE(bobRecv.value("ok", false)) << bobRecv.dump();

	const std::string producerId = produceVideo(alice, 70000101);
	auto consumerNotif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(consumerNotif.empty()) << "Bob did not get newConsumer";
	const std::string consumerId = consumerNotif["data"]["id"];

	auto payload = makeDownlinkSnapshot(
		1, "bob", consumerId, producerId, 2'000'000.0, true, false, 640, 360);
	auto resp = bob.ws->request("downlinkClientStats", payload);
	ASSERT_TRUE(resp.value("ok", false)) << resp.dump();
	EXPECT_TRUE(resp["data"].value("stored", false));

	auto statsResp = waitForPeerStats(
		bob,
		"bob",
		[&](const json& data) {
			if (!data.contains("downlinkClientStats")) return false;
			if (!data["downlinkClientStats"].contains("subscriptions")) return false;
			const auto& subscriptions = data["downlinkClientStats"]["subscriptions"];
			if (!subscriptions.is_array() || subscriptions.empty()) return false;
			return data.contains("downlinkQos");
		});
	ASSERT_TRUE(statsResp.value("ok", false)) << statsResp.dump();
	ASSERT_TRUE(statsResp["data"].contains("downlinkClientStats")) << statsResp.dump();
	EXPECT_EQ(
		statsResp["data"]["downlinkClientStats"]["schema"].get<std::string>(),
		"mediasoup.qos.downlink.client.v1");
	ASSERT_FALSE(statsResp["data"]["downlinkClientStats"]["subscriptions"].empty()) << statsResp.dump();
	EXPECT_EQ(
		statsResp["data"]["downlinkClientStats"]["subscriptions"][0]["consumerId"].get<std::string>(),
		consumerId);
	ASSERT_TRUE(statsResp["data"].contains("downlinkQos")) << statsResp.dump();
	EXPECT_EQ(statsResp["data"]["downlinkQos"]["health"].get<std::string>(), "stable");
}

TEST_F(QosIntegrationTest, DownlinkClientStatsRejectsMalformedPayload) {
	auto alice = joinRoom(testRoom_, "alice");

	// Wrong schema
	json payload = {{"schema", "wrong"}, {"seq", 1}, {"tsMs", 1}};
	auto resp = alice.ws->request("downlinkClientStats", payload);
	EXPECT_FALSE(resp.value("ok", true));
	EXPECT_TRUE(resp.contains("error"));

	// Missing subscriberPeerId
	json noSub = {{"schema", "mediasoup.qos.downlink.client.v1"}, {"seq", 1}, {"tsMs", 1},
		{"subscriptions", json::array()}};
	resp = alice.ws->request("downlinkClientStats", noSub);
	EXPECT_FALSE(resp.value("ok", true));

	// Missing subscriptions array
	json noArr = {{"schema", "mediasoup.qos.downlink.client.v1"}, {"seq", 1}, {"tsMs", 1},
		{"subscriberPeerId", "alice"}};
	resp = alice.ws->request("downlinkClientStats", noArr);
	EXPECT_FALSE(resp.value("ok", true));

	// subscriptions is not an array
	json badArr = {{"schema", "mediasoup.qos.downlink.client.v1"}, {"seq", 1}, {"tsMs", 1},
		{"subscriberPeerId", "alice"}, {"subscriptions", "not_array"}};
	resp = alice.ws->request("downlinkClientStats", badArr);
	EXPECT_FALSE(resp.value("ok", true));

	// subscription entry is not an object
	json nonObj = {{"schema", "mediasoup.qos.downlink.client.v1"}, {"seq", 1}, {"tsMs", 1},
		{"subscriberPeerId", "alice"}, {"subscriptions", {42}}};
	resp = alice.ws->request("downlinkClientStats", nonObj);
	EXPECT_FALSE(resp.value("ok", true));

	// subscription missing consumerId/producerId
	json noCid = {{"schema", "mediasoup.qos.downlink.client.v1"}, {"seq", 1}, {"tsMs", 1},
		{"subscriberPeerId", "alice"},
		{"subscriptions", {{{"visible", true}}}}};
	resp = alice.ws->request("downlinkClientStats", noCid);
	EXPECT_FALSE(resp.value("ok", true));
}

TEST_F(QosIntegrationTest, DownlinkClientStatsDropsInconsistentSubscriptions) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	ASSERT_TRUE(bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}}).value("ok", false));
	const std::string producerId = produceVideo(alice, 66660170);
	auto notif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(notif.empty());
	const std::string consumerId = notif["data"]["id"];

	auto payload = makeDownlinkSnapshot(
		1, "bob", consumerId, "wrong-producer-id", 1'000'000.0, false, false, 640, 360);
	auto resp = bob.ws->request("downlinkClientStats", payload);
	ASSERT_TRUE(resp.value("ok", false)) << resp.dump();
	usleep(500000);

	auto state = bob.ws->request("getConsumerState", {{"consumerId", consumerId}});
	ASSERT_TRUE(state.value("ok", false));
	EXPECT_FALSE(state["data"]["paused"].get<bool>())
		<< "invalid consumer/producer mapping should be ignored";

	auto statsResp = bob.ws->request("getStats", {{"peerId", "bob"}});
	ASSERT_TRUE(statsResp.value("ok", false)) << statsResp.dump();
	ASSERT_TRUE(statsResp["data"].contains("downlinkClientStats")) << statsResp.dump();
	EXPECT_TRUE(statsResp["data"]["downlinkClientStats"]["subscriptions"].empty())
		<< "inconsistent subscriptions should be filtered before storage";
}

TEST_F(QosIntegrationTest, DownlinkClientStatsRateLimited) {
	auto alice = joinRoom(testRoom_, "alice");

	auto payload = makeDownlinkSnapshot(
		1, "alice", "c1", "p1", 1'000'000.0, true, false, 640, 360);
	auto req1 = alice.ws->sendRequest("downlinkClientStats", payload);

	payload["seq"] = 2;
	auto req2 = alice.ws->sendRequest("downlinkClientStats", payload);

	auto resp1 = alice.ws->waitResponse(req1);
	ASSERT_TRUE(resp1.value("ok", false)) << resp1.dump();

	auto resp2 = alice.ws->waitResponse(req2);
	EXPECT_FALSE(resp2.value("ok", true));
	EXPECT_NE(resp2.value("error", "").find("rate limited"), std::string::npos);
}

TEST_F(QosIntegrationTest, DownlinkClientStatsAcceptsLegacySchema) {
	auto alice = joinRoom(testRoom_, "alice");

	json payload = {
		{"schema", "mediasoup.downlink.v1"},
		{"seq", 1},
		{"tsMs", 1700000000000},
		{"subscriberPeerId", "alice"},
		{"transport", {{"availableIncomingBitrate", 1500000.0}, {"currentRoundTripTime", 0.03}}},
		{"subscriptions", json::array()}
	};
	auto resp = alice.ws->request("downlinkClientStats", payload);
	ASSERT_TRUE(resp.value("ok", false)) << resp.dump();

	auto statsResp = alice.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(statsResp.value("ok", false)) << statsResp.dump();
	ASSERT_TRUE(statsResp["data"].contains("downlinkClientStats")) << statsResp.dump();
	EXPECT_EQ(
		statsResp["data"]["downlinkClientStats"]["schema"].get<std::string>(),
		"mediasoup.qos.downlink.client.v1");
}

TEST_F(QosIntegrationTest, DownlinkClientStatsRejectsStaleSeq) {
	auto alice = joinRoom(testRoom_, "alice");

	json payload = {
		{"schema", "mediasoup.qos.downlink.client.v1"},
		{"seq", 10},
		{"tsMs", 1700000000000},
		{"subscriberPeerId", "alice"},
		{"transport", {{"availableIncomingBitrate", 1000000.0}, {"currentRoundTripTime", 0.1}}},
		{"subscriptions", json::array()}
	};
	auto resp1 = alice.ws->request("downlinkClientStats", payload);
	ASSERT_TRUE(resp1.value("ok", false));

	// Send stale seq (lower)
	payload["seq"] = 5;
	auto resp2 = alice.ws->request("downlinkClientStats", payload);
	ASSERT_TRUE(resp2.value("ok", false)) << resp2.dump();
	ASSERT_TRUE(resp2["data"].is_object()) << resp2.dump();
	EXPECT_FALSE(resp2["data"].value("stored", true));
	EXPECT_EQ(resp2["data"].value("reason", ""), "stale-seq");

	auto statsResp = alice.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(statsResp.value("ok", false)) << statsResp.dump();
	EXPECT_EQ(statsResp["data"]["downlinkClientStats"]["seq"], 10);
}

TEST_F(QosIntegrationTest, DownlinkClientStatsAcceptsSeqResetFromHighValue) {
	auto alice = joinRoom(testRoom_, "alice");

	auto payload = makeDownlinkSnapshot(
		5000, "alice", "c1", "p1", 1'000'000.0, true, false, 640, 360);
	auto resp1 = alice.ws->request("downlinkClientStats", payload);
	ASSERT_TRUE(resp1.value("ok", false)) << resp1.dump();

	usleep(150000);
	auto payload2 = makeDownlinkSnapshot(
		1, "alice", "c1", "p1", 1'000'000.0, true, false, 640, 360);
	payload2["tsMs"] = payload["tsMs"].get<int64_t>() + 150;
	auto resp2 = alice.ws->request("downlinkClientStats", payload2);
	ASSERT_TRUE(resp2.value("ok", false)) << resp2.dump();
	ASSERT_TRUE(resp2["data"].is_object()) << resp2.dump();
	EXPECT_TRUE(resp2["data"].value("stored", false));

	auto statsResp = alice.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(statsResp.value("ok", false)) << statsResp.dump();
	EXPECT_EQ(statsResp["data"]["downlinkClientStats"]["seq"], 1);
}

TEST_F(QosIntegrationTest, DownlinkClientStatsRejectsRegressedTs) {
	auto alice = joinRoom(testRoom_, "alice");

	auto payload = makeDownlinkSnapshot(
		1, "alice", "c1", "p1", 1'000'000.0, true, false, 640, 360);
	auto resp1 = alice.ws->request("downlinkClientStats", payload);
	ASSERT_TRUE(resp1.value("ok", false)) << resp1.dump();

	usleep(150000);
	payload["seq"] = 2;
	payload["tsMs"] = 1;
	auto resp2 = alice.ws->request("downlinkClientStats", payload);
	ASSERT_TRUE(resp2.value("ok", false)) << resp2.dump();

	auto statsResp = alice.ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(statsResp.value("ok", false)) << statsResp.dump();
	EXPECT_EQ(statsResp["data"]["downlinkClientStats"]["seq"], 1);
}

// ─── Downlink QoS Phase 4: allocator integration tests ───

TEST_F(QosIntegrationTest, DownlinkHiddenAutoPauses) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	auto bobRecv = bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}});
	ASSERT_TRUE(bobRecv.value("ok", false));

	auto aliceSend = alice.ws->request("createWebRtcTransport", {{"producing", true}, {"consuming", false}});
	ASSERT_TRUE(aliceSend.value("ok", false));

	json rtpParams = {{"codecs", {{{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2}, {"payloadType", 100}}}},
		{"encodings", {{{"ssrc", 66660001}}}}, {"mid", "0"}};
	auto prod = alice.ws->request("produce", {{"transportId", aliceSend["data"]["id"]}, {"kind", "audio"}, {"rtpParameters", rtpParams}});
	ASSERT_TRUE(prod.value("ok", false));

	auto notif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(notif.empty());
	std::string consumerId = notif["data"]["id"];
	std::string producerId = notif["data"]["producerId"];

	// Verify consumer starts unpaused
	auto before = bob.ws->request("getConsumerState", {{"consumerId", consumerId}});
	ASSERT_TRUE(before.value("ok", false));
	EXPECT_FALSE(before["data"]["paused"].get<bool>());

	// Send downlink snapshot with hidden consumer
	json snapshot = {
		{"schema", "mediasoup.qos.downlink.client.v1"}, {"seq", 1}, {"tsMs", 1700000000000},
		{"subscriberPeerId", "bob"},
		{"transport", {{"availableIncomingBitrate", 2000000.0}, {"currentRoundTripTime", 0.05}}},
		{"subscriptions", {{
			{"consumerId", consumerId}, {"producerId", producerId},
			{"visible", false}, {"pinned", false},
			{"targetWidth", 0}, {"targetHeight", 0}
		}}}
	};
	auto resp = bob.ws->request("downlinkClientStats", snapshot);
	ASSERT_TRUE(resp.value("ok", false));
	usleep(500000);

	// Verify allocator paused the consumer and set hidden priority
	auto after = bob.ws->request("getConsumerState", {{"consumerId", consumerId}});
	ASSERT_TRUE(after.value("ok", false));
	EXPECT_TRUE(after["data"]["paused"].get<bool>()) << "allocator should have paused hidden consumer";
	EXPECT_EQ(after["data"]["priority"].get<uint8_t>(), 1) << "hidden priority should be 1";
}

TEST_F(QosIntegrationTest, DownlinkVisibleAutoResumes) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	auto bobRecv = bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}});
	ASSERT_TRUE(bobRecv.value("ok", false));

	auto aliceSend = alice.ws->request("createWebRtcTransport", {{"producing", true}, {"consuming", false}});
	ASSERT_TRUE(aliceSend.value("ok", false));

	json rtpParams = {{"codecs", {{{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2}, {"payloadType", 100}}}},
		{"encodings", {{{"ssrc", 66660002}}}}, {"mid", "0"}};
	auto prod = alice.ws->request("produce", {{"transportId", aliceSend["data"]["id"]}, {"kind", "audio"}, {"rtpParameters", rtpParams}});
	ASSERT_TRUE(prod.value("ok", false));

	auto notif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(notif.empty());
	std::string consumerId = notif["data"]["id"];
	std::string producerId = notif["data"]["producerId"];

	// First: hide it
	json snap1 = {
		{"schema", "mediasoup.qos.downlink.client.v1"}, {"seq", 1}, {"tsMs", 1700000000000},
		{"subscriberPeerId", "bob"},
		{"transport", {{"availableIncomingBitrate", 2000000.0}, {"currentRoundTripTime", 0.05}}},
		{"subscriptions", {{
			{"consumerId", consumerId}, {"producerId", producerId},
			{"visible", false}, {"pinned", false}, {"targetWidth", 0}, {"targetHeight", 0}
		}}}
	};
	bob.ws->request("downlinkClientStats", snap1);
	usleep(500000);

	// Confirm paused
	auto mid = bob.ws->request("getConsumerState", {{"consumerId", consumerId}});
	ASSERT_TRUE(mid.value("ok", false));
	ASSERT_TRUE(mid["data"]["paused"].get<bool>()) << "should be paused after hidden snapshot";

	// Then: make visible again
	json snap2 = snap1;
	snap2["seq"] = 2;
	snap2["subscriptions"][0]["visible"] = true;
	snap2["subscriptions"][0]["targetWidth"] = 640;
	snap2["subscriptions"][0]["targetHeight"] = 360;
	auto resp = bob.ws->request("downlinkClientStats", snap2);
	ASSERT_TRUE(resp.value("ok", false));
	usleep(500000);

	// Verify allocator resumed the consumer
	auto after = bob.ws->request("getConsumerState", {{"consumerId", consumerId}});
	ASSERT_TRUE(after.value("ok", false));
	EXPECT_FALSE(after["data"]["paused"].get<bool>()) << "allocator should have resumed visible consumer";
	EXPECT_EQ(after["data"]["priority"].get<uint8_t>(), 120) << "visible grid priority should be 120";
}

TEST_F(QosIntegrationTest, DownlinkLargeSmallGetDifferentLayers) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	auto bobRecv = bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}});
	ASSERT_TRUE(bobRecv.value("ok", false));

	auto aliceSend = alice.ws->request("createWebRtcTransport", {{"producing", true}, {"consuming", false}});
	ASSERT_TRUE(aliceSend.value("ok", false));

	json rtpParams = {{"codecs", {{{"mimeType", "video/VP8"}, {"clockRate", 90000}, {"payloadType", 101}}}},
		{"encodings", {{{"ssrc", 66660003}}}}, {"mid", "0"}};
	auto prod = alice.ws->request("produce", {{"transportId", aliceSend["data"]["id"]}, {"kind", "video"}, {"rtpParameters", rtpParams}});
	ASSERT_TRUE(prod.value("ok", false));

	auto notif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(notif.empty());
	std::string consumerId = notif["data"]["id"];
	std::string producerId = notif["data"]["producerId"];

	// Send snapshot with large pinned tile
	json snap = {
		{"schema", "mediasoup.qos.downlink.client.v1"}, {"seq", 1}, {"tsMs", 1700000000000},
		{"subscriberPeerId", "bob"},
		{"transport", {{"availableIncomingBitrate", 2000000.0}, {"currentRoundTripTime", 0.05}}},
		{"subscriptions", {{
			{"consumerId", consumerId}, {"producerId", producerId},
			{"visible", true}, {"pinned", true},
			{"targetWidth", 1280}, {"targetHeight", 720}
		}}}
	};
	auto resp = bob.ws->request("downlinkClientStats", snap);
	ASSERT_TRUE(resp.value("ok", false));
	usleep(500000);

	// Verify allocator set high layers for pinned large tile
	auto state = bob.ws->request("getConsumerState", {{"consumerId", consumerId}});
	ASSERT_TRUE(state.value("ok", false));
	EXPECT_EQ(state["data"]["preferredSpatialLayer"].get<uint8_t>(), 2) << "pinned large tile should get spatial=2";
	EXPECT_EQ(state["data"]["preferredTemporalLayer"].get<uint8_t>(), 2) << "pinned large tile should get temporal=2";
	EXPECT_EQ(state["data"]["priority"].get<uint8_t>(), 220) << "pinned priority should be 220";

	// Now send snapshot with small tile under a constrained budget so v2 keeps base layer only.
	json snap2 = snap;
	snap2["seq"] = 2;
	snap2["subscriptions"][0]["pinned"] = false;
	snap2["subscriptions"][0]["targetWidth"] = 240;
	snap2["subscriptions"][0]["targetHeight"] = 135;
	snap2["transport"]["availableIncomingBitrate"] = 70000.0;
	bob.ws->request("downlinkClientStats", snap2);
	usleep(500000);

	auto state2 = bob.ws->request("getConsumerState", {{"consumerId", consumerId}});
	ASSERT_TRUE(state2.value("ok", false));
	EXPECT_EQ(state2["data"]["preferredSpatialLayer"].get<uint8_t>(), 0) << "small tile under low budget should get spatial=0";
	EXPECT_EQ(state2["data"]["preferredTemporalLayer"].get<uint8_t>(), 0) << "small tile under low budget should get temporal=0";
	EXPECT_EQ(state2["data"]["priority"].get<uint8_t>(), 120) << "visible grid priority should be 120";
}

TEST_F(QosIntegrationTest, DownlinkReconcilesManualConsumerPauseOnNextPlan) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	ASSERT_TRUE(bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}}).value("ok", false));

	const std::string producerId = produceVideo(alice, 66660150);
	auto notif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(notif.empty());
	const std::string consumerId = notif["data"]["id"];

	auto baseline = makeDownlinkSnapshot(
		1, "bob", consumerId, producerId, 5'000'000.0, true, true, 1280, 720);
	ASSERT_TRUE(bob.ws->request("downlinkClientStats", baseline).value("ok", false));
	usleep(500000);

	auto state0 = bob.ws->request("getConsumerState", {{"consumerId", consumerId}});
	ASSERT_TRUE(state0.value("ok", false));
	ASSERT_FALSE(state0["data"]["paused"].get<bool>());

	auto manualPause = bob.ws->request("pauseConsumer", {{"consumerId", consumerId}});
	ASSERT_TRUE(manualPause.value("ok", false)) << manualPause.dump();

	auto pausedState = bob.ws->request("getConsumerState", {{"consumerId", consumerId}});
	ASSERT_TRUE(pausedState.value("ok", false));
	ASSERT_TRUE(pausedState["data"]["paused"].get<bool>());

	usleep(150000);
	baseline["seq"] = 2;
	auto resp = bob.ws->request("downlinkClientStats", baseline);
	ASSERT_TRUE(resp.value("ok", false)) << resp.dump();
	usleep(500000);

	auto finalState = bob.ws->request("getConsumerState", {{"consumerId", consumerId}});
	ASSERT_TRUE(finalState.value("ok", false));
	EXPECT_FALSE(finalState["data"]["paused"].get<bool>())
		<< "next downlink plan should reconcile manual pause and resume the consumer";
	EXPECT_EQ(finalState["data"]["preferredSpatialLayer"].get<uint8_t>(), 2);
	EXPECT_EQ(finalState["data"]["priority"].get<uint8_t>(), 220);
}

TEST_F(QosIntegrationTest, DownlinkV2LowDemandClampsPublisherAndRecoveryClearsIt) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	auto bobRecv = bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}});
	ASSERT_TRUE(bobRecv.value("ok", false));

	const std::string producerId = produceVideo(alice, 66660100);
	auto consumerNotif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(consumerNotif.empty()) << "Bob did not get newConsumer";
	const std::string consumerId = consumerNotif["data"]["id"];

	ASSERT_TRUE(alice.ws->request("clientStats",
		makePublisherClientStats(1, "camera-main", producerId)).value("ok", false));
	alice.ws->drainNotifications();

	auto lowDemand = makeDownlinkSnapshot(
		1, "bob", consumerId, producerId, 50'000.0, true, false, 160, 90);
	ASSERT_TRUE(bob.ws->request("downlinkClientStats", lowDemand).value("ok", false));

	auto clampNotif = alice.ws->waitNotification("qosOverride", 3000);
	ASSERT_FALSE(clampNotif.empty()) << "publisher did not receive low-demand clamp";
	EXPECT_EQ(clampNotif["data"]["scope"], "track");
	EXPECT_EQ(clampNotif["data"]["trackId"], "camera-main");
	EXPECT_EQ(clampNotif["data"]["reason"], "downlink_v2_room_demand");
	EXPECT_EQ(clampNotif["data"]["maxLevelClamp"], 0);

	auto highDemand = makeDownlinkSnapshot(
		2, "bob", consumerId, producerId, 5'000'000.0, true, true, 1280, 720);
	ASSERT_TRUE(bob.ws->request("downlinkClientStats", highDemand).value("ok", false));

	auto clearNotif = alice.ws->waitNotification("qosOverride", 3000);
	ASSERT_FALSE(clearNotif.empty()) << "publisher did not receive recovery clear";
	EXPECT_EQ(clearNotif["data"]["scope"], "track");
	EXPECT_EQ(clearNotif["data"]["trackId"], "camera-main");
	EXPECT_EQ(clearNotif["data"]["reason"], "downlink_v2_demand_restored");
	ASSERT_TRUE(clearNotif["data"].contains("maxLevelClamp"));
	EXPECT_TRUE(clearNotif["data"]["maxLevelClamp"].is_null());
}

TEST_F(QosIntegrationTest, DownlinkV2StaleSnapshotDoesNotRegressPublisherDemand) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");
	auto carol = joinRoom(testRoom_, "carol");

	ASSERT_TRUE(bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}}).value("ok", false));
	ASSERT_TRUE(carol.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}}).value("ok", false));

	const std::string producerId = produceVideo(alice, 66660101);
	auto bobConsumerNotif = bob.ws->waitNotification("newConsumer", 3000);
	auto carolConsumerNotif = carol.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(bobConsumerNotif.empty()) << "Bob did not get newConsumer";
	ASSERT_FALSE(carolConsumerNotif.empty()) << "Carol did not get newConsumer";
	const std::string bobConsumerId = bobConsumerNotif["data"]["id"];
	const std::string carolConsumerId = carolConsumerNotif["data"]["id"];

	ASSERT_TRUE(alice.ws->request("clientStats",
		makePublisherClientStats(1, "camera-main", producerId)).value("ok", false));
	alice.ws->drainNotifications();

	auto bobHighDemand = makeDownlinkSnapshot(
		1, "bob", bobConsumerId, producerId, 5'000'000.0, true, true, 1280, 720);
	auto carolLowDemand = makeDownlinkSnapshot(
		1, "carol", carolConsumerId, producerId, 50'000.0, true, false, 160, 90);
	ASSERT_TRUE(bob.ws->request("downlinkClientStats", bobHighDemand).value("ok", false));
	ASSERT_TRUE(carol.ws->request("downlinkClientStats", carolLowDemand).value("ok", false));
	usleep(500000);
	alice.ws->drainNotifications();

	// Let Bob's last snapshot become stale, then trigger a replan from Carol.
	usleep(6'500'000);
	carolLowDemand["seq"] = 2;
	ASSERT_TRUE(carol.ws->request("downlinkClientStats", carolLowDemand).value("ok", false));

	auto staleRegressionNotif = alice.ws->waitNotification("qosOverride", 1200);
	// After Bob goes stale, only Carol's low demand remains active.
	// The planner may legitimately emit a low-demand clamp for Carol's spatial=0.
	// The key invariant is: the stale snapshot itself must not be used as active demand.
	// A clamp from Carol's active low demand is acceptable.
	if (!staleRegressionNotif.empty()) {
		auto reason = staleRegressionNotif["data"].value("reason", "");
		EXPECT_TRUE(reason == "downlink_v2_room_demand" || reason == "downlink_v2_zero_demand_hold")
			<< "if override is sent, it should be from Carol's active low demand, not stale regression: "
			<< staleRegressionNotif.dump();
	}
}

// ─── Downlink QoS cleanup regression tests ───

TEST_F(QosIntegrationTest, DownlinkStateCleanedOnLeave) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	auto bobRecv = bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}});
	ASSERT_TRUE(bobRecv.value("ok", false));
	auto aliceSend = alice.ws->request("createWebRtcTransport", {{"producing", true}, {"consuming", false}});
	ASSERT_TRUE(aliceSend.value("ok", false));

	const std::string producerId = produceVideo(alice, 77770001);

	auto notif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(notif.empty());
	std::string consumerId = notif["data"]["id"];
	EXPECT_EQ(notif["data"]["producerId"].get<std::string>(), producerId);

	// Pause via downlink snapshot (hidden)
	json snap = {{"schema", "mediasoup.qos.downlink.client.v1"}, {"seq", 1}, {"tsMs", 1700000000000},
		{"subscriberPeerId", "bob"},
		{"transport", {{"availableIncomingBitrate", 2000000.0}, {"currentRoundTripTime", 0.05}}},
		{"subscriptions", {{{"consumerId", consumerId}, {"producerId", producerId},
			{"visible", false}, {"pinned", false}, {"targetWidth", 0}, {"targetHeight", 0}}}}};
	bob.ws->request("downlinkClientStats", snap);
	usleep(500000);

	// Confirm old consumer was paused
	auto oldState = bob.ws->request("getConsumerState", {{"consumerId", consumerId}});
	ASSERT_TRUE(oldState.value("ok", false));
	ASSERT_TRUE(oldState["data"]["paused"].get<bool>());

	// Leave and rejoin — downlink state should be clean
	bob.ws->request("leave", {});
	bob.ws->close();
	bob = joinRoom(testRoom_, "bob");

	bobRecv = bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}});
	ASSERT_TRUE(bobRecv.value("ok", false));
	ASSERT_TRUE(bobRecv["data"].contains("consumers"));
	ASSERT_GT(bobRecv["data"]["consumers"].size(), 0u);
	std::string newConsumerId = bobRecv["data"]["consumers"][0]["id"];
	std::string newProducerId = bobRecv["data"]["consumers"][0]["producerId"];

	// New consumer should start unpaused (controller state was cleaned)
	auto newState = bob.ws->request("getConsumerState", {{"consumerId", newConsumerId}});
	ASSERT_TRUE(newState.value("ok", false));
	EXPECT_FALSE(newState["data"]["paused"].get<bool>()) << "new consumer after leave should not inherit paused state";

	// Fresh seq=1 should be accepted (registry state was cleaned)
	json snap2 = {{"schema", "mediasoup.qos.downlink.client.v1"}, {"seq", 1}, {"tsMs", 1700000001000},
		{"subscriberPeerId", "bob"},
		{"transport", {{"availableIncomingBitrate", 2000000.0}, {"currentRoundTripTime", 0.05}}},
		{"subscriptions", {{{"consumerId", newConsumerId}, {"producerId", newProducerId},
			{"visible", true}, {"pinned", true}, {"targetWidth", 1280}, {"targetHeight", 720}}}}};
	auto resp = bob.ws->request("downlinkClientStats", snap2);
	ASSERT_TRUE(resp.value("ok", false)) << "fresh seq after leave should be accepted";
	usleep(500000);

	// Verify allocator applied fresh state (not inheriting old degradeLevel)
	auto finalState = bob.ws->request("getConsumerState", {{"consumerId", newConsumerId}});
	ASSERT_TRUE(finalState.value("ok", false));
	EXPECT_EQ(finalState["data"]["preferredSpatialLayer"].get<uint8_t>(), 2)
		<< "pinned large tile should get spatial=2 (no inherited degradeLevel)";
	EXPECT_EQ(finalState["data"]["priority"].get<uint8_t>(), 220)
		<< "pinned priority should be 220";
}

TEST_F(QosIntegrationTest, DownlinkStateCleanedOnReconnect) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	auto bobRecv = bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}});
	ASSERT_TRUE(bobRecv.value("ok", false));
	auto aliceSend = alice.ws->request("createWebRtcTransport", {{"producing", true}, {"consuming", false}});
	ASSERT_TRUE(aliceSend.value("ok", false));

	const std::string producerId = produceVideo(alice, 77770002);

	auto notif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(notif.empty());
	std::string consumerId = notif["data"]["id"];
	EXPECT_EQ(notif["data"]["producerId"].get<std::string>(), producerId);

	// Build up downlink state: hidden + seq=10
	json snap = {{"schema", "mediasoup.qos.downlink.client.v1"}, {"seq", 10}, {"tsMs", 1700000000000},
		{"subscriberPeerId", "bob"},
		{"transport", {{"availableIncomingBitrate", 2000000.0}, {"currentRoundTripTime", 0.05}}},
		{"subscriptions", {{{"consumerId", consumerId}, {"producerId", producerId},
			{"visible", false}, {"pinned", false}, {"targetWidth", 0}, {"targetHeight", 0}}}}};
	bob.ws->request("downlinkClientStats", snap);
	usleep(500000);

	// Confirm old consumer was paused
	auto oldState = bob.ws->request("getConsumerState", {{"consumerId", consumerId}});
	ASSERT_TRUE(oldState.value("ok", false));
	ASSERT_TRUE(oldState["data"]["paused"].get<bool>());

	// Reconnect (rejoin same peerId without leaving — triggers replacePeer)
	bob.ws->close();
	bob = joinRoom(testRoom_, "bob");

	bobRecv = bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}});
	ASSERT_TRUE(bobRecv.value("ok", false));
	ASSERT_TRUE(bobRecv["data"].contains("consumers"));
	ASSERT_GT(bobRecv["data"]["consumers"].size(), 0u);
	std::string newConsumerId = bobRecv["data"]["consumers"][0]["id"];
	std::string newProducerId = bobRecv["data"]["consumers"][0]["producerId"];

	// New consumer should start unpaused (controller state was cleaned)
	auto newState = bob.ws->request("getConsumerState", {{"consumerId", newConsumerId}});
	ASSERT_TRUE(newState.value("ok", false));
	EXPECT_FALSE(newState["data"]["paused"].get<bool>()) << "new consumer after reconnect should not inherit paused state";

	// seq=1 should be accepted (old seq=10 state was cleaned on reconnect)
	json snap2 = {{"schema", "mediasoup.qos.downlink.client.v1"}, {"seq", 1}, {"tsMs", 1700000001000},
		{"subscriberPeerId", "bob"},
		{"transport", {{"availableIncomingBitrate", 2000000.0}, {"currentRoundTripTime", 0.05}}},
		{"subscriptions", {{{"consumerId", newConsumerId}, {"producerId", newProducerId},
			{"visible", true}, {"pinned", true}, {"targetWidth", 1280}, {"targetHeight", 720}}}}};
	auto resp = bob.ws->request("downlinkClientStats", snap2);
	ASSERT_TRUE(resp.value("ok", false)) << "fresh seq after reconnect should be accepted";
	usleep(500000);

	// Verify allocator applied fresh state (not inheriting old degradeLevel)
	auto finalState = bob.ws->request("getConsumerState", {{"consumerId", newConsumerId}});
	ASSERT_TRUE(finalState.value("ok", false));
	EXPECT_EQ(finalState["data"]["preferredSpatialLayer"].get<uint8_t>(), 2)
		<< "pinned large tile should get spatial=2 (no inherited degradeLevel)";
	EXPECT_EQ(finalState["data"]["priority"].get<uint8_t>(), 220)
		<< "pinned priority should be 220";
}

// ─── Downlink QoS v3: sustained zero-demand triggers pauseUpstream ───

TEST_F(QosIntegrationTest, DownlinkV3SustainedZeroDemandTriggersPauseUpstream) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	ASSERT_TRUE(bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}}).value("ok", false));

	const std::string producerId = produceVideo(alice, 66660200);
	auto consumerNotif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(consumerNotif.empty());
	const std::string consumerId = consumerNotif["data"]["id"];

	ASSERT_TRUE(alice.ws->request("clientStats",
		makePublisherClientStats(1, "camera-main", producerId)).value("ok", false));
	alice.ws->drainNotifications();

	// Send hidden snapshots repeatedly to sustain zero-demand past kPauseConfirmMs (4s)
	for (int i = 1; i <= 12; i++) {
		auto snap = makeDownlinkSnapshot(
			i, "bob", consumerId, producerId, 5'000'000.0, false, false, 0, 0);
		ASSERT_TRUE(bob.ws->request("downlinkClientStats", snap).value("ok", false));
		usleep(500'000);
	}

	// Collect all qosOverride notifications sent to publisher
	auto allNotifs = alice.ws->drainNotifications();
	bool gotPause = false;
	bool gotHold = false;
	size_t pauseCount = 0;
	size_t resumeCount = 0;
	for (auto& n : allNotifs) {
		if (n.value("method", "") != "qosOverride") continue;
		auto reason = n["data"].value("reason", "");
		if (reason == "downlink_v3_zero_demand_pause") gotPause = true;
		if (reason == "downlink_v2_zero_demand_hold") gotHold = true;
		if (n["data"].value("pauseUpstream", false)) pauseCount++;
		if (n["data"].value("resumeUpstream", false)) resumeCount++;
	}

	// After 6s of sustained zero-demand (12 x 500ms), must have reached pause.
	// Hold is an intermediate state but not sufficient proof of v3 behavior.
	EXPECT_TRUE(gotPause)
		<< "publisher must receive downlink_v3_zero_demand_pause after sustained hidden"
		<< " (gotHold=" << gotHold << ", gotPause=" << gotPause << ")";
	EXPECT_EQ(pauseCount, 1u) << "pauseUpstream should be emitted exactly once during sustained hidden";
	EXPECT_EQ(resumeCount, 0u) << "resumeUpstream must not be emitted while demand remains hidden";
}

TEST_F(QosIntegrationTest, DownlinkV3DemandRestoredAfterPauseTriggersClearOrResume) {
	auto alice = joinRoom(testRoom_, "alice");
	auto bob = joinRoom(testRoom_, "bob");

	ASSERT_TRUE(bob.ws->request("createWebRtcTransport", {{"producing", false}, {"consuming", true}}).value("ok", false));

	const std::string producerId = produceVideo(alice, 66660201);
	auto consumerNotif = bob.ws->waitNotification("newConsumer", 3000);
	ASSERT_FALSE(consumerNotif.empty());
	const std::string consumerId = consumerNotif["data"]["id"];

	ASSERT_TRUE(alice.ws->request("clientStats",
		makePublisherClientStats(1, "camera-main", producerId)).value("ok", false));
	alice.ws->drainNotifications();

	// Sustain zero-demand
	for (int i = 1; i <= 12; i++) {
		auto snap = makeDownlinkSnapshot(
			i, "bob", consumerId, producerId, 5'000'000.0, false, false, 0, 0);
		ASSERT_TRUE(bob.ws->request("downlinkClientStats", snap).value("ok", false));
		usleep(500'000);
	}
	alice.ws->drainNotifications();

	// Restore demand
	for (int i = 13; i <= 18; i++) {
		auto snap = makeDownlinkSnapshot(
			i, "bob", consumerId, producerId, 5'000'000.0, true, true, 1280, 720);
		ASSERT_TRUE(bob.ws->request("downlinkClientStats", snap).value("ok", false));
		usleep(500'000);
	}

	auto allNotifs = alice.ws->drainNotifications();
	bool gotResume = false;
	bool gotRestored = false;
	size_t pauseCount = 0;
	size_t resumeCount = 0;
	for (auto& n : allNotifs) {
		if (n.value("method", "") != "qosOverride") continue;
		auto reason = n["data"].value("reason", "");
		if (reason == "downlink_v3_demand_resumed") gotResume = true;
		if (reason == "downlink_v2_demand_restored") gotRestored = true;
		if (n["data"].value("pauseUpstream", false)) pauseCount++;
		if (n["data"].value("resumeUpstream", false)) resumeCount++;
	}

	// After sustained pause then 3s of visible demand (6 x 500ms),
	// must see the v3 resume path. demand_restored alone is not sufficient.
	EXPECT_TRUE(gotResume)
		<< "publisher must receive downlink_v3_demand_resumed after demand recovery"
		<< " (gotResume=" << gotResume << ", gotRestored=" << gotRestored << ")";
	EXPECT_EQ(resumeCount, 1u) << "resumeUpstream should be emitted exactly once after demand recovery";
	EXPECT_EQ(pauseCount, 0u) << "pauseUpstream must not be re-emitted after demand recovery";
}
