// QoS recording accuracy tests: inject RTP with simulated loss,
// verify the .qos.json file contains correct loss/bitrate/score data.
#include <gtest/gtest.h>
#include "Worker.h"
#include "Router.h"
#include "PlainTransport.h"
#include "Producer.h"
#include "Recorder.h"
#include "Constants.h"
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <random>
#include <thread>
#include <chrono>
#include <sys/stat.h>

using json = nlohmann::json;
using namespace mediasoup;

static const std::vector<json> kCodecs = {
	{{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2}}
};

static int createUdpSocket(uint16_t& port) {
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;
	bind(fd, (sockaddr*)&addr, sizeof(addr));
	socklen_t len = sizeof(addr);
	getsockname(fd, (sockaddr*)&addr, &len);
	port = ntohs(addr.sin_port);
	return fd;
}

static void sendRtpPacket(int fd, const sockaddr_in& dest,
	uint8_t pt, uint16_t seq, uint32_t ts, uint32_t ssrc)
{
	uint8_t buf[200];
	buf[0] = 0x80;
	buf[1] = pt | 0x80;
	buf[2] = (seq >> 8) & 0xFF;
	buf[3] = seq & 0xFF;
	buf[4] = (ts >> 24) & 0xFF;
	buf[5] = (ts >> 16) & 0xFF;
	buf[6] = (ts >> 8) & 0xFF;
	buf[7] = ts & 0xFF;
	buf[8] = (ssrc >> 24) & 0xFF;
	buf[9] = (ssrc >> 16) & 0xFF;
	buf[10] = (ssrc >> 8) & 0xFF;
	buf[11] = ssrc & 0xFF;
	memset(buf + 12, 0xAB, 188);
	sendto(fd, buf, 200, 0, (sockaddr*)&dest, sizeof(dest));
}

class QosRecordingAccuracyTest : public ::testing::Test {
protected:
	std::shared_ptr<Worker> worker;
	std::string tmpDir;

	void SetUp() override {
		WorkerSettings settings;
		settings.workerBin = "./mediasoup-worker";
		settings.rtcMinPort = 41000;
		settings.rtcMaxPort = 41999;
		worker = std::make_shared<Worker>(settings);

		tmpDir = "/tmp/qos_rec_accuracy_" + std::to_string(getpid());
		mkdir(tmpDir.c_str(), 0755);
	}

	void TearDown() override {
		if (worker) worker->close();
		// Cleanup tmp files
		system(("rm -rf " + tmpDir).c_str());
	}

	struct TestSetup {
		std::shared_ptr<Router> router;
		std::shared_ptr<PlainTransport> prodTransport;
		std::shared_ptr<Producer> producer;
		std::shared_ptr<PeerRecorder> recorder;
		std::shared_ptr<PlainTransport> recTransport;
		int senderFd = -1;
		sockaddr_in destAddr{};
		uint16_t seq = 1;
		uint32_t ts = 1000;
		uint32_t ssrc = 0;
		uint8_t pt = 100;
		std::string qosPath;
	};

	TestSetup createSetup(const std::string& name) {
		TestSetup s;
		std::mt19937 rng(std::random_device{}());
		s.ssrc = std::uniform_int_distribution<uint32_t>(100000000, 999999999)(rng);

		s.router = worker->createRouter(kCodecs);
		auto& caps = s.router->rtpCapabilities();
		for (auto& c : caps.codecs) {
			if (c.mimeType.find("opus") != std::string::npos &&
				c.mimeType.find("rtx") == std::string::npos) {
				s.pt = c.preferredPayloadType;
				break;
			}
		}

		// Producer transport
		PlainTransportOptions opts;
		opts.listenInfos = {{{"ip", "127.0.0.1"}}};
		opts.rtcpMux = true;
		opts.comedia = false;
		s.prodTransport = s.router->createPlainTransport(opts);

		uint16_t senderPort;
		s.senderFd = createUdpSocket(senderPort);
		s.prodTransport->connect("127.0.0.1", senderPort);

		s.destAddr.sin_family = AF_INET;
		inet_pton(AF_INET, "127.0.0.1", &s.destAddr.sin_addr);
		s.destAddr.sin_port = htons(s.prodTransport->tuple().localPort);

		json rtpParams = {
			{"codecs", {{
				{"mimeType", "audio/opus"}, {"payloadType", s.pt},
				{"clockRate", 48000}, {"channels", 2}
			}}},
			{"encodings", {{{"ssrc", s.ssrc}}}},
			{"mid", "0"}
		};
		s.producer = s.prodTransport->produce({
			{"kind", "audio"}, {"rtpParameters", rtpParams},
			{"routerRtpCapabilities", caps}
		});

		// Recorder
		std::string path = tmpDir + "/" + name + ".webm";
		s.qosPath = tmpDir + "/" + name + ".qos.json";
		s.recorder = std::make_shared<PeerRecorder>(
			name, path, s.pt, 101, 48000, 90000, VideoCodec::VP8, "test-room");
		int recPort = s.recorder->createSocket();
		EXPECT_GT(recPort, 0);
		s.recorder->start();

		// PlainTransport for recorder
		PlainTransportOptions recOpts;
		recOpts.listenInfos = {{{"ip", "127.0.0.1"}}};
		recOpts.rtcpMux = true;
		recOpts.comedia = false;
		s.recTransport = s.router->createPlainTransport(recOpts);
		s.recTransport->connect("127.0.0.1", recPort);

		json consumeOpts = {
			{"producerId", s.producer->id()},
			{"rtpCapabilities", caps},
			{"consumableRtpParameters", s.producer->consumableRtpParameters()},
			{"pipe", true}
		};
		s.recTransport->consume(consumeOpts);

		return s;
	}

