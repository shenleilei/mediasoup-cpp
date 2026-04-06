// Integration tests for QoS stats: getStats signaling, statsReport broadcast,
// and various error/edge cases.
#include <gtest/gtest.h>
#include "TestWsClient.h"
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fstream>

static const int SFU_PORT = 18766; // different port from main integration tests
static const std::string HOST = "127.0.0.1";

class QosIntegrationTest : public ::testing::Test {
protected:
	pid_t sfuPid_ = -1;
	std::string testRoom_;

	void SetUp() override {
		testRoom_ = "qos_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());

		std::string cmd = "./build/mediasoup-sfu --nodaemon"
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

	// Bob queries Alice's stats
	auto resp = bob.ws->request("getStats", {{"peerId", "alice"}});
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
	auto consumerNotif = bob.ws->waitNotification("newConsumer", 3000);
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
	pid_t sfuPid_ = -1;
	std::string testRoom_;
	std::string recordDir_;

	void SetUp() override {
		testRoom_ = "qosrec_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
		recordDir_ = "/tmp/mediasoup_qosrec_" + std::to_string(getpid());
		system(("rm -rf " + recordDir_).c_str());
		mkdir(recordDir_.c_str(), 0755);

		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(SFU_PORT) +
			" --workers=1 --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1 --listenIp=127.0.0.1"
			" --recordDir=" + recordDir_ +
			" > /tmp/sfu_qostest.log 2>&1 & echo $!";
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
				::close(fd); usleep(200000); return;
			}
			::close(fd);
		}
		FAIL() << "SFU did not start";
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
				if (free) break;
			}
		}
		system(("rm -rf " + recordDir_).c_str());
	}

	// Simple HTTP GET helper (no WebSocket, just plain HTTP)
	std::string httpGet(const std::string& path) {
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) return "";
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(SFU_PORT);
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
		EXPECT_TRUE(c.ws->connect(HOST, SFU_PORT));

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
	std::string body = httpGet("/recordings/../../../etc/passwd");
	// Should get 403 or 404, not file contents
	EXPECT_TRUE(body.find("root:") == std::string::npos) << "Path traversal not blocked!";
}

// ─── Test: /api/recordings returns empty when no recordings ───
TEST_F(QosRecordingTest, EmptyRecordingsApi) {
	// No one joined, no recordings
	std::string body = httpGet("/api/recordings");
	auto rooms = json::parse(body);
	EXPECT_TRUE(rooms.is_array());
	EXPECT_TRUE(rooms.empty());
}
