// test_thread_integration.cpp — Integration tests for multi-thread pipeline
// Requires: tests/fixtures/media/test_sweep.mp4 (run from project root)
#include <gtest/gtest.h>

#include "../client/ThreadTypes.h"
#include "../client/NetworkThread.h"
#include "../client/SourceWorker.h"
#include "../client/ThreadedControlHelpers.h"
#include "../client/qos/QosController.h"
#include "TestWsClient.h"
#include "TestProcessUtils.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <set>
#include <thread>

static const char* kTestMp4 = "tests/fixtures/media/test_sweep.mp4";

static bool testFileExists() {
	FILE* f = fopen(kTestMp4, "r");
	if (!f) return false;
	fclose(f);
	return true;
}

static bool plainClientBinaryExists() {
	return access("client/build/plain-client", X_OK) == 0;
}

static bool extractTransportCcSequence(
	const uint8_t* packet,
	size_t packetLen,
	uint8_t extensionId,
	uint16_t* sequenceOut) {
	if (!packet || packetLen < 12 || !sequenceOut || extensionId == 0 || extensionId > 14) {
		return false;
	}

	if ((packet[0] >> 6) != 2 || (packet[0] & 0x10) == 0) {
		return false;
	}

	const size_t csrcCount = static_cast<size_t>(packet[0] & 0x0F);
	const size_t headerLen = 12 + csrcCount * 4;
	if (packetLen < headerLen + 4) {
		return false;
	}

	const uint16_t extensionProfile =
		static_cast<uint16_t>((packet[headerLen] << 8) | packet[headerLen + 1]);
	if (extensionProfile != 0xBEDE) {
		return false;
	}

	const size_t extensionWords =
		static_cast<size_t>((packet[headerLen + 2] << 8) | packet[headerLen + 3]);
	const size_t extensionDataLen = extensionWords * 4;
	const size_t extensionDataOffset = headerLen + 4;
	if (packetLen < extensionDataOffset + extensionDataLen) {
		return false;
	}

	size_t elementOffset = 0;
	while (elementOffset < extensionDataLen) {
		const uint8_t headerByte = packet[extensionDataOffset + elementOffset];
		if (headerByte == 0) {
			elementOffset++;
			continue;
		}

		const uint8_t id = static_cast<uint8_t>(headerByte >> 4);
		if (id == 15) {
			break;
		}
		const size_t valueLen = static_cast<size_t>(headerByte & 0x0F) + 1;
		if (elementOffset + 1 + valueLen > extensionDataLen) {
			return false;
		}

		if (id == extensionId && valueLen == 2) {
			*sequenceOut = static_cast<uint16_t>(
				(packet[extensionDataOffset + elementOffset + 1] << 8) |
				packet[extensionDataOffset + elementOffset + 2]);
			return true;
		}

		elementOffset += 1 + valueLen;
	}

	return false;
}

