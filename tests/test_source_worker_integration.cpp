#include <gtest/gtest.h>

#include "../client/ThreadTypes.h"
#include "../client/SourceWorker.h"

#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

namespace {

constexpr const char* kTestMp4 = "tests/fixtures/media/test_sweep.mp4";

bool testFileExists()
{
	FILE* f = std::fopen(kTestMp4, "r");
	if (!f) return false;
	std::fclose(f);
	return true;
}

} // namespace

TEST(SourceWorkerIntegration, ProducesEncodedAccessUnits)
{
	if (!testFileExists()) {
		GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found";
	}

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

	std::this_thread::sleep_for(std::chrono::milliseconds(1200));
	worker.stop();

	std::vector<mt::EncodedAccessUnit> frames;
	mt::EncodedAccessUnit au;
	while (auQueue.tryPop(au)) frames.push_back(std::move(au));

	ASSERT_GT(frames.size(), 5u);
	EXPECT_TRUE(frames.front().isKeyframe);
	EXPECT_TRUE(frames.front().encoderRecreated);
	EXPECT_EQ(frames.front().ssrc, 11111111u);
	EXPECT_EQ(frames.front().payloadType, 96u);
}

TEST(SourceWorkerIntegration, SetEncodingParametersChangesOutput)
{
	if (!testFileExists()) {
		GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found";
	}

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

	uint64_t initialGen = UINT64_MAX;
	{
		mt::EncodedAccessUnit initialAu;
		while (auQueue.tryPop(initialAu)) initialGen = initialAu.configGeneration;
	}
	EXPECT_EQ(initialGen, 0u);

	mt::TrackControlCommand cmd;
	cmd.type = mt::TrackCommandType::SetEncodingParameters;
	cmd.commandId = 42;
	cmd.bitrateBps = 300000;
	cmd.fps = 15;
	cmd.scaleResolutionDownBy = 2.0;
	ASSERT_TRUE(ctrlQueue.tryPush(std::move(cmd)));

	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	worker.stop();

	mt::CommandAck ack;
	bool gotAck = false;
	while (ackQueue.tryPop(ack)) gotAck = true;
	ASSERT_TRUE(gotAck);
	EXPECT_TRUE(ack.applied);
	EXPECT_EQ(ack.commandId, 42u);
	EXPECT_EQ(ack.actualBitrateBps, 300000);
	EXPECT_GT(ack.configGeneration, 0u);

	uint64_t newGen = 0;
	mt::EncodedAccessUnit updatedAu;
	while (auQueue.tryPop(updatedAu)) newGen = updatedAu.configGeneration;
	EXPECT_GT(newGen, 0u);
}
