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
	pid_t sfuPid_ = -1;
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

		// Start SFU as a detached background process (avoids fork fd inheritance issues)
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
			terminateSfuProcess(sfuPid_);
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
		EXPECT_TRUE(c.ws->connect(HOST, SFU_PORT));

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

	bool waitForFreshRoomReady(int timeoutMs = 5000) {
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
		int attempt = 0;
		while (std::chrono::steady_clock::now() < deadline) {
			TestWsClient ws;
			if (!ws.connect(HOST, SFU_PORT)) {
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
	ASSERT_TRUE(ws1.connect(HOST, SFU_PORT));
	ASSERT_TRUE(ws2.connect(HOST, SFU_PORT));
	ASSERT_TRUE(ws3.connect(HOST, SFU_PORT));
	ASSERT_TRUE(ws4.connect(HOST, SFU_PORT));

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

static const int SFU_PORT_A = 14000;
static const int SFU_PORT_B = 14006;

class MultiNodeTest : public ::testing::Test {
protected:
	pid_t pidA_ = -1, pidB_ = -1;
	std::string testRoom_;

	static pid_t startSfu(int port) {
		std::string cmd = "./build/mediasoup-sfu --nodaemon"
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
		terminateSfuProcess(pid);
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

// ═══════════════════════════════════════════════════════════════
// Recording tests: verify PeerRecorder produces valid WebM with aligned A/V
// ═══════════════════════════════════════════════════════════════

#include <sys/stat.h>
#include "Recorder.h"

// Helper: build a minimal RTP packet
static std::vector<uint8_t> makeRtpPacket(uint8_t pt, uint16_t seq, uint32_t ts, uint32_t ssrc,
	const uint8_t* payload, int payloadLen, bool marker = false)
{
	std::vector<uint8_t> pkt(12 + payloadLen);
	pkt[0] = 0x80; // V=2
	pkt[1] = (marker ? 0x80 : 0x00) | pt;
	pkt[2] = (seq >> 8) & 0xFF; pkt[3] = seq & 0xFF;
	pkt[4] = (ts >> 24); pkt[5] = (ts >> 16); pkt[6] = (ts >> 8); pkt[7] = ts;
	pkt[8] = (ssrc >> 24); pkt[9] = (ssrc >> 16); pkt[10] = (ssrc >> 8); pkt[11] = ssrc;
	memcpy(pkt.data() + 12, payload, payloadLen);
	return pkt;
}

// Build a VP8 RTP packet with minimal payload descriptor (S=1 single-partition frame + marker)
static std::vector<uint8_t> makeVp8RtpPacket(uint8_t pt, uint16_t seq, uint32_t ts, uint32_t ssrc,
	const uint8_t* payload, int payloadLen, bool keyframe = false)
{
	// VP8 payload descriptor: 1 byte, S=1 (start of partition), PartID=0
	std::vector<uint8_t> vp8payload(1 + payloadLen);
	vp8payload[0] = 0x10; // S=1
	if (payloadLen > 0) {
		memcpy(vp8payload.data() + 1, payload, payloadLen);
		// VP8 keyframe: first byte of VP8 data has bit0=0
		if (keyframe) vp8payload[1] &= 0xFE;
		else          vp8payload[1] |= 0x01;
	}
	return makeRtpPacket(pt, seq, ts, ssrc, vp8payload.data(), vp8payload.size(), true);
}

static void sendUdp(int port, const std::vector<uint8_t>& data) {
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	sendto(fd, data.data(), data.size(), 0, (sockaddr*)&addr, sizeof(addr));
	::close(fd);
}

class RecordingTest : public ::testing::Test {
protected:
	std::string recordDir_;

	void SetUp() override {
		recordDir_ = "/tmp/mediasoup_rec_test_" + std::to_string(getpid());
		system(("rm -rf " + recordDir_).c_str());
		mkdir(recordDir_.c_str(), 0755);
	}

	void TearDown() override {
		system(("rm -rf " + recordDir_).c_str());
	}
};

// ─── Test: Recording file created with valid WebM header ───
TEST_F(RecordingTest, RecordingFileCreated) {
	std::string path = recordDir_ + "/test.webm";
	// Use same PT as SFU would assign: audio=100, video=101
	mediasoup::PeerRecorder rec("alice", path, 100, 101, 48000, 90000);
	int port = rec.createSocket();
	ASSERT_GT(port, 0);
	ASSERT_TRUE(rec.start());

	// Send some fake RTP packets (audio + video)
	uint8_t fakePayload[160] = {};
	for (int i = 0; i < 50; i++) {
		// Audio: PT=100, 960 samples per frame (20ms at 48kHz)
		auto audioPkt = makeRtpPacket(100, i, i * 960, 0xAAAA, fakePayload, 80);
		sendUdp(port, audioPkt);
		// Video: PT=101, 3000 ticks per frame (33ms at 90kHz ≈ 30fps)
		auto videoPkt = makeVp8RtpPacket(101, i, i * 3000, 0xBBBB, fakePayload, 160, i == 0);
		sendUdp(port, videoPkt);
		usleep(20000); // 20ms
	}

	rec.stop();

	// Verify file exists and has content
	struct stat st;
	ASSERT_EQ(stat(path.c_str(), &st), 0) << "Recording file not found";
	EXPECT_GT(st.st_size, 200) << "Recording file too small";

	// ffprobe to verify streams
	char buf[4096];
	std::string probeCmd = "ffprobe -v error -show_entries stream=codec_type,codec_name -of csv=p=0 " + path + " 2>&1";
	FILE* fp = popen(probeCmd.c_str(), "r");
	std::string probeOut;
	while (fgets(buf, sizeof(buf), fp)) probeOut += buf;
	pclose(fp);

	EXPECT_NE(probeOut.find("audio"), std::string::npos) << "No audio stream. ffprobe: " << probeOut;
	EXPECT_NE(probeOut.find("video"), std::string::npos) << "No video stream. ffprobe: " << probeOut;
}

// ─── Test: Audio/Video timestamps are aligned ───
TEST_F(RecordingTest, AudioVideoAlignment) {
	std::string path = recordDir_ + "/align_test.webm";
	mediasoup::PeerRecorder rec("bob", path, 100, 101, 48000, 90000);
	int port = rec.createSocket();
	ASSERT_GT(port, 0);
	ASSERT_TRUE(rec.start());

	uint8_t fakePayload[160] = {};

	// Simulate 2 seconds of audio+video starting at the same time
	// Audio: 20ms frames, 960 samples/frame at 48kHz
	// Video: 33ms frames, 3000 ticks/frame at 90kHz (~30fps)
	uint32_t audioTs = 1000000; // arbitrary start
	uint32_t videoTs = 2000000; // different start (will be normalized by base subtraction)
	for (int i = 0; i < 100; i++) {
		auto aPkt = makeRtpPacket(100, i, audioTs + i * 960, 0xAAAA, fakePayload, 80);
		sendUdp(port, aPkt);
		if (i % 2 == 0) { // video at ~30fps (every other 20ms = 40ms ≈ 25fps)
			auto vPkt = makeVp8RtpPacket(101, i/2, videoTs + (i/2) * 3600, 0xBBBB, fakePayload, 160, i == 0);
			sendUdp(port, vPkt);
		}
		usleep(20000);
	}

	rec.stop();

	// Use ffprobe to get start_time of each stream
	char buf[4096];
	std::string probeCmd = "ffprobe -v error -show_entries stream=codec_type,start_time -of csv=p=0 " + path + " 2>&1";
	FILE* fp = popen(probeCmd.c_str(), "r");
	std::string probeOut;
	while (fgets(buf, sizeof(buf), fp)) probeOut += buf;
	pclose(fp);

	double audioStart = -1, videoStart = -1;
	std::istringstream iss(probeOut);
	std::string line;
	while (std::getline(iss, line)) {
		auto pos = line.rfind(',');
		if (pos == std::string::npos) continue;
		double t = std::atof(line.substr(pos + 1).c_str());
		if (line.find("audio") != std::string::npos) audioStart = t;
		else if (line.find("video") != std::string::npos) videoStart = t;
	}

	ASSERT_GE(audioStart, 0) << "No audio start_time. ffprobe: " << probeOut;
	ASSERT_GE(videoStart, 0) << "No video start_time. ffprobe: " << probeOut;

	// Both should start near 0 (base timestamp subtracted)
	// Drift should be < 0.5 seconds
	double drift = std::abs(audioStart - videoStart);
	EXPECT_LT(drift, 0.5)
		<< "Audio/video misaligned: audio=" << audioStart
		<< " video=" << videoStart << " drift=" << drift << "s";
}

// ─── Test: SFU auto-recording creates file on produce ───
class SfuRecordingTest : public ::testing::Test {
protected:
	pid_t sfuPid_ = -1;
	std::string testRoom_;
	std::string recordDir_;

	void SetUp() override {
		testRoom_ = "rec_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
		recordDir_ = "/tmp/mediasoup_rec_sfu_" + std::to_string(getpid());
		system(("rm -rf " + recordDir_).c_str());
		mkdir(recordDir_.c_str(), 0755);

		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(SFU_PORT) +
			" --workers=1 --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1 --listenIp=127.0.0.1"
			" --redisHost=0.0.0.0 --redisPort=1"
			" --recordDir=" + recordDir_ +
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
};

TEST_F(SfuRecordingTest, AutoRecordOnProduce) {
	TestWsClient ws;
	ASSERT_TRUE(ws.connect(HOST, SFU_PORT));

	json rtpCaps = {{"codecs", {{
		{"mimeType", "audio/opus"}, {"kind", "audio"},
		{"clockRate", 48000}, {"channels", 2}, {"preferredPayloadType", 100}
	}, {
		{"mimeType", "video/VP8"}, {"kind", "video"},
		{"clockRate", 90000}, {"preferredPayloadType", 101}
	}}}, {"headerExtensions", json::array()}};

	auto joinResp = ws.request("join", {
		{"roomId", testRoom_}, {"peerId", "streamer"},
		{"displayName", "streamer"}, {"rtpCapabilities", rtpCaps}
	});
	ASSERT_TRUE(joinResp.value("ok", false));

	auto sendResp = ws.request("createWebRtcTransport", {{"producing", true}, {"consuming", false}});
	ASSERT_TRUE(sendResp.value("ok", false));
	std::string tid = sendResp["data"]["id"];

	auto audioResp = ws.request("produce", {
		{"transportId", tid}, {"kind", "audio"},
		{"rtpParameters", {{"codecs", {{{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2}, {"payloadType", 100}}}},
		{"encodings", {{{"ssrc", 11111111}}}}, {"mid", "0"}}}
	});
	ASSERT_TRUE(audioResp.value("ok", false));

	auto videoResp = ws.request("produce", {
		{"transportId", tid}, {"kind", "video"},
		{"rtpParameters", {{"codecs", {{{"mimeType", "video/VP8"}, {"clockRate", 90000}, {"payloadType", 101}}}},
		{"encodings", {{{"ssrc", 22222222}}}}, {"mid", "1"}}}
	});
	ASSERT_TRUE(videoResp.value("ok", false));

	usleep(500000); // let recorder initialize

	// Verify recording file was created while recorder is active
	// (H264 deferred header creates the file via avio_open, but deletes it on stop if empty)
	std::string findCmd = "find " + recordDir_ + " \\( -name '*.webm' -o -name '*.mp4' \\) 2>/dev/null";
	FILE* fp = popen(findCmd.c_str(), "r");
	char buf[512]{};
	std::string files;
	while (fgets(buf, sizeof(buf), fp)) files += buf;
	pclose(fp);

	EXPECT_FALSE(files.empty()) << "No recording file created by SFU auto-record";

	ws.close();
	usleep(500000);
}

// ═══════════════════════════════════════════════════════════════
// H264 recording tests
// ═══════════════════════════════════════════════════════════════

// Build H264 STAP-A packet containing SPS + PPS
static std::vector<uint8_t> makeH264StapA(uint8_t pt, uint16_t seq, uint32_t ts, uint32_t ssrc,
	const uint8_t* sps, int spsLen, const uint8_t* pps, int ppsLen)
{
	// STAP-A: nalType=24, then [2-byte size + NAL] for each
	std::vector<uint8_t> payload;
	payload.push_back(24); // STAP-A indicator (NRI=0, type=24)
	payload.push_back((spsLen >> 8) & 0xFF); payload.push_back(spsLen & 0xFF);
	payload.insert(payload.end(), sps, sps + spsLen);
	payload.push_back((ppsLen >> 8) & 0xFF); payload.push_back(ppsLen & 0xFF);
	payload.insert(payload.end(), pps, pps + ppsLen);
	return makeRtpPacket(pt, seq, ts, ssrc, payload.data(), payload.size(), false);
}

// Build H264 FU-A packets from a NAL unit
static std::vector<std::vector<uint8_t>> makeH264FuA(uint8_t pt, uint16_t& seq, uint32_t ts,
	uint32_t ssrc, uint8_t nalType, const uint8_t* data, int dataLen, int maxFrag = 900)
{
	std::vector<std::vector<uint8_t>> pkts;
	uint8_t fuIndicator = 0x60 | 28; // NRI=3, type=28
	int offset = 0;
	while (offset < dataLen) {
		int fragLen = std::min(maxFrag, dataLen - offset);
		bool isFirst = (offset == 0);
		bool isLast = (offset + fragLen >= dataLen);
		uint8_t fuHeader = nalType;
		if (isFirst) fuHeader |= 0x80;
		if (isLast) fuHeader |= 0x40;
		std::vector<uint8_t> payload;
		payload.push_back(fuIndicator);
		payload.push_back(fuHeader);
		payload.insert(payload.end(), data + offset, data + offset + fragLen);
		pkts.push_back(makeRtpPacket(pt, seq++, ts, ssrc, payload.data(), payload.size(), isLast));
		offset += fragLen;
	}
	return pkts;
}

// Build H264 single NAL RTP packet
static std::vector<uint8_t> makeH264SingleNal(uint8_t pt, uint16_t seq, uint32_t ts,
	uint32_t ssrc, uint8_t nalType, int payloadLen, bool marker = true)
{
	std::vector<uint8_t> payload(1 + payloadLen);
	payload[0] = 0x60 | nalType; // NRI=3
	for (int i = 1; i <= payloadLen; i++) payload[i] = (uint8_t)(i & 0xFF);
	return makeRtpPacket(pt, seq, ts, ssrc, payload.data(), payload.size(), marker);
}

TEST_F(RecordingTest, H264RecordingValid) {
	std::string path = recordDir_ + "/h264_test.webm";
	mediasoup::PeerRecorder rec("alice", path, 100, 107, 48000, 90000, mediasoup::VideoCodec::H264);
	int port = rec.createSocket();
	ASSERT_GT(port, 0);
	ASSERT_TRUE(rec.start());

	// Minimal SPS (NAL type 7) and PPS (NAL type 8)
	uint8_t sps[] = {0x67, 0x42, 0xC0, 0x1E, 0xD9, 0x00, 0xA0, 0x47, 0xFE, 0x88};
	uint8_t pps[] = {0x68, 0xCE, 0x38, 0x80};

	uint16_t vseq = 0;
	uint32_t vts = 0;

	// Send STAP-A with SPS+PPS
	auto stap = makeH264StapA(107, vseq++, vts, 0xBBBB, sps, sizeof(sps), pps, sizeof(pps));
	sendUdp(port, stap);

	// Send IDR frame as FU-A (nalType=5)
	uint8_t idrData[1500];
	memset(idrData, 0xAB, sizeof(idrData));
	auto idrPkts = makeH264FuA(107, vseq, vts, 0xBBBB, 5, idrData, sizeof(idrData));
	for (auto& p : idrPkts) sendUdp(port, p);

	// Send audio
	uint8_t fakeAudio[80] = {};
	for (int i = 0; i < 50; i++) {
		auto aPkt = makeRtpPacket(100, i, i * 960, 0xAAAA, fakeAudio, 80);
		sendUdp(port, aPkt);
	}

	// Send more P-frames (nalType=1)
	for (int i = 1; i < 30; i++) {
		vts += 3000;
		uint8_t pData[500];
		memset(pData, (uint8_t)i, sizeof(pData));
		auto pPkt = makeH264SingleNal(107, vseq++, vts, 0xBBBB, 1, sizeof(pData));
		sendUdp(port, pPkt);
		usleep(20000);
	}

	rec.stop();

	// Verify file exists and has content
	struct stat st;
	ASSERT_EQ(stat(path.c_str(), &st), 0) << "H264 recording file not found";
	EXPECT_GT(st.st_size, 200) << "H264 recording file too small: " << st.st_size;

	// ffprobe to verify H264 video stream
	char buf[4096];
	std::string probeCmd = "ffprobe -v error -show_entries stream=codec_type,codec_name -of csv=p=0 " + path + " 2>&1";
	FILE* fp = popen(probeCmd.c_str(), "r");
	std::string probeOut;
	while (fgets(buf, sizeof(buf), fp)) probeOut += buf;
	pclose(fp);

	EXPECT_NE(probeOut.find("audio"), std::string::npos) << "No audio stream. ffprobe: " << probeOut;
	EXPECT_NE(probeOut.find("h264"), std::string::npos) << "No H264 video stream. ffprobe: " << probeOut;
}

TEST_F(RecordingTest, H264NoIdrProducesEmptyFile) {
	std::string path = recordDir_ + "/h264_noidr.webm";
	mediasoup::PeerRecorder rec("bob", path, 100, 107, 48000, 90000, mediasoup::VideoCodec::H264);
	int port = rec.createSocket();
	ASSERT_GT(port, 0);
	ASSERT_TRUE(rec.start());

	// Send only P-frames (no SPS/PPS/IDR) — should not crash
	for (int i = 0; i < 10; i++) {
		auto pPkt = makeH264SingleNal(107, i, i * 3000, 0xBBBB, 1, 200);
		sendUdp(port, pPkt);
		usleep(10000);
	}

	rec.stop(); // must not crash

	struct stat st;
	if (stat(path.c_str(), &st) == 0) {
		// File exists but should be very small (no header written)
		EXPECT_LT(st.st_size, 100) << "File should be nearly empty without IDR";
	}
}

TEST_F(RecordingTest, Vp8MultiPacketFrame) {
	std::string path = recordDir_ + "/vp8_multi.webm";
	mediasoup::PeerRecorder rec("charlie", path, 100, 101, 48000, 90000, mediasoup::VideoCodec::VP8);
	int port = rec.createSocket();
	ASSERT_GT(port, 0);
	ASSERT_TRUE(rec.start());

	uint8_t fakeAudio[80] = {};
	uint8_t fakeVideo[400] = {};

	for (int frame = 0; frame < 30; frame++) {
		// Audio
		auto aPkt = makeRtpPacket(100, frame, frame * 960, 0xAAAA, fakeAudio, 80);
		sendUdp(port, aPkt);

		// VP8 frame split into 3 RTP packets
		uint32_t vts = frame * 3000;
		for (int frag = 0; frag < 3; frag++) {
			std::vector<uint8_t> vp8payload;
			uint8_t desc = 0;
			if (frag == 0) {
				desc = 0x10; // S=1
				vp8payload.push_back(desc);
				// VP8 keyframe byte: bit0=0 for keyframe on first frame
				uint8_t firstByte = (frame == 0) ? 0x9c : 0x9d;
				vp8payload.push_back(firstByte);
				vp8payload.insert(vp8payload.end(), fakeVideo, fakeVideo + 130);
			} else {
				desc = 0x00; // continuation
				vp8payload.push_back(desc);
				vp8payload.insert(vp8payload.end(), fakeVideo, fakeVideo + 130);
			}
			bool marker = (frag == 2);
			auto pkt = makeRtpPacket(101, frame * 3 + frag, vts, 0xBBBB,
				vp8payload.data(), vp8payload.size(), marker);
			sendUdp(port, pkt);
		}
		usleep(20000);
	}

	rec.stop();

	struct stat st;
	ASSERT_EQ(stat(path.c_str(), &st), 0);
	EXPECT_GT(st.st_size, 200);

	// ffprobe
	char buf[4096];
	std::string probeCmd = "ffprobe -v error -show_entries stream=codec_type,codec_name -of csv=p=0 " + path + " 2>&1";
	FILE* fp = popen(probeCmd.c_str(), "r");
	std::string probeOut;
	while (fgets(buf, sizeof(buf), fp)) probeOut += buf;
	pclose(fp);

	EXPECT_NE(probeOut.find("audio"), std::string::npos) << "No audio. ffprobe: " << probeOut;
	EXPECT_NE(probeOut.find("vp8"), std::string::npos) << "No VP8 video. ffprobe: " << probeOut;
}

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
		std::string cmd = "pgrep -P " + std::to_string(sfuPid_) + " 2>/dev/null";
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
		std::string cmd = "pgrep -P " + std::to_string(sfuPid_) + " 2>/dev/null";
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