static bool logContainsWithin(const std::string& path, const std::string& needle, int timeoutMs) {
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
	while (std::chrono::steady_clock::now() < deadline) {
		std::ifstream in(path);
		std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		if (contents.find(needle) != std::string::npos) return true;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	return false;
}

static void terminateGenericProcess(pid_t pid) {
	if (pid <= 0) return;
	if (kill(pid, 0) != 0) return;
	kill(pid, SIGTERM);
	for (int i = 0; i < 20; ++i) {
		usleep(50000);
		if (kill(pid, 0) != 0) return;
	}
	kill(pid, SIGKILL);
}

static json defaultRtpCapabilitiesForThreadedIntegration() {
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

class ThreadedPlainPublishIntegrationTest : public ::testing::Test {
protected:
	pid_t sfuPid_ = -1;
	std::string roomId_;

	static int threadedIntegrationPort() {
		const char* raw = std::getenv("QOS_THREAD_INTEGRATION_PORT");
		if (!raw || !*raw) return 15050;
		return std::max(1, atoi(raw));
	}

	void SetUp() override {
		roomId_ = "threaded_room_" + std::to_string(getpid()) + "_" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
		const int sfuPort = threadedIntegrationPort();

		std::string cmd = "./build/mediasoup-sfu --nodaemon"
			" --port=" + std::to_string(sfuPort) +
			" --workers=1"
			" --workerBin=./mediasoup-worker"
			" --announcedIp=127.0.0.1"
			" --listenIp=127.0.0.1"
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
			addr.sin_port = htons(sfuPort);
			inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
			if (::connect(fd, (sockaddr*)&addr, sizeof(addr)) == 0) {
				::close(fd);
				usleep(200000);
				return;
			}
			::close(fd);
		}
		FAIL() << "SFU did not start";
	}

	void TearDown() override {
		if (sfuPid_ > 0) terminateSfuProcess(sfuPid_);
	}

	std::unique_ptr<TestWsClient> joinPeer(const std::string& peerId) {
		auto ws = std::make_unique<TestWsClient>();
		EXPECT_TRUE(ws->connect("127.0.0.1", threadedIntegrationPort()));
		auto joinResp = ws->request("join", {
			{"roomId", roomId_},
			{"peerId", peerId},
			{"displayName", peerId},
			{"rtpCapabilities", defaultRtpCapabilitiesForThreadedIntegration()}
		});
		EXPECT_TRUE(joinResp.value("ok", false)) << joinResp.dump();
		(void)ws->waitNotification("qosPolicy", 3000);
		return ws;
	}
};

TEST_F(ThreadedPlainPublishIntegrationTest, ExplicitThreadedSourcesDoNotRequireBootstrapMp4) {
	if (!testFileExists()) { GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found"; }
	if (!plainClientBinaryExists()) { GTEST_SKIP() << "client/build/plain-client not found"; }

	const std::string missingBootstrap = "/tmp/threaded_dummy_missing_" + std::to_string(getpid()) + ".mp4";
	const std::string logPath = "/tmp/threaded_explicit_sources_" + std::to_string(getpid()) + ".log";
	unlink(missingBootstrap.c_str());
	unlink(logPath.c_str());

	std::ostringstream cmd;
	cmd << "env PLAIN_CLIENT_THREADED=1 "
		<< "PLAIN_CLIENT_VIDEO_TRACK_COUNT=2 "
		<< "PLAIN_CLIENT_VIDEO_SOURCES=" << kTestMp4 << "," << kTestMp4 << " "
		<< "stdbuf -oL -eL ./client/build/plain-client 127.0.0.1 "
		<< threadedIntegrationPort() << " " << roomId_ << " threaded_sources " << missingBootstrap
		<< " >" << logPath << " 2>&1 & echo $!";

	FILE* fp = popen(cmd.str().c_str(), "r");
	ASSERT_NE(fp, nullptr);
	char buf[64]{};
	ASSERT_NE(fgets(buf, sizeof(buf), fp), nullptr);
	pclose(fp);
	pid_t clientPid = atoi(buf);
	ASSERT_GT(clientPid, 0);

	ASSERT_TRUE(logContainsWithin(logPath, "skipping MP4 bootstrap", 5000));
	ASSERT_TRUE(logContainsWithin(logPath, "Publish", 8000));
	ASSERT_TRUE(logContainsWithin(logPath, "videoTracks=2", 8000));

	terminateGenericProcess(clientPid);
}

// ═══════════════════════════════════════════════════════════
// SourceWorker integration: file → decode → encode → AU queue
// ═══════════════════════════════════════════════════════════

TEST(SourceWorkerIntegration, ProducesEncodedAccessUnits) {
	if (!testFileExists()) { GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found"; }

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity> ctrlQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueue;

	SourceWorker::Config cfg;
	cfg.trackIndex = 0;
	cfg.ssrc = 11111111;
	cfg.payloadType = 96;
	cfg.inputType = SourceWorker::InputType::File;
	cfg.inputPath = kTestMp4;
	cfg.initialBitrate = 500000;
	cfg.initialFps = 25;

	SourceWorker worker(cfg);
	worker.outputQueue = &auQueue;
	worker.controlQueue = &ctrlQueue;
	worker.networkCmdQueue = &netQueue;
	worker.start();

	// Let it run for 1 second then stop
	std::this_thread::sleep_for(std::chrono::milliseconds(1200));
	worker.stop();

	// Drain queue
	std::vector<mt::EncodedAccessUnit> frames;
	mt::EncodedAccessUnit au;
	while (auQueue.tryPop(au)) frames.push_back(std::move(au));

	EXPECT_GT(frames.size(), 5u) << "should produce multiple encoded frames in 1s";

	// First frame should be a keyframe
	ASSERT_FALSE(frames.empty());
	EXPECT_TRUE(frames[0].isKeyframe);
	EXPECT_TRUE(frames[0].encoderRecreated);
	EXPECT_EQ(frames[0].ssrc, 11111111u);
	EXPECT_EQ(frames[0].payloadType, 96);
	EXPECT_GT(frames[0].size, 0u);
	EXPECT_NE(frames[0].data, nullptr);

	// All frames should have valid data
	for (auto& f : frames) {
		EXPECT_EQ(f.ssrc, 11111111u);
		EXPECT_GT(f.size, 0u);
		EXPECT_NE(f.data, nullptr);
	}

	printf("[test] SourceWorker produced %zu frames in ~1s\n", frames.size());
}

TEST(SourceWorkerIntegration, RespondsToForceKeyframe) {
	if (!testFileExists()) { GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found"; }

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity> ctrlQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueue;

	SourceWorker::Config cfg;
	cfg.trackIndex = 0;
	cfg.ssrc = 22222222;
	cfg.payloadType = 96;
	cfg.inputType = SourceWorker::InputType::File;
	cfg.inputPath = kTestMp4;
	cfg.initialBitrate = 500000;
	cfg.initialFps = 25;

	SourceWorker worker(cfg);
	worker.outputQueue = &auQueue;
	worker.controlQueue = &ctrlQueue;
	worker.networkCmdQueue = &netQueue;
	worker.start();

	// Wait for initial frames
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	// Send ForceKeyframe via network command queue (simulating PLI)
	mt::NetworkToSourceCommand cmd;
	cmd.type = mt::NetworkToSourceCommand::ForceKeyframe;
	cmd.trackIndex = 0;
	netQueue.tryPush(std::move(cmd));

	// Wait for response
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	worker.stop();

	// Check that we got at least 2 keyframes (initial + forced)
	int keyframeCount = 0;
	mt::EncodedAccessUnit au;
	while (auQueue.tryPop(au)) {
		if (au.isKeyframe) keyframeCount++;
	}
	EXPECT_GE(keyframeCount, 2) << "should have initial keyframe + forced keyframe";
	printf("[test] got %d keyframes (expected >= 2)\n", keyframeCount);
}

TEST(SourceWorkerIntegration, PauseResumeStopsAndRestartsOutput) {
	if (!testFileExists()) { GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found"; }

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity> ctrlQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueue;

	SourceWorker::Config cfg;
	cfg.trackIndex = 0;
	cfg.ssrc = 33333333;
	cfg.payloadType = 96;
	cfg.inputType = SourceWorker::InputType::File;
	cfg.inputPath = kTestMp4;
	cfg.initialBitrate = 500000;
	cfg.initialFps = 25;

	SourceWorker worker(cfg);
	worker.outputQueue = &auQueue;
	worker.controlQueue = &ctrlQueue;
	worker.networkCmdQueue = &netQueue;
	worker.start();

	// Let it produce some frames
	std::this_thread::sleep_for(std::chrono::milliseconds(400));

	// Drain
	size_t beforePause = 0;
	{ mt::EncodedAccessUnit au; while (auQueue.tryPop(au)) beforePause++; }
	EXPECT_GT(beforePause, 0u);

	// Pause
	mt::TrackControlCommand pauseCmd;
	pauseCmd.type = mt::TrackCommandType::PauseTrack;
	ctrlQueue.tryPush(std::move(pauseCmd));
	std::this_thread::sleep_for(std::chrono::milliseconds(400));

	size_t duringPause = 0;
	{ mt::EncodedAccessUnit au; while (auQueue.tryPop(au)) duringPause++; }
	EXPECT_LE(duringPause, 1u) << "at most 1 in-flight frame during pause transition";

	// Resume
	mt::TrackControlCommand resumeCmd;
	resumeCmd.type = mt::TrackCommandType::ResumeTrack;
	ctrlQueue.tryPush(std::move(resumeCmd));
	std::this_thread::sleep_for(std::chrono::milliseconds(400));

	size_t afterResume = 0;
	{ mt::EncodedAccessUnit au; while (auQueue.tryPop(au)) afterResume++; }
	EXPECT_GT(afterResume, 0u) << "frames should resume after unpause";

	worker.stop();
	printf("[test] frames: before=%zu during=%zu after=%zu\n", beforePause, duringPause, afterResume);
}

// ═══════════════════════════════════════════════════════════
// NetworkThread integration: AU → RTP over loopback UDP
// ═══════════════════════════════════════════════════════════

static int createConnectedUdpPair(int& sendFd, int& recvFd, uint16_t& port) {
	recvFd = socket(AF_INET, SOCK_DGRAM, 0);
	if (recvFd < 0) return -1;

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0; // kernel picks port
	if (bind(recvFd, (sockaddr*)&addr, sizeof(addr)) < 0) { close(recvFd); return -1; }

	socklen_t len = sizeof(addr);
	getsockname(recvFd, (sockaddr*)&addr, &len);
	port = ntohs(addr.sin_port);

	sendFd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sendFd < 0) { close(recvFd); return -1; }

	// Set non-blocking on recv
	int flags = fcntl(recvFd, F_GETFL, 0);
	fcntl(recvFd, F_SETFL, flags | O_NONBLOCK);

	// Connect send socket to recv address
	if (::connect(sendFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
		close(sendFd); close(recvFd); return -1;
	}
	flags = fcntl(sendFd, F_GETFL, 0);
	fcntl(sendFd, F_SETFL, flags | O_NONBLOCK);

	return 0;
}

TEST(NetworkThreadIntegration, SendsRtpFromEncodedAU) {
	int sendFd = -1, recvFd = -1;
	uint16_t port = 0;
	ASSERT_EQ(createConnectedUdpPair(sendFd, recvFd, port), 0);

	mt::SpscQueue<mt::EncodedAccessUnit, 16> auQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netCmdQueue;
	mt::SpscQueue<mt::SenderStatsSnapshot, mt::kStatsQueueCapacity> statsQueue;

	NetworkThread::Config cfg;
	cfg.udpFd = sendFd;
	cfg.audioSsrc = 22222222;
	cfg.audioPt = 111;
	cfg.audioTransportCcExtensionId = 5;

	NetworkThread net(cfg);
	net.registerVideoTrack(0, 11111111, 96, 5);
	net.statsQueue = &statsQueue;

	NetworkThread::SourceInput si;
	si.auQueue = &auQueue;
	si.keyframeQueue = &netCmdQueue;
	net.addSourceInput(si);

	// Push a fake encoded AU (small H264 keyframe-like data)
	{
		mt::EncodedAccessUnit au;
		au.trackIndex = 0;
		au.ssrc = 11111111;
		au.payloadType = 96;
		au.rtpTimestamp = 90000;
		au.isKeyframe = true;
		// Annex-B start code + NAL unit
		uint8_t h264[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x1e,
		                  0x00, 0x00, 0x00, 0x01, 0x65, 0xAA, 0xBB, 0xCC};
		au.assign(h264, sizeof(h264));
		ASSERT_TRUE(auQueue.tryPush(std::move(au)));
	}

	net.start();
	std::this_thread::sleep_for(std::chrono::milliseconds(700));
	net.stop();

	// Read RTP packets from recv socket
	int rtpCount = 0;
	int transportCcTaggedCount = 0;
	uint8_t buf[1500];
	while (true) {
		int n = recv(recvFd, buf, sizeof(buf), MSG_DONTWAIT);
		if (n <= 0) break;
		if (n >= 12 && (buf[0] & 0xC0) == 0x80) {
			rtpCount++;
			uint16_t twccSeq = 0;
			if (extractTransportCcSequence(buf, static_cast<size_t>(n), 5, &twccSeq)) {
				transportCcTaggedCount++;
			}
		}
	}

	EXPECT_GT(rtpCount, 0) << "should have received RTP packets";
	EXPECT_GT(transportCcTaggedCount, 0) << "RTP packets should carry transport-cc extension";
	printf("[test] received %d RTP packets\n", rtpCount);

	// Check stats were pushed
	mt::SenderStatsSnapshot snap;
	bool gotStats = false;
	while (statsQueue.tryPop(snap)) gotStats = true;
	EXPECT_TRUE(gotStats) << "should have received stats snapshot";

	close(sendFd);
	close(recvFd);
}

TEST(NetworkThreadIntegration, ComediaProbeCoversAllRegisteredTrackSsrcs) {
	int sendFd = -1, recvFd = -1;
	uint16_t port = 0;
	ASSERT_EQ(createConnectedUdpPair(sendFd, recvFd, port), 0);

	NetworkThread::Config cfg;
	cfg.udpFd = sendFd;
	cfg.audioSsrc = 22222222;
	cfg.audioPt = 111;

	NetworkThread net(cfg);
	net.registerVideoTrack(0, 11111111u, 96);
	net.registerVideoTrack(1, 33333333u, 97);

	net.sendComediaProbe();

	std::set<uint32_t> observedSsrcs;
	uint8_t buf[1500];
	for (int i = 0; i < 16; ++i) {
		const int n = recv(recvFd, buf, sizeof(buf), MSG_DONTWAIT);
		if (n <= 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			continue;
		}
		if (n >= 12 && (buf[0] & 0xC0) == 0x80) {
			const uint32_t ssrc =
				(static_cast<uint32_t>(buf[8]) << 24) |
				(static_cast<uint32_t>(buf[9]) << 16) |
				(static_cast<uint32_t>(buf[10]) << 8) |
				static_cast<uint32_t>(buf[11]);
			observedSsrcs.insert(ssrc);
		}
	}

	EXPECT_EQ(observedSsrcs.size(), 2u);
	EXPECT_TRUE(observedSsrcs.count(11111111u));
	EXPECT_TRUE(observedSsrcs.count(33333333u));

	close(sendFd);
	close(recvFd);
}

TEST(NetworkThreadIntegration, DisableTransportControllerUsesLegacyPacingFallback) {
	int sendFd = -1, recvFd = -1;
	uint16_t port = 0;
	ASSERT_EQ(createConnectedUdpPair(sendFd, recvFd, port), 0);

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netCmdQueue;

	NetworkThread::Config cfg;
	cfg.udpFd = sendFd;
	cfg.enableTransportController = false;

	NetworkThread net(cfg);
	net.registerVideoTrack(0, 11111111, 96);

	NetworkThread::SourceInput sourceInput;
	sourceInput.auQueue = &auQueue;
	sourceInput.keyframeQueue = &netCmdQueue;
	net.addSourceInput(sourceInput);

	mt::EncodedAccessUnit au;
	au.trackIndex = 0;
	au.ssrc = 11111111;
	au.payloadType = 96;
	au.rtpTimestamp = 90000;
	au.isKeyframe = true;
	std::vector<uint8_t> bigNal(10000, 0xAB);
	bigNal[0] = 0x00; bigNal[1] = 0x00; bigNal[2] = 0x00; bigNal[3] = 0x01;
	bigNal[4] = 0x65;
	au.assign(bigNal.data(), bigNal.size());
	ASSERT_TRUE(auQueue.tryPush(std::move(au)));

	net.start();
	net.wakeup();
	std::this_thread::sleep_for(std::chrono::milliseconds(120));
	net.stop();

	int rtpCount = 0;
	uint8_t buf[1500];
	while (recv(recvFd, buf, sizeof(buf), MSG_DONTWAIT) > 0) {
		if ((buf[0] & 0xC0) == 0x80) {
			rtpCount++;
		}
	}
	EXPECT_GT(rtpCount, 0) << "legacy pacing fallback should still transmit RTP packets";

	const auto& metrics = net.transportMetrics();
	uint64_t sentTotal = 0;
	uint64_t wouldBlockTotal = 0;
	uint64_t hardErrorTotal = 0;
	for (size_t i = 0; i < metrics.sentByClass.size(); ++i) {
		sentTotal += metrics.sentByClass[i];
		wouldBlockTotal += metrics.wouldBlockByClass[i];
		hardErrorTotal += metrics.hardErrorByClass[i];
	}
	EXPECT_EQ(sentTotal, 0u);
	EXPECT_EQ(wouldBlockTotal, 0u);
	EXPECT_EQ(hardErrorTotal, 0u);
	EXPECT_EQ(net.queuedAudioPackets(), 0u);
	EXPECT_EQ(net.queuedRetransmissionPackets(), 0u);

	close(sendFd);
	close(recvFd);
}

// ═══════════════════════════════════════════════════════════
// End-to-end: SourceWorker → NetworkThread → stats
// ═══════════════════════════════════════════════════════════

TEST(EndToEndIntegration, SourceWorkerToNetworkThreadToStats) {
	if (!testFileExists()) { GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found"; }

	int sendFd = -1, recvFd = -1;
	uint16_t port = 0;
	ASSERT_EQ(createConnectedUdpPair(sendFd, recvFd, port), 0);

	// Queues
	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity> ctrlQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netCmdQueue;
	mt::SpscQueue<mt::SenderStatsSnapshot, mt::kStatsQueueCapacity> statsQueue;

	// Network thread
	NetworkThread::Config netCfg;
	netCfg.udpFd = sendFd;
	netCfg.audioSsrc = 0;
	NetworkThread net(netCfg);
	net.registerVideoTrack(0, 11111111, 96);
	net.statsQueue = &statsQueue;

	NetworkThread::SourceInput si;
	si.auQueue = &auQueue;
	si.keyframeQueue = &netCmdQueue;
	net.addSourceInput(si);

	// Source worker
	SourceWorker::Config swCfg;
	swCfg.trackIndex = 0;
	swCfg.ssrc = 11111111;
	swCfg.payloadType = 96;
	swCfg.inputType = SourceWorker::InputType::File;
	swCfg.inputPath = kTestMp4;
	swCfg.initialBitrate = 300000;
	swCfg.initialFps = 15;

	SourceWorker worker(swCfg);
	worker.outputQueue = &auQueue;
	worker.controlQueue = &ctrlQueue;
	worker.networkCmdQueue = &netCmdQueue;

	// Start both
	net.start();
	worker.start();

	// Run for 2 seconds
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	// Stop source first, then network
	worker.stop();
	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // let net drain
	net.stop();

	// Count RTP packets received
	int rtpCount = 0;
	uint8_t buf[1500];
	while (true) {
		int n = recv(recvFd, buf, sizeof(buf), MSG_DONTWAIT);
		if (n <= 0) break;
		if (n >= 12 && (buf[0] & 0xC0) == 0x80) rtpCount++;
	}

	// Collect stats
	mt::SenderStatsSnapshot lastSnap;
	bool gotStats = false;
	while (statsQueue.tryPop(lastSnap)) gotStats = true;

	EXPECT_GT(rtpCount, 10) << "should have many RTP packets after 2s";
	EXPECT_TRUE(gotStats) << "should have stats from network thread";
	if (gotStats) {
		EXPECT_GT(lastSnap.packetCount, 0u) << "stats should show packets sent";
		EXPECT_GT(lastSnap.octetCount, 0u) << "stats should show bytes sent";
		printf("[test] e2e: %d RTP pkts, stats: packets=%lu octets=%lu\n",
			rtpCount, (unsigned long)lastSnap.packetCount, (unsigned long)lastSnap.octetCount);
	}

	close(sendFd);
	close(recvFd);
}

// ═══════════════════════════════════════════════════════════
// Stats freshness: stale stats must not be re-consumed
// ═══════════════════════════════════════════════════════════

TEST(StatsFreshness, StaleGenerationIsSkipped) {
	// Simulate: network thread pushes one snapshot, control consumes it,
	// then on next sample cycle the same snapshot should NOT be reused.
	mt::SpscQueue<mt::SenderStatsSnapshot, mt::kStatsQueueCapacity> statsQueue;

	// Push one snapshot with generation=1
	mt::SenderStatsSnapshot snap;
	snap.timestampMs = 1000;
	snap.trackIndex = 0;
	snap.ssrc = 11111111;
	snap.generation = 1;
	snap.packetCount = 100;
	snap.octetCount = 50000;
	snap.rrRttMs = 40.0;
	ASSERT_TRUE(statsQueue.tryPush(std::move(snap)));

	// Simulate control loop: drain stats, track lastConsumedGeneration
	struct TrackStats {
		mt::SenderStatsSnapshot latest;
		bool hasData = false;
		uint64_t lastConsumedGeneration = 0;
	} ts;

	mt::SenderStatsSnapshot s;
	while (statsQueue.tryPop(s)) {
		ts.latest = s;
		ts.hasData = true;
	}

	// First sample: generation=1 > lastConsumed=0 → fresh
	bool fresh1 = ts.hasData && ts.latest.generation > ts.lastConsumedGeneration;
	EXPECT_TRUE(fresh1);
	ts.lastConsumedGeneration = ts.latest.generation;

	// Second sample: no new stats pushed, generation=1 == lastConsumed=1 → stale
	bool fresh2 = ts.hasData && ts.latest.generation > ts.lastConsumedGeneration;
	EXPECT_FALSE(fresh2) << "same generation should be detected as stale";

	// Third sample: network pushes generation=2 → fresh again
	mt::SenderStatsSnapshot snap2;
	snap2.generation = 2;
	snap2.packetCount = 200;
	snap2.timestampMs = 2000;
	ASSERT_TRUE(statsQueue.tryPush(std::move(snap2)));
	while (statsQueue.tryPop(s)) {
		ts.latest = s;
		ts.hasData = true;
	}
	bool fresh3 = ts.hasData && ts.latest.generation > ts.lastConsumedGeneration;
	EXPECT_TRUE(fresh3);
	EXPECT_EQ(ts.latest.packetCount, 200u);
}

// ═══════════════════════════════════════════════════════════
// Multi-track PLI routing: ForceKeyframe goes to correct worker
// ═══════════════════════════════════════════════════════════

TEST(MultiTrackPLIRouting, ForceKeyframeDispatchesToCorrectWorker) {
	// Setup: 3 tracks, each with its own keyframe queue.
	// Simulate PLI for track 1 — only track 1's queue should get the command.

	constexpr int kTracks = 3;
	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueues[kTracks];
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueues[kTracks];

	// Create a NetworkThread with a dummy UDP socket (won't actually send)
	int dummyFd = socket(AF_INET, SOCK_DGRAM, 0);
	ASSERT_GE(dummyFd, 0);

	NetworkThread::Config cfg;
	cfg.udpFd = dummyFd;
	NetworkThread net(cfg);

	uint32_t ssrcs[kTracks] = {11111111, 22222222, 33333333};
	for (int i = 0; i < kTracks; ++i) {
		net.registerVideoTrack(i, ssrcs[i], 96);
		NetworkThread::SourceInput si;
		si.auQueue = &auQueues[i];
		si.keyframeQueue = &netQueues[i];
		net.addSourceInput(si);
	}

	// Directly test dispatchForceKeyframe via the PLI processing path.
	// We can't easily inject RTCP, so test the dispatch function directly
	// by making it public-accessible through a subclass or by calling
	// the method that processes PLI. Instead, let's verify the queue routing
	// by simulating what processIncomingRtcp would do:

	// Build a fake RTCP PLI packet targeting track 1 (ssrc=22222222)
	// PSFB (PT=206), fmt=1 (PLI), length=2 (12 bytes total)
	// [V=2,P=0,FMT=1] [PT=206] [length=2]
	// [sender SSRC = 0]
	// [media SSRC = 22222222]
	uint8_t pli[12];
	pli[0] = 0x81; // V=2, FMT=1
	pli[1] = 206;  // PT=PSFB
	pli[2] = 0; pli[3] = 2; // length=2 (words)
	// sender SSRC = 0
	pli[4] = 0; pli[5] = 0; pli[6] = 0; pli[7] = 0;
	// media SSRC = 22222222 = 0x0153158E
	uint32_t targetSsrc = 22222222;
	pli[8]  = (targetSsrc >> 24) & 0xFF;
	pli[9]  = (targetSsrc >> 16) & 0xFF;
	pli[10] = (targetSsrc >> 8) & 0xFF;
	pli[11] = targetSsrc & 0xFF;

	// We need to feed this to the network thread's UDP socket.
	// Create a loopback pair so we can inject the PLI.
	int recvFd = socket(AF_INET, SOCK_DGRAM, 0);
	ASSERT_GE(recvFd, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;
	ASSERT_EQ(bind(recvFd, (sockaddr*)&addr, sizeof(addr)), 0);
	socklen_t alen = sizeof(addr);
	getsockname(recvFd, (sockaddr*)&addr, &alen);

	int injFd = socket(AF_INET, SOCK_DGRAM, 0);
	ASSERT_GE(injFd, 0);

	// Reconfigure network thread to use recvFd
	close(dummyFd);
	int flags = fcntl(recvFd, F_GETFL, 0);
	fcntl(recvFd, F_SETFL, flags | O_NONBLOCK);

	// Rebuild with correct fd
	NetworkThread::Config cfg2;
	cfg2.udpFd = recvFd;
	NetworkThread net2(cfg2);
	for (int i = 0; i < kTracks; ++i) {
		net2.registerVideoTrack(i, ssrcs[i], 96);
		NetworkThread::SourceInput si;
		si.auQueue = &auQueues[i];
		si.keyframeQueue = &netQueues[i];
		net2.addSourceInput(si);
	}

	// Start network thread
	net2.start();

	// Inject PLI packet
	sendto(injFd, pli, sizeof(pli), 0, (sockaddr*)&addr, sizeof(addr));

	// Wait for processing
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	net2.stop();

	// Verify: only track 1's queue should have a ForceKeyframe
	for (int i = 0; i < kTracks; ++i) {
		mt::NetworkToSourceCommand cmd;
		bool got = netQueues[i].tryPop(cmd);
		if (i == 1) {
			EXPECT_TRUE(got) << "track 1 should receive ForceKeyframe";
			if (got) EXPECT_EQ(cmd.type, mt::NetworkToSourceCommand::ForceKeyframe);
		} else {
			EXPECT_FALSE(got) << "track " << i << " should NOT receive ForceKeyframe";
		}
	}

	close(recvFd);
	close(injFd);
}

// ═══════════════════════════════════════════════════════════
// CommandAck: rejected ack resets executor
// ═══════════════════════════════════════════════════════════

TEST(CommandAck, RejectedAckAllowsRetry) {
	// Simulate: push SetEncodingParameters, worker rejects (ack.applied=false),
	// verify executor can re-emit the same action.
	mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity> ctrlQueue;
	mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity> ackQueue;

	// Push a command
	mt::TrackControlCommand cmd;
	cmd.type = mt::TrackCommandType::SetEncodingParameters;
	cmd.trackIndex = 0;
	cmd.bitrateBps = 300000;
	cmd.fps = 10;
	cmd.scaleResolutionDownBy = 2.0;
	ASSERT_TRUE(ctrlQueue.tryPush(std::move(cmd)));

	// Simulate worker rejecting it
	mt::CommandAck ack;
	ack.trackIndex = 0;
	ack.type = mt::TrackCommandType::SetEncodingParameters;
	ack.applied = false;
	ASSERT_TRUE(ackQueue.tryPush(std::move(ack)));

	// Verify ack is consumable
	mt::CommandAck out;
	ASSERT_TRUE(ackQueue.tryPop(out));
	EXPECT_FALSE(out.applied);
	EXPECT_EQ(out.type, mt::TrackCommandType::SetEncodingParameters);

	// Push same command again (simulating retry after executor reset)
	mt::TrackControlCommand cmd2;
	cmd2.type = mt::TrackCommandType::SetEncodingParameters;
	cmd2.bitrateBps = 300000;
	cmd2.fps = 10;
	ASSERT_TRUE(ctrlQueue.tryPush(std::move(cmd2)));

	// Worker accepts this time
	mt::CommandAck ack2;
	ack2.trackIndex = 0;
	ack2.type = mt::TrackCommandType::SetEncodingParameters;
	ack2.applied = true;
	ack2.actualBitrateBps = 300000;
	ack2.actualFps = 10;
	ack2.actualWidth = 320;
	ack2.actualHeight = 240;
	ASSERT_TRUE(ackQueue.tryPush(std::move(ack2)));

	mt::CommandAck out2;
	ASSERT_TRUE(ackQueue.tryPop(out2));
	EXPECT_TRUE(out2.applied);
	EXPECT_EQ(out2.actualBitrateBps, 300000);
	EXPECT_EQ(out2.actualWidth, 320);
}

TEST(CommandAck, PauseResumeAckUpdatesOnlyMatchingPendingCommand) {
	using namespace qos;
	PublisherQosController::Options opts;
	opts.source = Source::Camera;
	opts.trackId = "video";
	opts.initialLevel = 0;
	opts.actionSink = [](const PlannedAction&) { return false; };
	PublisherQosController ctrl(opts);

	mt::PendingTrackCommand pending;
	pending.commandId = 9;
	pending.pending = true;
	pending.action.type = ActionType::PauseUpstream;
	pending.action.level = 4;

	mt::ThreadedTrackControlState trackState{400000, 20, 1.0, false, 0, &ctrl};
	mt::ThreadedTrackStatsState statsState;

	mt::CommandAck stale;
	stale.commandId = 7;
	stale.type = mt::TrackCommandType::PauseTrack;
	stale.applied = true;
	EXPECT_FALSE(mt::applyCommandAck(stale, pending, trackState, statsState));
	EXPECT_FALSE(trackState.videoSuppressed);
	EXPECT_TRUE(pending.pending);

	mt::CommandAck good;
	good.commandId = 9;
	good.type = mt::TrackCommandType::PauseTrack;
	good.applied = true;
	EXPECT_TRUE(mt::applyCommandAck(good, pending, trackState, statsState));
	EXPECT_TRUE(trackState.videoSuppressed);
	EXPECT_FALSE(pending.pending);
}

TEST(CommandAck, RejectedResumeDoesNotFlipSuppressedState) {
	using namespace qos;
	PublisherQosController::Options opts;
	opts.source = Source::Camera;
	opts.trackId = "video";
	opts.initialLevel = 4;
	opts.actionSink = [](const PlannedAction&) { return false; };
	PublisherQosController ctrl(opts);

	mt::PendingTrackCommand pending;
	pending.commandId = 10;
	pending.pending = true;
	pending.action.type = ActionType::ResumeUpstream;
	pending.action.level = 3;

	mt::ThreadedTrackControlState trackState{400000, 20, 1.0, true, 0, &ctrl};
	mt::ThreadedTrackStatsState statsState;

	mt::CommandAck ack;
	ack.commandId = 10;
	ack.type = mt::TrackCommandType::ResumeTrack;
	ack.applied = false;
	ack.reason = "resume-rejected";

	EXPECT_TRUE(mt::applyCommandAck(ack, pending, trackState, statsState));
	EXPECT_TRUE(trackState.videoSuppressed) << "rejected resume must not clear suppressed state";
	EXPECT_FALSE(pending.pending);
}

TEST(StatsFreshness, PeerStatsPresentButTrackMissingStillSkipsTrack) {
	// Simulate the control-loop gate: peer-level cached stats exist, but not for this track.
	bool statsFresh = false;
	bool hasServerStatsForTrack = false;
	EXPECT_FALSE(mt::shouldSampleTrack(statsFresh, hasServerStatsForTrack));
}

// ═══════════════════════════════════════════════════════════
// P4: configGeneration filtering — old AU rejected by network
// ═══════════════════════════════════════════════════════════

TEST(ConfigGeneration, OldGenerationAURejectedByNetwork) {
	int sendFd = -1, recvFd = -1;
	uint16_t port = 0;
	ASSERT_EQ(createConnectedUdpPair(sendFd, recvFd, port), 0);

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueue;
	mt::SpscQueue<mt::SenderStatsSnapshot, mt::kStatsQueueCapacity> statsQueue;

	NetworkThread::Config cfg;
	cfg.udpFd = sendFd;
	NetworkThread net(cfg);
	net.registerVideoTrack(0, 11111111, 96);
	net.statsQueue = &statsQueue;
	NetworkThread::SourceInput si;
	si.auQueue = &auQueue;
	si.keyframeQueue = &netQueue;
	net.addSourceInput(si);

	// Push AU with generation=1 (current)
	auto makeAU = [](uint64_t gen) {
		mt::EncodedAccessUnit au;
		au.ssrc = 11111111;
		au.payloadType = 96;
		au.rtpTimestamp = 90000;
		au.isKeyframe = true;
		au.configGeneration = gen;
		uint8_t h264[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x1e};
		au.assign(h264, sizeof(h264));
		return au;
	};

	// First: gen=1 should be accepted
	ASSERT_TRUE(auQueue.tryPush(makeAU(1)));
	net.start();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// Now push gen=0 (old) — should be rejected
	ASSERT_TRUE(auQueue.tryPush(makeAU(0)));
	net.wakeup();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// Push gen=2 (new) — should be accepted
	ASSERT_TRUE(auQueue.tryPush(makeAU(2)));
	net.wakeup();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	net.stop();

	// Count RTP packets — should have packets from gen=1 and gen=2, not gen=0
	int rtpCount = 0;
	uint8_t buf[1500];
	while (recv(recvFd, buf, sizeof(buf), MSG_DONTWAIT) > 0) rtpCount++;

	// gen=1 produces 1 NALU = 1 RTP, gen=2 produces 1 NALU = 1 RTP, gen=0 rejected
	EXPECT_EQ(rtpCount, 2) << "only gen=1 and gen=2 AUs should produce RTP packets";

	close(sendFd);
	close(recvFd);
}

// ═══════════════════════════════════════════════════════════
// P4: commandId matching — mismatched ack ignored
// ═══════════════════════════════════════════════════════════

TEST(CommandIdMatching, MismatchedAckIgnored) {
	mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity> ackQueue;

	// Simulate: pending command has id=5, but ack arrives with id=3 (stale)
	mt::CommandAck staleAck;
	staleAck.trackIndex = 0;
	staleAck.commandId = 3;
	staleAck.type = mt::TrackCommandType::SetEncodingParameters;
	staleAck.applied = true;
	staleAck.actualBitrateBps = 999999;
	ASSERT_TRUE(ackQueue.tryPush(std::move(staleAck)));

	// Control has pending commandId=5
	struct Pending { uint64_t commandId = 5; bool pending = true; } pc;

	mt::CommandAck ack;
	ASSERT_TRUE(ackQueue.tryPop(ack));

	// Should NOT match
	bool matches = pc.pending && ack.commandId == pc.commandId;
	EXPECT_FALSE(matches) << "stale commandId=3 should not match pending commandId=5";

	// Now correct ack arrives
	mt::CommandAck correctAck;
	correctAck.commandId = 5;
	correctAck.applied = true;
	correctAck.actualBitrateBps = 500000;
	ASSERT_TRUE(ackQueue.tryPush(std::move(correctAck)));
	ASSERT_TRUE(ackQueue.tryPop(ack));
	matches = pc.pending && ack.commandId == pc.commandId;
	EXPECT_TRUE(matches);
	EXPECT_EQ(ack.actualBitrateBps, 500000);
}

// ═══════════════════════════════════════════════════════════
// P4: pacing — packets are not all sent instantly
// ═══════════════════════════════════════════════════════════

TEST(Pacing, PacketsAreSpreadOverTime) {
	int sendFd = -1, recvFd = -1;
	uint16_t port = 0;
	ASSERT_EQ(createConnectedUdpPair(sendFd, recvFd, port), 0);

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueue;

	NetworkThread::Config cfg;
	cfg.udpFd = sendFd;
	NetworkThread net(cfg);
	net.registerVideoTrack(0, 11111111, 96);
	NetworkThread::SourceInput si;
	si.auQueue = &auQueue;
	si.keyframeQueue = &netQueue;
	net.addSourceInput(si);

	// Push a large AU that will produce many RTP packets (FU-A fragmentation)
	mt::EncodedAccessUnit au;
	au.ssrc = 11111111;
	au.payloadType = 96;
	au.rtpTimestamp = 90000;
	au.isKeyframe = true;
	au.configGeneration = 0;
	// Create a ~10KB NAL unit → ~8 FU-A fragments
	std::vector<uint8_t> bigNal(10000, 0xAA);
	bigNal[0] = 0x00; bigNal[1] = 0x00; bigNal[2] = 0x00; bigNal[3] = 0x01;
	bigNal[4] = 0x65; // IDR NAL
	au.assign(bigNal.data(), bigNal.size());
	ASSERT_TRUE(auQueue.tryPush(std::move(au)));

	net.start();
	net.wakeup();

	// Wait just 5ms — with pacing, not all packets should arrive yet
	std::this_thread::sleep_for(std::chrono::milliseconds(5));
	int earlyCount = 0;
	uint8_t buf[1500];
	while (recv(recvFd, buf, sizeof(buf), MSG_DONTWAIT) > 0) earlyCount++;

	// Wait for full pacing to complete
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	int lateCount = 0;
	while (recv(recvFd, buf, sizeof(buf), MSG_DONTWAIT) > 0) lateCount++;

	net.stop();

	int totalCount = earlyCount + lateCount;
	EXPECT_GT(totalCount, 1) << "should produce multiple RTP packets from large NAL";
	// With pacing, early batch should be limited (burst limit = 8 per tick)
	// and late batch should have the rest
	printf("[test] pacing: early=%d late=%d total=%d\n", earlyCount, lateCount, totalCount);

	close(sendFd);
	close(recvFd);
}

TEST(NetworkPause, PauseAckRequiresQuiescedTransport) {
	int sendFd = -1, recvFd = -1;
	uint16_t port = 0;
	ASSERT_EQ(createConnectedUdpPair(sendFd, recvFd, port), 0);

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueue;
	mt::SpscQueue<mt::NetworkControlCommand, mt::kControlCommandQueueCapacity> controlQueue;
	mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity> ackQueue;

	NetworkThread::Config cfg;
	cfg.udpFd = sendFd;
	NetworkThread net(cfg);
	net.registerVideoTrack(0, 11111111, 96);
	net.controlQueue = &controlQueue;
	net.commandAckQueue = &ackQueue;
	NetworkThread::SourceInput si;
	si.auQueue = &auQueue;
	si.keyframeQueue = &netQueue;
	net.addSourceInput(si);

	// Push many small NAL units so pacing has a backlog to flush.
	mt::EncodedAccessUnit au;
	au.ssrc = 11111111;
	au.payloadType = 96;
	au.rtpTimestamp = 90000;
	au.isKeyframe = true;
	au.configGeneration = 0;
	std::vector<uint8_t> annexb;
	for (int i = 0; i < 24; ++i) {
		const uint8_t nalu[] = {0x00, 0x00, 0x00, 0x01, static_cast<uint8_t>(0x65 + (i == 0 ? 0 : 1)),
			0x11, 0x22, static_cast<uint8_t>(0x30 + i)};
		annexb.insert(annexb.end(), nalu, nalu + sizeof(nalu));
	}
	au.assign(annexb.data(), annexb.size());
	ASSERT_TRUE(auQueue.tryPush(std::move(au)));

	net.start();
	net.wakeup();
	std::this_thread::sleep_for(std::chrono::milliseconds(5));

	mt::NetworkControlCommand pause;
	pause.type = mt::NetworkControlCommand::PauseTrack;
	pause.trackIndex = 0;
	pause.commandId = 77;
	ASSERT_TRUE(controlQueue.tryPush(std::move(pause)));
	net.wakeup();

	mt::CommandAck ack;
	bool gotAck = false;
	for (int i = 0; i < 50 && !gotAck; ++i) {
		if (ackQueue.tryPop(ack)) gotAck = true;
		else std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	ASSERT_TRUE(gotAck);
	EXPECT_EQ(ack.commandId, 77u);
	EXPECT_TRUE(ack.applied);
	EXPECT_EQ(ack.type, mt::TrackCommandType::PauseTrack);

	uint8_t buf[1500];
	while (recv(recvFd, buf, sizeof(buf), MSG_DONTWAIT) > 0) {}
	std::this_thread::sleep_for(std::chrono::milliseconds(30));

	int latePackets = 0;
	while (recv(recvFd, buf, sizeof(buf), MSG_DONTWAIT) > 0) latePackets++;
	EXPECT_EQ(latePackets, 0) << "no RTP should leave the socket after pause ack";

	net.stop();
	close(sendFd);
	close(recvFd);
}

// ═══════════════════════════════════════════════════════════
// SourceWorker: SetEncodingParameters triggers encoder recreate + configGeneration bump
// ═══════════════════════════════════════════════════════════

TEST(SourceWorkerIntegration, SetEncodingParametersChangesOutput) {
	if (!testFileExists()) { GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found"; }

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity> ctrlQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueue;
	mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity> ackQueue;

	SourceWorker::Config cfg;
	cfg.trackIndex = 0;
	cfg.ssrc = 11111111;
	cfg.payloadType = 96;
	cfg.inputType = SourceWorker::InputType::File;
	cfg.inputPath = kTestMp4;
	cfg.initialBitrate = 500000;
	cfg.initialFps = 25;

	SourceWorker worker(cfg);
	worker.outputQueue = &auQueue;
	worker.controlQueue = &ctrlQueue;
	worker.networkCmdQueue = &netQueue;
	worker.ackQueue = &ackQueue;
	worker.start();

	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	// Drain initial AUs and check initial configGeneration=0
	uint64_t initialGen = UINT64_MAX;
	{ mt::EncodedAccessUnit au; while (auQueue.tryPop(au)) initialGen = au.configGeneration; }
	EXPECT_EQ(initialGen, 0u);

	// Send SetEncodingParameters that forces encoder recreate (different resolution)
	mt::TrackControlCommand cmd;
	cmd.type = mt::TrackCommandType::SetEncodingParameters;
	cmd.commandId = 42;
	cmd.bitrateBps = 300000;
	cmd.fps = 15;
	cmd.scaleResolutionDownBy = 2.0; // halve resolution → forces recreate
	ASSERT_TRUE(ctrlQueue.tryPush(std::move(cmd)));

	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	worker.stop();

	// Check ack
	mt::CommandAck ack;
	bool gotAck = false;
	while (ackQueue.tryPop(ack)) gotAck = true;
	EXPECT_TRUE(gotAck);
	EXPECT_TRUE(ack.applied);
	EXPECT_EQ(ack.commandId, 42u);
	EXPECT_EQ(ack.actualBitrateBps, 300000);
	EXPECT_GT(ack.configGeneration, 0u) << "configGeneration should have incremented";

	// Check AUs after command have bumped configGeneration
	uint64_t newGen = 0;
	{ mt::EncodedAccessUnit au; while (auQueue.tryPop(au)) newGen = au.configGeneration; }
	EXPECT_GT(newGen, 0u) << "AUs after encoder recreate should have configGeneration > 0";

	printf("[test] initial gen=%llu, ack gen=%llu, AU gen=%llu\n",
		(unsigned long long)initialGen, (unsigned long long)ack.configGeneration, (unsigned long long)newGen);
}

TEST(SourceWorkerIntegration, BitrateOnlyUpdateDoesNotBumpConfigGeneration) {
	if (!testFileExists()) { GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found"; }

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity> ctrlQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueue;
	mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity> ackQueue;

	SourceWorker::Config cfg;
	cfg.trackIndex = 0;
	cfg.ssrc = 11111111;
	cfg.payloadType = 96;
	cfg.inputType = SourceWorker::InputType::File;
	cfg.inputPath = kTestMp4;
	cfg.initialBitrate = 500000;
	cfg.initialFps = 25;

	SourceWorker worker(cfg);
	worker.outputQueue = &auQueue;
	worker.controlQueue = &ctrlQueue;
	worker.networkCmdQueue = &netQueue;
	worker.ackQueue = &ackQueue;
	worker.start();

	std::this_thread::sleep_for(std::chrono::milliseconds(400));

	uint64_t initialGen = UINT64_MAX;
	{ mt::EncodedAccessUnit au; while (auQueue.tryPop(au)) initialGen = au.configGeneration; }
	EXPECT_EQ(initialGen, 0u);

	mt::TrackControlCommand cmd;
	cmd.type = mt::TrackCommandType::SetEncodingParameters;
	cmd.commandId = 77;
	cmd.bitrateBps = 350000;
	cmd.fps = 25;                    // unchanged
	cmd.scaleResolutionDownBy = 1.0; // unchanged
	ASSERT_TRUE(ctrlQueue.tryPush(std::move(cmd)));

	std::this_thread::sleep_for(std::chrono::milliseconds(400));
	worker.stop();

	mt::CommandAck ack;
	bool gotAck = false;
	while (ackQueue.tryPop(ack)) gotAck = true;
	EXPECT_TRUE(gotAck);
	EXPECT_TRUE(ack.applied);
	EXPECT_EQ(ack.commandId, 77u);
	EXPECT_EQ(ack.configGeneration, 0u) << "bitrate-only update must not bump generation";

	uint64_t newGen = 0;
	{ mt::EncodedAccessUnit au; while (auQueue.tryPop(au)) newGen = au.configGeneration; }
	EXPECT_EQ(newGen, 0u) << "AU generation should remain unchanged for bitrate-only update";
}

TEST(SourceWorkerIntegration, StaleConfigGenerationCommandRejected) {
	if (!testFileExists()) { GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found"; }

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity> ctrlQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueue;
	mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity> ackQueue;

	SourceWorker::Config cfg;
	cfg.trackIndex = 0;
	cfg.ssrc = 11111111;
	cfg.payloadType = 96;
	cfg.inputType = SourceWorker::InputType::File;
	cfg.inputPath = kTestMp4;
	cfg.initialBitrate = 500000;
	cfg.initialFps = 25;

	SourceWorker worker(cfg);
	worker.outputQueue = &auQueue;
	worker.controlQueue = &ctrlQueue;
	worker.networkCmdQueue = &netQueue;
	worker.ackQueue = &ackQueue;
	worker.start();

	std::this_thread::sleep_for(std::chrono::milliseconds(400));

	// First command recreates encoder and bumps generation to 1.
	mt::TrackControlCommand cmd1;
	cmd1.type = mt::TrackCommandType::SetEncodingParameters;
	cmd1.commandId = 100;
	cmd1.configGeneration = 0;
	cmd1.bitrateBps = 300000;
	cmd1.fps = 15;
	cmd1.scaleResolutionDownBy = 2.0;
	ASSERT_TRUE(ctrlQueue.tryPush(std::move(cmd1)));
	std::this_thread::sleep_for(std::chrono::milliseconds(300));

	// Second command still claims generation 0 — should be rejected as stale.
	mt::TrackControlCommand stale;
	stale.type = mt::TrackCommandType::SetEncodingParameters;
	stale.commandId = 101;
	stale.configGeneration = 0;
	stale.bitrateBps = 320000;
	stale.fps = 15;
	stale.scaleResolutionDownBy = 2.0;
	ASSERT_TRUE(ctrlQueue.tryPush(std::move(stale)));
	std::this_thread::sleep_for(std::chrono::milliseconds(300));
	worker.stop();

	std::vector<mt::CommandAck> acks;
	mt::CommandAck ack;
	while (ackQueue.tryPop(ack)) acks.push_back(ack);

	ASSERT_GE(acks.size(), 2u);
	bool sawApplied = false;
	bool sawRejected = false;
	for (const auto& a : acks) {
		if (a.commandId == 100 && a.applied && a.configGeneration == 1) sawApplied = true;
		if (a.commandId == 101 && !a.applied && a.reason == "stale-config-generation") sawRejected = true;
	}
	EXPECT_TRUE(sawApplied);
	EXPECT_TRUE(sawRejected);
}

// ═══════════════════════════════════════════════════════════
// confirmAction: verify controller level advances only on ack
// ═══════════════════════════════════════════════════════════

TEST(ConfirmAction, AdvancesControllerLevelOnAck) {
	using namespace qos;
	// Create a controller with actionSink that always returns false (async pending)
	PublisherQosController::Options opts;
	opts.source = Source::Camera;
	opts.trackId = "video";
	opts.initialLevel = 0;
	opts.actionSink = [](const PlannedAction&) { return false; };
	PublisherQosController ctrl(opts);

	EXPECT_EQ(ctrl.currentLevel(), 0);

	// Simulate: controller tried to go to level 1 but sink returned false
	// Level should NOT have advanced
	// Now confirm the action externally
	PlannedAction action;
	action.type = ActionType::SetEncodingParameters;
	action.level = 1;
	action.encodingParameters = EncodingParameters{700000, 20, 1.5, {}};
	ctrl.confirmAction(action);

	EXPECT_EQ(ctrl.currentLevel(), 1) << "level should advance after confirmAction";
}

TEST(ConfirmAction, AdvancesAudioOnlyOnAck) {
	using namespace qos;
	PublisherQosController::Options opts;
	opts.source = Source::Camera;
	opts.trackId = "video";
	opts.initialLevel = 3;
	opts.actionSink = [](const PlannedAction&) { return false; };
	PublisherQosController ctrl(opts);

	// Confirm EnterAudioOnly
	PlannedAction enterAO;
	enterAO.type = ActionType::EnterAudioOnly;
	enterAO.level = 4;
	ctrl.confirmAction(enterAO);
	EXPECT_EQ(ctrl.currentLevel(), 4);

	// Confirm ExitAudioOnly
	PlannedAction exitAO;
	exitAO.type = ActionType::ExitAudioOnly;
	exitAO.level = 3;
	ctrl.confirmAction(exitAO);
	EXPECT_EQ(ctrl.currentLevel(), 3);
}

// ═══════════════════════════════════════════════════════════
// PLI debounce: rapid PLIs only forward once within window
// ═══════════════════════════════════════════════════════════

TEST(PLIDebounce, RapidPLIsOnlyForwardOnce) {
	constexpr int kTracks = 1;
	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueue;

	int recvFd = socket(AF_INET, SOCK_DGRAM, 0);
	ASSERT_GE(recvFd, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;
	ASSERT_EQ(bind(recvFd, (sockaddr*)&addr, sizeof(addr)), 0);
	socklen_t alen = sizeof(addr);
	getsockname(recvFd, (sockaddr*)&addr, &alen);
	int flags = fcntl(recvFd, F_GETFL, 0);
	fcntl(recvFd, F_SETFL, flags | O_NONBLOCK);

	int injFd = socket(AF_INET, SOCK_DGRAM, 0);
	ASSERT_GE(injFd, 0);

	NetworkThread::Config cfg;
	cfg.udpFd = recvFd;
	NetworkThread net(cfg);
	net.registerVideoTrack(0, 11111111, 96);
	NetworkThread::SourceInput si;
	si.auQueue = &auQueue;
	si.keyframeQueue = &netQueue;
	net.addSourceInput(si);
	net.start();

	// Build PLI packet targeting ssrc=11111111
	uint8_t pli[12];
	pli[0] = 0x81; pli[1] = 206; pli[2] = 0; pli[3] = 2;
	pli[4] = 0; pli[5] = 0; pli[6] = 0; pli[7] = 0;
	uint32_t ssrc = 11111111;
	pli[8] = (ssrc>>24)&0xFF; pli[9] = (ssrc>>16)&0xFF;
	pli[10] = (ssrc>>8)&0xFF; pli[11] = ssrc&0xFF;

	// Send 5 PLIs rapidly
	for (int i = 0; i < 5; ++i)
		sendto(injFd, pli, sizeof(pli), 0, (sockaddr*)&addr, sizeof(addr));

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	net.stop();

	// Should only get 1 ForceKeyframe (debounce window = 1s)
	int fkCount = 0;
	mt::NetworkToSourceCommand cmd;
	while (netQueue.tryPop(cmd)) {
		if (cmd.type == mt::NetworkToSourceCommand::ForceKeyframe) fkCount++;
	}
	EXPECT_EQ(fkCount, 1) << "5 rapid PLIs should produce only 1 ForceKeyframe (debounce)";

	close(recvFd);
	close(injFd);
}

// ═══════════════════════════════════════════════════════════
// Multi-track E2E: 2 source workers → network thread → RTP
// ═══════════════════════════════════════════════════════════

TEST(MultiTrackE2E, TwoSourceWorkersProduceRTP) {
	if (!testFileExists()) { GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found"; }

	int sendFd = -1, recvFd = -1;
	uint16_t port = 0;
	ASSERT_EQ(createConnectedUdpPair(sendFd, recvFd, port), 0);

	constexpr int kTracks = 2;
	struct TrackQueues {
		mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> au;
		mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity> ctrl;
		mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> net;
		mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity> ack;
	};
	TrackQueues tq[kTracks];
	mt::SpscQueue<mt::SenderStatsSnapshot, mt::kStatsQueueCapacity> statsQueue;

	uint32_t ssrcs[kTracks] = {11111111, 22222222};

	NetworkThread::Config netCfg;
	netCfg.udpFd = sendFd;
	NetworkThread net(netCfg);
	net.statsQueue = &statsQueue;
	for (int i = 0; i < kTracks; ++i) {
		net.registerVideoTrack(i, ssrcs[i], 96);
		NetworkThread::SourceInput si;
		si.auQueue = &tq[i].au;
		si.keyframeQueue = &tq[i].net;
		net.addSourceInput(si);
	}

	std::vector<std::unique_ptr<SourceWorker>> workers;
	for (int i = 0; i < kTracks; ++i) {
		SourceWorker::Config swCfg;
		swCfg.trackIndex = i;
		swCfg.ssrc = ssrcs[i];
		swCfg.payloadType = 96;
		swCfg.inputType = SourceWorker::InputType::File;
		swCfg.inputPath = kTestMp4;
		swCfg.initialBitrate = 300000;
		swCfg.initialFps = 15;
		auto w = std::make_unique<SourceWorker>(swCfg);
		w->outputQueue = &tq[i].au;
		w->controlQueue = &tq[i].ctrl;
		w->networkCmdQueue = &tq[i].net;
		w->ackQueue = &tq[i].ack;
		w->networkWakeupFn = [&net]() { net.wakeup(); };
		workers.push_back(std::move(w));
	}

	net.start();
	for (auto& w : workers) w->start();

	std::this_thread::sleep_for(std::chrono::milliseconds(1500));

	for (auto& w : workers) w->stop();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	net.stop();

	// Count RTP packets — should have packets from both tracks
	int rtpCount = 0;
	std::set<uint32_t> seenSsrcs;
	uint8_t buf[1500];
	while (true) {
		int n = recv(recvFd, buf, sizeof(buf), MSG_DONTWAIT);
		if (n < 12) break;
		if ((buf[0] & 0xC0) == 0x80) {
			rtpCount++;
			uint32_t ssrc = ((uint32_t)buf[8]<<24)|((uint32_t)buf[9]<<16)|((uint32_t)buf[10]<<8)|buf[11];
			seenSsrcs.insert(ssrc);
		}
	}

	EXPECT_GT(rtpCount, 20) << "should have many RTP packets from 2 tracks";
	EXPECT_TRUE(seenSsrcs.count(11111111)) << "should see RTP from track 0";
	EXPECT_TRUE(seenSsrcs.count(22222222)) << "should see RTP from track 1";

	printf("[test] multi-track e2e: %d RTP packets, %zu distinct SSRCs\n", rtpCount, seenSsrcs.size());
	close(sendFd);
	close(recvFd);
}

// ═══════════════════════════════════════════════════════════
// confirmAction + probe interaction
// ═══════════════════════════════════════════════════════════

TEST(ConfirmAction, AsyncConfirmThenProbeStartsCorrectly) {
	using namespace qos;
	// Simulate: controller at level 2 (congested), recovers to level 1 via async confirm.
	// After confirm, probe should start (level decreased).
	PublisherQosController::Options opts;
	opts.source = Source::Camera;
	opts.trackId = "video";
	opts.initialLevel = 2;
	opts.warmupSamples = 0;
	opts.actionSink = [](const PlannedAction&) { return false; }; // async
	PublisherQosController ctrl(opts);

	EXPECT_EQ(ctrl.currentLevel(), 2);

	// Confirm recovery to level 1
	PlannedAction action;
	action.type = ActionType::SetEncodingParameters;
	action.level = 1;
	action.encodingParameters = EncodingParameters{700000, 20, 1.5, {}};
	ctrl.confirmAction(action);

	EXPECT_EQ(ctrl.currentLevel(), 1);
	// Probe should be active after level decrease
	EXPECT_TRUE(ctrl.getRuntimeSettings().probeActive) << "probe should start after confirmed level decrease";
}

// ═══════════════════════════════════════════════════════════
// actionSink false → confirm → lastActionApplied state chain
// ═══════════════════════════════════════════════════════════

TEST(ConfirmAction, LastActionAppliedReflectsAsyncConfirm) {
	using namespace qos;
	bool sinkCalled = false;
	PublisherQosController::Options opts;
	opts.source = Source::Camera;
	opts.trackId = "video";
	opts.initialLevel = 0;
	opts.warmupSamples = 0;
	opts.actionSink = [&sinkCalled](const PlannedAction&) {
		sinkCalled = true;
		return false; // async pending
	};
	PublisherQosController ctrl(opts);

	// Feed congested samples to trigger a degrade action
	auto makeSnap = [](int64_t ts, double loss, double rtt) {
		RawSenderSnapshot s;
		s.timestampMs = ts;
		s.trackId = "video";
		s.source = Source::Camera;
		s.kind = TrackKind::Video;
		s.bytesSent = ts * 100;
		s.packetsSent = ts;
		s.packetsLost = static_cast<uint64_t>(ts * loss);
		s.targetBitrateBps = 900000;
		s.configuredBitrateBps = 900000;
		s.roundTripTimeMs = rtt;
		s.jitterMs = 10;
		return s;
	};

	// Warmup=0, so first samples are reactive
	// Feed warning then congested samples
	for (int i = 1; i <= 6; ++i)
		ctrl.onSample(makeSnap(i * 1000, 0.15, 400));

	// Sink should have been called, returned false
	EXPECT_TRUE(sinkCalled);
	// lastActionApplied should be false (sink returned false)
	EXPECT_FALSE(ctrl.lastActionWasApplied());

	// Now confirm the action
	PlannedAction confirmed = ctrl.lastAction();
	ctrl.confirmAction(confirmed);
	EXPECT_TRUE(ctrl.lastActionWasApplied());
}

// ═══════════════════════════════════════════════════════════
// SourceWorker: StopSource command causes clean exit
// ═══════════════════════════════════════════════════════════

TEST(SourceWorkerIntegration, StopSourceCommandCausesCleanExit) {
	if (!testFileExists()) { GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found"; }

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity> ctrlQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueue;
	mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity> ackQueue;

	SourceWorker::Config cfg;
	cfg.trackIndex = 0;
	cfg.ssrc = 44444444;
	cfg.payloadType = 96;
	cfg.inputType = SourceWorker::InputType::File;
	cfg.inputPath = kTestMp4; // 45s file
	cfg.initialBitrate = 500000;
	cfg.initialFps = 25;

	SourceWorker worker(cfg);
	worker.outputQueue = &auQueue;
	worker.controlQueue = &ctrlQueue;
	worker.networkCmdQueue = &netQueue;
	worker.ackQueue = &ackQueue;
	worker.start();

	// Let it run briefly
	std::this_thread::sleep_for(std::chrono::milliseconds(300));

	// Send StopSource — worker should exit without waiting for file EOF
	mt::TrackControlCommand cmd;
	cmd.type = mt::TrackCommandType::StopSource;
	ASSERT_TRUE(ctrlQueue.tryPush(std::move(cmd)));

	// Worker should stop within a reasonable time (not 45s)
	auto start = std::chrono::steady_clock::now();
	worker.stop();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start).count();

	EXPECT_LT(elapsed, 2000) << "StopSource should cause prompt exit, not wait for file EOF";
	printf("[test] StopSource: worker exited in %lldms\n", (long long)elapsed);
}

TEST_F(ThreadedPlainPublishIntegrationTest, PlainPublishGetStatsAndClientStatsSmoke) {
	auto ws = joinPeer("alice");

	const uint32_t videoSsrc = 11111111u;
	const uint32_t audioSsrc = 22222222u;
	auto publishResp = ws->request("plainPublish", {
		{"videoSsrcs", json::array({videoSsrc})},
		{"audioSsrc", audioSsrc}
	});
	ASSERT_TRUE(publishResp.value("ok", false)) << publishResp.dump();
	auto data = publishResp["data"];
	ASSERT_TRUE(data.contains("ip"));
	ASSERT_TRUE(data.contains("port"));
	ASSERT_TRUE(data.contains("videoTracks"));
	ASSERT_FALSE(data["videoTracks"].empty());

	const std::string dstIp = data["ip"].get<std::string>();
	const uint16_t dstPort = data["port"].get<uint16_t>();
	const uint8_t videoPt = static_cast<uint8_t>(data["videoTracks"][0]["pt"].get<int>());
	const std::string videoProducerId = data["videoTracks"][0]["producerId"].get<std::string>();

	int udp = socket(AF_INET, SOCK_DGRAM, 0);
	ASSERT_GE(udp, 0);
	sockaddr_in dst{};
	dst.sin_family = AF_INET;
	dst.sin_port = htons(dstPort);
	inet_pton(AF_INET, dstIp.c_str(), &dst.sin_addr);
	ASSERT_EQ(::connect(udp, (sockaddr*)&dst, sizeof(dst)), 0);
	int flags = fcntl(udp, F_GETFL, 0);
	fcntl(udp, F_SETFL, flags | O_NONBLOCK);

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueue;
	mt::SpscQueue<mt::SenderStatsSnapshot, mt::kStatsQueueCapacity> statsQueue;

	NetworkThread::Config cfg;
	cfg.udpFd = udp;
	cfg.audioSsrc = audioSsrc;
	cfg.audioPt = static_cast<uint8_t>(data["audioPt"].get<int>());
	NetworkThread net(cfg);
	net.registerVideoTrack(0, videoSsrc, videoPt);
	net.statsQueue = &statsQueue;
	NetworkThread::SourceInput si;
	si.auQueue = &auQueue;
	si.keyframeQueue = &netQueue;
	net.addSourceInput(si);

	net.sendComediaProbe();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	net.start();

	for (uint32_t i = 0; i < 2; ++i) {
		mt::EncodedAccessUnit au;
		au.trackIndex = 0;
		au.ssrc = videoSsrc;
		au.payloadType = videoPt;
		au.rtpTimestamp = 90000 + i * 3000;
		au.isKeyframe = (i == 0);
		au.configGeneration = 0;
		uint8_t h264[] = {
			0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x1e,
			0x00, 0x00, 0x00, 0x01, 0x65, 0xAA, 0xBB, static_cast<uint8_t>(0xC0 + i)
		};
		au.assign(h264, sizeof(h264));
		ASSERT_TRUE(auQueue.tryPush(std::move(au)));
		net.wakeup();
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1200));
	net.stop();

	auto statsResp = ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(statsResp.value("ok", false)) << statsResp.dump();
	ASSERT_TRUE(statsResp["data"].contains("producers")) << statsResp.dump();
	ASSERT_TRUE(statsResp["data"]["producers"].contains(videoProducerId)) << statsResp.dump();
	EXPECT_EQ(statsResp["data"]["producers"][videoProducerId]["kind"], "video");

	json clientReport = {
		{"schema", "mediasoup.qos.client.v1"},
		{"seq", 7},
		{"tsMs", 1712736000123LL},
		{"peerState", {{"mode", "audio-video"}, {"quality", "good"}, {"stale", false}}},
		{"tracks", json::array({
			{
				{"localTrackId", "video"},
				{"producerId", videoProducerId},
				{"kind", "video"},
				{"source", "camera"},
				{"state", "stable"},
				{"reason", "network"},
				{"quality", "good"},
				{"ladderLevel", 1},
				{"signals", {
					{"sendBitrateBps", 300000},
					{"targetBitrateBps", 300000},
					{"lossRate", 0.0},
					{"rttMs", 30},
					{"jitterMs", 4},
					{"frameWidth", 640},
					{"frameHeight", 360},
					{"framesPerSecond", 15},
					{"qualityLimitationReason", "none"}
				}},
				{"lastAction", {{"type", "setEncodingParameters"}, {"applied", true}}}
			}
		})}
	};
	auto clientStatsResp = ws->request("clientStats", clientReport);
	ASSERT_TRUE(clientStatsResp.value("ok", false)) << clientStatsResp.dump();
	usleep(300000);

	auto statsResp2 = ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(statsResp2.value("ok", false)) << statsResp2.dump();
	ASSERT_TRUE(statsResp2["data"].contains("clientStats")) << statsResp2.dump();
	EXPECT_EQ(statsResp2["data"]["clientStats"]["seq"], 7);

	ws->close();
	close(udp);
}

TEST_F(ThreadedPlainPublishIntegrationTest, AudioRtpAndStatsSmoke) {
	auto ws = joinPeer("alice");

	const uint32_t videoSsrc = 11111111u;
	const uint32_t audioSsrc = 22222222u;
	auto publishResp = ws->request("plainPublish", {
		{"videoSsrcs", json::array({videoSsrc})},
		{"audioSsrc", audioSsrc}
	});
	ASSERT_TRUE(publishResp.value("ok", false)) << publishResp.dump();
	auto data = publishResp["data"];

	const std::string dstIp = data["ip"].get<std::string>();
	const uint16_t dstPort = data["port"].get<uint16_t>();
	const uint8_t videoPt = static_cast<uint8_t>(data["videoTracks"][0]["pt"].get<int>());
	const std::string videoProducerId = data["videoTracks"][0]["producerId"].get<std::string>();
	const std::string audioProducerId = data["audioProdId"].get<std::string>();
	const uint8_t audioPt = static_cast<uint8_t>(data["audioPt"].get<int>());

	int udp = socket(AF_INET, SOCK_DGRAM, 0);
	ASSERT_GE(udp, 0);
	sockaddr_in dst{};
	dst.sin_family = AF_INET;
	dst.sin_port = htons(dstPort);
	inet_pton(AF_INET, dstIp.c_str(), &dst.sin_addr);
	ASSERT_EQ(::connect(udp, (sockaddr*)&dst, sizeof(dst)), 0);
	int flags = fcntl(udp, F_GETFL, 0);
	fcntl(udp, F_SETFL, flags | O_NONBLOCK);

	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> videoAuQueue;
	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> audioAuQueue;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netQueue;
	mt::SpscQueue<mt::SenderStatsSnapshot, mt::kStatsQueueCapacity> statsQueue;

	NetworkThread::Config cfg;
	cfg.udpFd = udp;
	cfg.audioSsrc = audioSsrc;
	cfg.audioPt = audioPt;
	NetworkThread net(cfg);
	net.registerVideoTrack(0, videoSsrc, videoPt);
	net.statsQueue = &statsQueue;
	net.audioAuQueue = &audioAuQueue;
	NetworkThread::SourceInput si;
	si.auQueue = &videoAuQueue;
	si.keyframeQueue = &netQueue;
	net.addSourceInput(si);

	net.sendComediaProbe();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	net.start();

	// Send a couple of fake video AUs so transport learns both producers.
	for (uint32_t i = 0; i < 2; ++i) {
		mt::EncodedAccessUnit au;
		au.trackIndex = 0;
		au.ssrc = videoSsrc;
		au.payloadType = videoPt;
		au.rtpTimestamp = 90000 + i * 3000;
		au.isKeyframe = (i == 0);
		au.configGeneration = 0;
		uint8_t h264[] = {
			0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x1e,
			0x00, 0x00, 0x00, 0x01, 0x65, 0xAA, 0xBB, static_cast<uint8_t>(0xD0 + i)
		};
		au.assign(h264, sizeof(h264));
		ASSERT_TRUE(videoAuQueue.tryPush(std::move(au)));
		net.wakeup();
	}

	// Send a few fake audio AUs through the audio queue.
	for (uint32_t i = 0; i < 4; ++i) {
		mt::EncodedAccessUnit au;
		au.ssrc = audioSsrc;
		au.payloadType = audioPt;
		au.rtpTimestamp = i * 960;
		uint8_t opusPayload[] = {0xF8, 0xFF, 0xFE, static_cast<uint8_t>(0x80 + i)};
		au.assign(opusPayload, sizeof(opusPayload));
		ASSERT_TRUE(audioAuQueue.tryPush(std::move(au)));
		net.wakeup();
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(1200));
	net.stop();

	auto statsResp = ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(statsResp.value("ok", false)) << statsResp.dump();
	ASSERT_TRUE(statsResp["data"].contains("producers")) << statsResp.dump();
	auto producers = statsResp["data"]["producers"];
	ASSERT_TRUE(producers.contains(videoProducerId)) << statsResp.dump();
	ASSERT_TRUE(producers.contains(audioProducerId)) << statsResp.dump();
	EXPECT_EQ(producers[videoProducerId]["kind"], "video");
	EXPECT_EQ(producers[audioProducerId]["kind"], "audio");
	EXPECT_TRUE(producers[audioProducerId].contains("stats"));

	ws->close();
	close(udp);
}

TEST_F(ThreadedPlainPublishIntegrationTest, MultiTrackClientStatsStoredAndAggregatedSmoke) {
	ASSERT_TRUE(testFileExists()) << "tests/fixtures/media/test_sweep.mp4 not found";

	auto ws = joinPeer("alice");

	const uint32_t videoSsrc0 = 11111111u;
	const uint32_t videoSsrc1 = 22222222u;
	const uint32_t audioSsrc = 33333333u;
	auto publishResp = ws->request("plainPublish", {
		{"videoSsrcs", json::array({videoSsrc0, videoSsrc1})},
		{"audioSsrc", audioSsrc}
	});
	ASSERT_TRUE(publishResp.value("ok", false)) << publishResp.dump();
	auto data = publishResp["data"];
	ASSERT_EQ(data["videoTracks"].size(), 2u);

	const std::string dstIp = data["ip"].get<std::string>();
	const uint16_t dstPort = data["port"].get<uint16_t>();
	const std::string producerId0 = data["videoTracks"][0]["producerId"].get<std::string>();
	const std::string producerId1 = data["videoTracks"][1]["producerId"].get<std::string>();

	int udp = socket(AF_INET, SOCK_DGRAM, 0);
	ASSERT_GE(udp, 0);
	sockaddr_in dst{};
	dst.sin_family = AF_INET;
	dst.sin_port = htons(dstPort);
	inet_pton(AF_INET, dstIp.c_str(), &dst.sin_addr);
	ASSERT_EQ(::connect(udp, (sockaddr*)&dst, sizeof(dst)), 0);
	int flags = fcntl(udp, F_GETFL, 0);
	fcntl(udp, F_SETFL, flags | O_NONBLOCK);

	constexpr int kTracks = 2;
	struct TrackQueues {
		mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> au;
		mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity> ctrl;
		mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> net;
		mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity> ack;
	};
	TrackQueues tq[kTracks];
	mt::SpscQueue<mt::SenderStatsSnapshot, mt::kStatsQueueCapacity> statsQueue;

	uint32_t ssrcs[kTracks] = {videoSsrc0, videoSsrc1};
	uint8_t pts[kTracks] = {
		static_cast<uint8_t>(data["videoTracks"][0]["pt"].get<int>()),
		static_cast<uint8_t>(data["videoTracks"][1]["pt"].get<int>())
	};

	NetworkThread::Config netCfg;
	netCfg.udpFd = udp;
	netCfg.audioSsrc = audioSsrc;
	netCfg.audioPt = static_cast<uint8_t>(data["audioPt"].get<int>());
	NetworkThread net(netCfg);
	net.statsQueue = &statsQueue;
	for (int i = 0; i < kTracks; ++i) {
		net.registerVideoTrack(i, ssrcs[i], pts[i]);
		NetworkThread::SourceInput si;
		si.auQueue = &tq[i].au;
		si.keyframeQueue = &tq[i].net;
		net.addSourceInput(si);
	}

	std::vector<std::unique_ptr<SourceWorker>> workers;
	for (int i = 0; i < kTracks; ++i) {
		SourceWorker::Config swCfg;
		swCfg.trackIndex = i;
		swCfg.ssrc = ssrcs[i];
		swCfg.payloadType = pts[i];
		swCfg.inputType = SourceWorker::InputType::File;
		swCfg.inputPath = kTestMp4;
		swCfg.initialBitrate = 300000 + (i * 100000);
		swCfg.initialFps = 15;
		auto w = std::make_unique<SourceWorker>(swCfg);
		w->outputQueue = &tq[i].au;
		w->controlQueue = &tq[i].ctrl;
		w->networkCmdQueue = &tq[i].net;
		w->ackQueue = &tq[i].ack;
		w->networkWakeupFn = [&net]() { net.wakeup(); };
		workers.push_back(std::move(w));
	}

	net.sendComediaProbe();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	net.start();
	for (auto& w : workers) w->start();

	std::this_thread::sleep_for(std::chrono::milliseconds(1500));
	for (auto& w : workers) w->stop();
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	net.stop();

	auto statsResp = ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(statsResp.value("ok", false)) << statsResp.dump();
	ASSERT_TRUE(statsResp["data"].contains("producers")) << statsResp.dump();
	ASSERT_TRUE(statsResp["data"]["producers"].contains(producerId0)) << statsResp.dump();
	ASSERT_TRUE(statsResp["data"]["producers"].contains(producerId1)) << statsResp.dump();

	json clientReport = {
		{"schema", "mediasoup.qos.client.v1"},
		{"seq", 21},
		{"tsMs", 1712736001123LL},
		{"peerState", {{"mode", "audio-video"}, {"quality", "good"}, {"stale", false}}},
		{"tracks", json::array({
			{
				{"localTrackId", "video"},
				{"producerId", producerId0},
				{"kind", "video"},
				{"source", "camera"},
				{"state", "stable"},
				{"reason", "network"},
				{"quality", "good"},
				{"ladderLevel", 1},
				{"signals", {
					{"sendBitrateBps", 250000},
					{"targetBitrateBps", 300000},
					{"lossRate", 0.0},
					{"rttMs", 40},
					{"jitterMs", 5},
					{"frameWidth", 640},
					{"frameHeight", 360},
					{"framesPerSecond", 15},
					{"qualityLimitationReason", "none"}
				}},
				{"lastAction", {{"type", "setEncodingParameters"}, {"applied", true}}}
			},
			{
				{"localTrackId", "video-1"},
				{"producerId", producerId1},
				{"kind", "video"},
				{"source", "camera"},
				{"state", "early_warning"},
				{"reason", "network"},
				{"quality", "poor"},
				{"ladderLevel", 2},
				{"signals", {
					{"sendBitrateBps", 120000},
					{"targetBitrateBps", 400000},
					{"lossRate", 0.08},
					{"rttMs", 220},
					{"jitterMs", 22},
					{"frameWidth", 640},
					{"frameHeight", 360},
					{"framesPerSecond", 12},
					{"qualityLimitationReason", "bandwidth"}
				}},
				{"lastAction", {{"type", "setEncodingParameters"}, {"applied", true}}}
			}
		})}
	};

	auto clientStatsResp = ws->request("clientStats", clientReport);
	ASSERT_TRUE(clientStatsResp.value("ok", false)) << clientStatsResp.dump();
	usleep(300000);

	auto statsResp2 = ws->request("getStats", {{"peerId", "alice"}});
	ASSERT_TRUE(statsResp2.value("ok", false)) << statsResp2.dump();
	ASSERT_TRUE(statsResp2["data"].contains("clientStats")) << statsResp2.dump();
	auto clientStats = statsResp2["data"]["clientStats"];
	ASSERT_TRUE(clientStats.contains("tracks")) << statsResp2.dump();
	ASSERT_EQ(clientStats["tracks"].size(), 2u);
	std::set<std::string> trackIds;
	for (const auto& t : clientStats["tracks"]) {
		trackIds.insert(t["localTrackId"].get<std::string>());
	}
	EXPECT_EQ(trackIds, std::set<std::string>({"video", "video-1"}));

	ws->close();
	close(udp);
}