	json collectStats(TestSetup& s) {
		json result = {{"peerId", "test-peer"}};

		// Producer stats
		json producers = json::object();
		auto pstats = s.producer->getStats();
		json pentry;
		pentry["kind"] = "audio";
		pentry["stats"] = pstats;
		json scores = json::array();
		for (auto& sc : s.producer->scores())
			scores.push_back(json({{"ssrc", sc.ssrc}, {"score", sc.score}}));
		pentry["scores"] = scores;
		producers[s.producer->id()] = pentry;
		result["producers"] = producers;

		// Transport stats
		auto tstats = s.prodTransport->getStats();
		result["sendTransport"] = tstats;

		return result;
	}

	json readQosFile(const std::string& path) {
		std::ifstream f(path);
		if (!f.is_open()) return json::array();
		std::string content((std::istreambuf_iterator<char>(f)),
			std::istreambuf_iterator<char>());
		// Handle unclosed JSON array (SFU may not have written closing ])
		if (!content.empty() && content.back() != ']')
			content += "\n]";
		try { return json::parse(content); }
		catch (...) { return json::array(); }
	}
};

// ─── Test 1: No loss → QoS file shows 0 loss ───
TEST_F(QosRecordingAccuracyTest, NoLossQosFile) {
	auto s = createSetup("noloss");

	// Send 300 packets, no gaps
	for (int i = 0; i < 300; i++) {
		sendRtpPacket(s.senderFd, s.destAddr, s.pt, s.seq++, s.ts, s.ssrc);
		s.ts += 960;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	// Collect stats and write to recorder
	std::this_thread::sleep_for(std::chrono::seconds(2));
	auto stats = collectStats(s);
	s.recorder->appendQosSnapshot(stats);

	// Stop recorder to finalize QoS file
	s.recorder->stop();

	auto qos = readQosFile(s.qosPath);
	ASSERT_FALSE(qos.empty()) << "QoS file should not be empty";

	auto& last = qos.back();
	EXPECT_TRUE(last.contains("t"));
	EXPECT_TRUE(last.contains("stats"));

	auto& pstats = last["stats"]["producers"];
	ASSERT_FALSE(pstats.empty());
	auto& pentry = pstats.begin().value();
	ASSERT_FALSE(pentry["stats"].empty());
	int lost = pentry["stats"][0].value("packetsLost", -1);
	EXPECT_EQ(lost, 0) << "No loss scenario should report 0 packetsLost";
}

// ─── Test 2: 20% loss → QoS file shows loss ───
TEST_F(QosRecordingAccuracyTest, TwentyPercentLossQosFile) {
	auto s = createSetup("loss20");

	std::mt19937 rng(99);
	int dropped = 0;

	for (int i = 0; i < 500; i++) {
		if (rng() % 5 == 0) {
			// Skip: advance seq but don't send
			s.seq++;
			s.ts += 960;
			dropped++;
		} else {
			sendRtpPacket(s.senderFd, s.destAddr, s.pt, s.seq++, s.ts, s.ssrc);
			s.ts += 960;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	std::this_thread::sleep_for(std::chrono::seconds(3));

	// Write multiple snapshots to simulate broadcastStats
	for (int i = 0; i < 3; i++) {
		auto stats = collectStats(s);
		s.recorder->appendQosSnapshot(stats);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	s.recorder->stop();

	auto qos = readQosFile(s.qosPath);
	ASSERT_GE(qos.size(), 2u) << "Should have multiple QoS snapshots";

	// Check last snapshot has loss data
	auto& last = qos.back();
	auto& pstats = last["stats"]["producers"];
	ASSERT_FALSE(pstats.empty());
	auto& pentry = pstats.begin().value();
	ASSERT_FALSE(pentry["stats"].empty());

	int lost = pentry["stats"][0].value("packetsLost", 0);
	EXPECT_GT(lost, dropped * 0.3) << "packetsLost too low: " << lost << " vs dropped " << dropped;
	EXPECT_LT(lost, dropped * 2.0) << "packetsLost too high: " << lost << " vs dropped " << dropped;

	// fractionLost should be non-zero (RTCP 0-255 format)
	double frac = pentry["stats"][0].value("fractionLost", 0.0);
	EXPECT_GT(frac, 0) << "fractionLost should be non-zero with 20% loss";
}

// ─── Test 3: QoS timestamps are monotonically increasing ───
TEST_F(QosRecordingAccuracyTest, QosTimestampsMonotonic) {
	auto s = createSetup("monotonic");

	for (int i = 0; i < 200; i++) {
		sendRtpPacket(s.senderFd, s.destAddr, s.pt, s.seq++, s.ts, s.ssrc);
		s.ts += 960;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));

	// Write 5 snapshots with 1s intervals
	for (int i = 0; i < 5; i++) {
		auto stats = collectStats(s);
		s.recorder->appendQosSnapshot(stats);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	s.recorder->stop();

	auto qos = readQosFile(s.qosPath);
	ASSERT_GE(qos.size(), 4u);

	double prevT = -1;
	for (auto& entry : qos) {
		double t = entry.value("t", -1.0);
		EXPECT_GT(t, prevT) << "Timestamps should be monotonically increasing";
		prevT = t;
	}
}

// ─── Test 4: Bitrate in QoS file is reasonable ───
TEST_F(QosRecordingAccuracyTest, BitrateInQosFile) {
	auto s = createSetup("bitrate");

	// Send 200-byte packets at 50pps for 6 seconds = 80kbps
	for (int i = 0; i < 300; i++) {
		sendRtpPacket(s.senderFd, s.destAddr, s.pt, s.seq++, s.ts, s.ssrc);
		s.ts += 960;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	// Collect stats while stream is warm
	auto stats = collectStats(s);
	s.recorder->appendQosSnapshot(stats);
	s.recorder->stop();

	auto qos = readQosFile(s.qosPath);
	ASSERT_FALSE(qos.empty());

	auto& last = qos.back();
	auto& pstats = last["stats"]["producers"];
	ASSERT_FALSE(pstats.empty());
	auto& pentry = pstats.begin().value();
	ASSERT_FALSE(pentry["stats"].empty());

	int64_t bitrate = pentry["stats"][0].value("bitrate", 0);
	EXPECT_GT(bitrate, 10000) << "Bitrate too low in QoS file: " << bitrate;
	EXPECT_LT(bitrate, 200000) << "Bitrate too high in QoS file: " << bitrate;
}

// ─── Test 5: Score present in QoS file ───
TEST_F(QosRecordingAccuracyTest, ScoreInQosFile) {
	auto s = createSetup("score");

	for (int i = 0; i < 300; i++) {
		sendRtpPacket(s.senderFd, s.destAddr, s.pt, s.seq++, s.ts, s.ssrc);
		s.ts += 960;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));
	auto stats = collectStats(s);
	s.recorder->appendQosSnapshot(stats);
	s.recorder->stop();

	auto qos = readQosFile(s.qosPath);
	ASSERT_FALSE(qos.empty());

	auto& last = qos.back();
	auto& pstats = last["stats"]["producers"];
	ASSERT_FALSE(pstats.empty());
	auto& pentry = pstats.begin().value();

	// Scores array should exist
	EXPECT_TRUE(pentry.contains("scores"));
	if (!pentry["scores"].empty()) {
		int score = pentry["scores"][0].value("score", -1);
		EXPECT_GE(score, 0);
		EXPECT_LE(score, 10);
	}
}
