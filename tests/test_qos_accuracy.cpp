// QoS accuracy tests: inject RTP via PlainTransport with simulated
// packet loss and jitter, then verify Worker stats match expectations.
#include <gtest/gtest.h>
#include "Worker.h"
#include "Router.h"
#include "PlainTransport.h"
#include "Producer.h"
#include "Consumer.h"
#include "Constants.h"
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <random>
#include <thread>
#include <chrono>

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

struct TestRoom {
	std::shared_ptr<Router> router;
	std::shared_ptr<PlainTransport> prodTransport;
	std::shared_ptr<Producer> producer;
	int senderFd = -1;
	sockaddr_in destAddr{};
	uint16_t seq = 1;
	uint32_t ts = 1000;
	uint32_t ssrc = 0;
	uint8_t pt = 100;

	~TestRoom() {
		if (senderFd >= 0) close(senderFd);
		if (router) try { router->close(); } catch (...) {}
	}
};

static TestRoom createTestRoom(std::shared_ptr<Worker>& worker) {
	TestRoom room;
	std::mt19937 rng(std::random_device{}());
	room.ssrc = std::uniform_int_distribution<uint32_t>(100000000, 999999999)(rng);

	room.router = worker->createRouter(kCodecs);
	auto& caps = room.router->rtpCapabilities();
	for (auto& c : caps.codecs) {
		if (c.mimeType.find("opus") != std::string::npos &&
			c.mimeType.find("rtx") == std::string::npos) {
			room.pt = c.preferredPayloadType;
			break;
		}
	}

	PlainTransportOptions opts;
	opts.listenInfos = {{{"ip", "127.0.0.1"}}};
	opts.rtcpMux = true;
	opts.comedia = false;
	room.prodTransport = room.router->createPlainTransport(opts);

	uint16_t senderPort;
	room.senderFd = createUdpSocket(senderPort);
	room.prodTransport->connect("127.0.0.1", senderPort);

	room.destAddr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &room.destAddr.sin_addr);
	room.destAddr.sin_port = htons(room.prodTransport->tuple().localPort);

	json rtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"payloadType", room.pt},
			{"clockRate", 48000}, {"channels", 2}
		}}},
		{"encodings", {{{"ssrc", room.ssrc}}}},
		{"mid", "0"}
	};
	room.producer = room.prodTransport->produce({
		{"kind", "audio"}, {"rtpParameters", rtpParams},
		{"routerRtpCapabilities", caps}
	});

	return room;
}

static void sendPacket(TestRoom& room) {
	uint8_t buf[200];
	buf[0] = 0x80;
	buf[1] = room.pt | 0x80; // marker
	buf[2] = (room.seq >> 8) & 0xFF;
	buf[3] = room.seq & 0xFF;
	buf[4] = (room.ts >> 24) & 0xFF;
	buf[5] = (room.ts >> 16) & 0xFF;
	buf[6] = (room.ts >> 8) & 0xFF;
	buf[7] = room.ts & 0xFF;
	buf[8] = (room.ssrc >> 24) & 0xFF;
	buf[9] = (room.ssrc >> 16) & 0xFF;
	buf[10] = (room.ssrc >> 8) & 0xFF;
	buf[11] = room.ssrc & 0xFF;
	memset(buf + 12, 0xAB, 188);
	sendto(room.senderFd, buf, 200, 0, (sockaddr*)&room.destAddr, sizeof(room.destAddr));
	room.seq++;
	room.ts += 960; // 20ms at 48kHz
}

class QosAccuracyTest : public ::testing::Test {
protected:
	std::shared_ptr<Worker> worker;

	void SetUp() override {
		WorkerSettings settings;
		settings.workerBin = "./mediasoup-worker";
		settings.rtcMinPort = 40000;
		settings.rtcMaxPort = 40999;
		worker = std::make_shared<Worker>(settings);
	}

	void TearDown() override {
		if (worker) worker->close();
	}
};

