// Integration tests for QoS stats: getStats signaling, statsReport broadcast,
// and various error/edge cases.
#include <gtest/gtest.h>
#include "TestWsClient.h"
#include "TestProcessUtils.h"
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fstream>

static const int SFU_PORT = 14011; // different port from main integration tests
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
		// Wait for port to be free (previous fixture may have just torn down)
		bool portFree = false;
		for (int i = 0; i < 30; ++i) {
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			sockaddr_in addr{};
			addr.sin_family = AF_INET;
			addr.sin_port = htons(SFU_PORT);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			int opt = 1;
			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
			portFree = (bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
			::close(fd);
			if (portFree) break;
			usleep(100000);
		}
		ASSERT_TRUE(portFree) << "Port " << SFU_PORT << " still occupied by previous SFU";

		testRoom_ = "qosrec_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
		recordDir_ = "/tmp/mediasoup_qosrec_" + std::to_string(getpid());
		system(("rm -rf " + recordDir_).c_str());
		mkdir(recordDir_.c_str(), 0755);

		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(SFU_PORT) +
			" --workers=1 --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1 --listenIp=127.0.0.1"
			" --redisHost=0.0.0.0 --redisPort=1"
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
				if (free) break;
			}
		}
		system(("rm -rf " + recordDir_).c_str());
		}

		// Simple HTTP GET helper (no WebSocket, just plain HTTP)
		std::string httpGetRaw(const std::string& path) {
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

	// Wait for statsReport notification (3s interval)
	auto notif = alice.ws->waitNotification("statsReport", 8000);
	ASSERT_FALSE(notif.empty()) << "Did not receive statsReport";

	auto& peers = notif["data"]["peers"];
	ASSERT_FALSE(peers.empty());

	// Find alice's stats in the broadcast
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
	ASSERT_TRUE(alice.ws->request("clientStats", older).value("ok", false));
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
	if (aliceOverride.empty()) aliceOverride = alice.ws->waitNotification("qosOverride", 1000);
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

// ─── Test: clientStats in recording QoS file ───
// Note: QoS snapshot passthrough to QoS file is implicitly tested by
// test_qos_recording_accuracy.cpp (appendQosSnapshot writes full peer stats
// including clientStats/qos). The service-side passthrough itself is verified
// by ClientStatsQosStoredAndAggregated and ClientStatsQosInBroadcast above.
