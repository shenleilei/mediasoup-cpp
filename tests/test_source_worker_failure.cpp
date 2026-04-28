#include <gtest/gtest.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
}

#include <chrono>
#include <cstdio>
#include <thread>
#include "../client/SourceWorker.h"

namespace {

constexpr const char* kTestMp4 = "tests/fixtures/media/test_sweep.mp4";

bool testFileExists()
{
	FILE* f = std::fopen(kTestMp4, "r");
	if (!f) return false;
	std::fclose(f);
	return true;
}

class ScopedDecoderOpenFailure {
public:
	ScopedDecoderOpenFailure()
	{
		previousHook_ = mediasoup::ffmpeg::Decoder::SetOpenFailureHook(&ScopedDecoderOpenFailure::FailDecoderOpen);
	}

	~ScopedDecoderOpenFailure()
	{
		mediasoup::ffmpeg::Decoder::SetOpenFailureHook(previousHook_);
	}

private:
	static int FailDecoderOpen(AVCodecContext*, const AVCodec* codec)
	{
		return (codec && av_codec_is_decoder(codec)) ? AVERROR(EIO) : 0;
	}

	mediasoup::ffmpeg::Decoder::OpenFailureHook previousHook_{nullptr};
};

class ScopedReadFailureCountdown {
public:
	explicit ScopedReadFailureCountdown(int readsBeforeFailure)
	{
		previousValue_ = mediasoup::ffmpeg::InputFormat::SetReadFailureCountdown(readsBeforeFailure);
	}

	~ScopedReadFailureCountdown()
	{
		mediasoup::ffmpeg::InputFormat::SetReadFailureCountdown(previousValue_);
	}

private:
	int previousValue_{-1};
};

} // namespace

TEST(SourceWorkerFailureHandling, DecoderOpenFailureStopsWorkerWithoutOutput)
{
	if (!testFileExists()) {
		GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found";
	}

	ScopedDecoderOpenFailure failDecoderOpen;

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

	std::this_thread::sleep_for(std::chrono::milliseconds(150));
	worker.stop();

	mt::EncodedAccessUnit au;
	EXPECT_FALSE(auQueue.tryPop(au));
}

TEST(SourceWorkerFailureHandling, RuntimeReadFailureStopsWorkerWithoutTerminatingProcess)
{
	if (!testFileExists()) {
		GTEST_SKIP() << "tests/fixtures/media/test_sweep.mp4 not found";
	}

	ScopedReadFailureCountdown failDuringRead(2);

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

	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	worker.stop();

	SUCCEED() << "worker stop returned after runtime read failure";
}