// ─── Test 1: No loss → packetsLost=0, fractionLost=0 ───
TEST_F(QosAccuracyTest, NoLossReportsZero) {
	auto room = createTestRoom(worker);

	// Send 500 packets with no gaps (10 seconds at 50pps)
	for (int i = 0; i < 500; i++) {
		sendPacket(room);
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	// Wait for RTCP to propagate
	std::this_thread::sleep_for(std::chrono::seconds(2));

	auto stats = room.producer->getStats();
	ASSERT_FALSE(stats.empty());
	auto& s = stats[0];
	EXPECT_EQ(s.value("packetsLost", -1), 0) << "Expected 0 lost packets";
	EXPECT_NEAR(s.value("fractionLost", 1.0), 0.0, 0.01) << "Expected ~0% fraction lost";
	EXPECT_GT(s.value("packetCount", 0), 400) << "Should have received most packets";
}

// ─── Test 2: 10% loss → fractionLost ≈ 0.1 ───
TEST_F(QosAccuracyTest, TenPercentLossDetected) {
	auto room = createTestRoom(worker);

	std::mt19937 rng(42); // deterministic seed
	int sent = 0, dropped = 0;

	// Send 500 packets, skip ~10% (advance seq but don't send)
	for (int i = 0; i < 500; i++) {
		if (rng() % 10 == 0) {
			// Simulate loss: advance seq/ts but don't send
			room.seq++;
			room.ts += 960;
			dropped++;
		} else {
			sendPacket(room);
			sent++;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	std::this_thread::sleep_for(std::chrono::seconds(3));

	auto stats = room.producer->getStats();
	ASSERT_FALSE(stats.empty());
	auto& s = stats[0];
	int lost = s.value("packetsLost", 0);
	double frac = s.value("fractionLost", 0.0);

	// fractionLost is in RTCP format: 0-255 (RFC 3550), not 0.0-1.0
	// 10% loss ≈ 25.6 in RTCP units
	double fracNorm = frac / 256.0;

	// packetsLost should be close to dropped count
	EXPECT_GT(lost, dropped * 0.5) << "Lost too few: " << lost << " vs dropped " << dropped;
	EXPECT_LT(lost, dropped * 1.5) << "Lost too many: " << lost << " vs dropped " << dropped;

	// fractionLost normalized should be roughly 10%
	EXPECT_GT(fracNorm, 0.03) << "fractionLost too low: " << fracNorm;
	EXPECT_LT(fracNorm, 0.25) << "fractionLost too high: " << fracNorm;
}

// ─── Test 3: 50% loss → fractionLost ≈ 0.5 ───
TEST_F(QosAccuracyTest, FiftyPercentLossDetected) {
	auto room = createTestRoom(worker);

	int sent = 0, dropped = 0;

	// Send every other packet
	for (int i = 0; i < 500; i++) {
		if (i % 2 == 0) {
			room.seq++;
			room.ts += 960;
			dropped++;
		} else {
			sendPacket(room);
			sent++;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	std::this_thread::sleep_for(std::chrono::seconds(3));

	auto stats = room.producer->getStats();
	ASSERT_FALSE(stats.empty());
	auto& s = stats[0];
	int lost = s.value("packetsLost", 0);
	double frac = s.value("fractionLost", 0.0);

	EXPECT_GT(lost, dropped * 0.5);
	EXPECT_GT(frac, 0.2) << "fractionLost should be significant: " << frac;
}

// ─── Test 4: Jitter injection → jitter > 0 ───
TEST_F(QosAccuracyTest, JitterDetected) {
	auto room = createTestRoom(worker);

	std::mt19937 rng(123);
	std::uniform_int_distribution<int> jitterDist(0, 40); // 0-40ms random jitter

	for (int i = 0; i < 500; i++) {
		sendPacket(room);
		int delay = 20 + jitterDist(rng); // 20ms base + 0-40ms jitter
		std::this_thread::sleep_for(std::chrono::milliseconds(delay));
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));

	auto stats = room.producer->getStats();
	ASSERT_FALSE(stats.empty());
	auto& s = stats[0];
	uint32_t jitter = s.value("jitter", 0);

	// Jitter in RTP timestamp units (48kHz). 20ms jitter = 960 units.
	// With 0-40ms random jitter, expect noticeable jitter value.
	EXPECT_GT(jitter, 0) << "Jitter should be non-zero with random delays";
}

// ─── Test 5: No jitter → low jitter value ───
TEST_F(QosAccuracyTest, SteadyStreamLowJitter) {
	auto room = createTestRoom(worker);

	// Send at perfectly steady 20ms intervals
	auto next = std::chrono::steady_clock::now();
	for (int i = 0; i < 300; i++) {
		sendPacket(room);
		next += std::chrono::milliseconds(20);
		std::this_thread::sleep_for(next - std::chrono::steady_clock::now());
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));

	auto stats = room.producer->getStats();
	ASSERT_FALSE(stats.empty());
	auto& s = stats[0];
	uint32_t jitter = s.value("jitter", 9999);

	// Steady stream should have very low jitter
	// 48kHz clock: 1ms = 48 units. Allow up to ~5ms jitter from OS scheduling.
	EXPECT_LT(jitter, 48 * 5) << "Jitter too high for steady stream: "
		<< jitter << " units = " << (jitter / 48.0) << " ms";
}

// ─── Test 6: Bitrate accuracy ───
TEST_F(QosAccuracyTest, BitrateAccuracy) {
	auto room = createTestRoom(worker);

	// Send 200-byte packets at 50pps for 8 seconds
	// Expected bitrate: 200 * 8 * 50 = 80000 bps
	for (int i = 0; i < 400; i++) {
		sendPacket(room);
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	// Sample stats while still "warm" (bitrate is a moving average that decays)
	auto stats = room.producer->getStats();
	ASSERT_FALSE(stats.empty());
	auto& s = stats[0];
	int64_t bitrate = s.value("bitrate", 0);

	// Allow wide margin — Worker's bitrate is a moving average
	EXPECT_GT(bitrate, 20000) << "Bitrate too low: " << bitrate;
	EXPECT_LT(bitrate, 200000) << "Bitrate too high: " << bitrate;
}

// ─── Test 7: Packet count accuracy ───
TEST_F(QosAccuracyTest, PacketCountAccuracy) {
	auto room = createTestRoom(worker);

	int totalSent = 300;
	for (int i = 0; i < totalSent; i++) {
		sendPacket(room);
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));

	auto stats = room.producer->getStats();
	ASSERT_FALSE(stats.empty());
	auto& s = stats[0];
	int count = s.value("packetCount", 0);

	// Should match exactly (no loss on loopback)
	EXPECT_GE(count, totalSent - 5) << "Packet count too low";
	EXPECT_LE(count, totalSent + 5) << "Packet count too high";
}

// ─── Test 8: Score is reported via getStats ───
TEST_F(QosAccuracyTest, ScoreReportedInStats) {
	auto room = createTestRoom(worker);

	// Send enough packets for Worker to compute score
	for (int i = 0; i < 300; i++) {
		sendPacket(room);
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	std::this_thread::sleep_for(std::chrono::seconds(2));

	auto stats = room.producer->getStats();
	ASSERT_FALSE(stats.empty());
	auto& s = stats[0];

	// Score should be present in stats and be a valid value (0-10)
	EXPECT_TRUE(s.contains("score")) << "Stats should contain score field";
	int score = s.value("score", -1);
	EXPECT_GE(score, 0);
	EXPECT_LE(score, 10);
}
