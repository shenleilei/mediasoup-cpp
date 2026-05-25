#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <thread>
#include <unistd.h>

#include "common/BoundedQueue.h"
#include "play/DecodedAuSinkWorker.h"
#include "play/FfmpegDecodeSink.h"
#include "push/RealtimeH264Source.h"

namespace {

webrtc_qos_plain::RealtimeH264SourceConfig SmallSourceConfig()
{
	webrtc_qos_plain::RealtimeH264SourceConfig config;
	config.width = 64;
	config.height = 48;
	config.fps = 15;
	config.bitrateBps = 500000;
	config.minBitrateBps = 120000;
	config.maxBitrateBps = 800000;
	return config;
}

} // namespace

TEST(WebRtcQosDecodeSinkTest, BoundedQueueDropsOldestAndCloses)
{
	webrtc_qos_plain::BoundedQueue<int> queue(2);
	EXPECT_TRUE(queue.PushDropOldest(1));
	EXPECT_TRUE(queue.PushDropOldest(2));
	EXPECT_TRUE(queue.PushDropOldest(3));
	EXPECT_EQ(queue.dropped(), 1u);
	EXPECT_EQ(queue.maxDepth(), 2u);

	int value = 0;
	ASSERT_TRUE(queue.TryPop(&value));
	EXPECT_EQ(value, 2);
	ASSERT_TRUE(queue.TryPop(&value));
	EXPECT_EQ(value, 3);
	EXPECT_FALSE(queue.TryPop(&value));

	queue.Close();
	EXPECT_FALSE(queue.Pop(&value));
	EXPECT_FALSE(queue.PushDropOldest(4));
}

TEST(WebRtcQosDecodeSinkTest, BoundedQueuePopForTimesOutWithoutClosing)
{
	webrtc_qos_plain::BoundedQueue<int> queue(1);
	int value = 0;
	EXPECT_FALSE(queue.PopFor(&value, std::chrono::milliseconds(1)));
	EXPECT_FALSE(queue.closed());
	queue.Close();
	EXPECT_FALSE(queue.PopFor(&value, std::chrono::milliseconds(1)));
	EXPECT_TRUE(queue.closed());
}

TEST(WebRtcQosDecodeSinkTest, DecodesSyntheticH264AccessUnits)
{
	std::string error;
	webrtc_qos_plain::RealtimeH264Source source(SmallSourceConfig());
	ASSERT_TRUE(source.Open(&error)) << error;

	webrtc_qos_plain::FfmpegDecodeSink decoder;
	ASSERT_TRUE(decoder.Open(&error)) << error;

	webrtc_qos_plain::AnnexBAccessUnit au;
	uint64_t decodedInputs = 0;
	for (int i = 0; i < 20; ++i) {
		const int64_t nowUs = 1000000 + i * 70000;
		if (!source.NextAccessUnit(nowUs, &au, &error)) continue;
		webrtc_qos::AnnexBAccessUnitView view;
		view.bytes = au.bytes.data();
		view.size = au.bytes.size();
		view.capture_time_us = au.mediaTimeUs;
		view.keyframe = au.keyframe;
		ASSERT_TRUE(decoder.Decode(view, nowUs, &error)) << error;
		++decodedInputs;
		if (decoder.metrics().decodedFrames > 0) break;
	}

	EXPECT_GT(decodedInputs, 0u);
	const auto& metrics = decoder.metrics();
	EXPECT_TRUE(metrics.enabled);
	EXPECT_GT(metrics.accessUnitsIn, 0u);
	EXPECT_GT(metrics.keyframesIn, 0u);
	EXPECT_GT(metrics.decodedFrames, 0u);
	EXPECT_EQ(metrics.decodeErrors, 0u);
	EXPECT_EQ(metrics.width, 64);
	EXPECT_EQ(metrics.height, 48);
	EXPECT_GE(metrics.firstFrameDelayUs, 0);
}

TEST(WebRtcQosDecodeSinkTest, DecodedAuSinkWorkerWritesAndDecodesAsynchronously)
{
	std::string error;
	webrtc_qos_plain::RealtimeH264Source source(SmallSourceConfig());
	ASSERT_TRUE(source.Open(&error)) << error;

	const auto output = std::filesystem::temp_directory_path() /
		("webrtc-qos-decoded-sink-worker-" + std::to_string(::getpid()) + ".h264");
	std::filesystem::remove(output);
	webrtc_qos_plain::DecodedAuSinkWorker worker(
		false,
		output.string(),
		true,
		nullptr,
		8);
	ASSERT_TRUE(worker.Start(&error)) << error;

	webrtc_qos_plain::AnnexBAccessUnit au;
	uint64_t enqueued = 0;
	for (int i = 0; i < 30; ++i) {
		const int64_t nowUs = 1000000 + i * 70000;
		if (!source.NextAccessUnit(nowUs, &au, &error)) continue;
		webrtc_qos::AnnexBAccessUnitView view;
		view.bytes = au.bytes.data();
		view.size = au.bytes.size();
		view.capture_time_us = au.mediaTimeUs;
		view.keyframe = au.keyframe;
		ASSERT_TRUE(worker.Enqueue(view)) << "enqueue failed";
		++enqueued;
		if (enqueued >= 8) break;
	}
	EXPECT_GT(enqueued, 0u);

	for (int i = 0; i < 50; ++i) {
		const auto metrics = worker.metrics();
		if (metrics.writtenAccessUnits > 0 && metrics.qoe.decodedFrames > 0) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	worker.Stop();
	const auto metrics = worker.metrics();
	EXPECT_GT(metrics.enqueuedAccessUnits, 0u);
	EXPECT_GT(metrics.writtenAccessUnits, 0u);
	EXPECT_EQ(metrics.sinkWriteFailures, 0u);
	EXPECT_TRUE(metrics.started);
	EXPECT_TRUE(metrics.stopped);
	EXPECT_EQ(metrics.stopReason, "queue_closed");
	EXPECT_GT(metrics.lastHeartbeatUs, 0);
	EXPECT_GT(metrics.loopIterations, 0u);
	EXPECT_GE(metrics.loopGapMaxUs, 0);
	EXPECT_GT(metrics.qoe.decodedFrames, 0u);
	EXPECT_EQ(metrics.qoe.decodeErrors, 0u);
	EXPECT_TRUE(std::filesystem::exists(output));
	EXPECT_GT(std::filesystem::file_size(output), 0u);
	std::filesystem::remove(output);
}

TEST(WebRtcQosDecodeSinkTest, DecodedAuSinkWorkerCopiesAccessUnitBytesBeforeReturning)
{
	const auto output = std::filesystem::temp_directory_path() /
		("webrtc-qos-decoded-sink-worker-copy-" + std::to_string(::getpid()) + ".h264");
	std::filesystem::remove(output);
	webrtc_qos_plain::DecodedAuSinkWorker worker(
		false,
		output.string(),
		false,
		nullptr,
		4);
	std::string error;
	ASSERT_TRUE(worker.Start(&error)) << error;

	std::vector<uint8_t> bytes = {0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84};
	webrtc_qos::AnnexBAccessUnitView view;
	view.bytes = bytes.data();
	view.size = bytes.size();
	view.capture_time_us = 1234;
	view.keyframe = true;
	ASSERT_TRUE(worker.Enqueue(view));
	std::fill(bytes.begin(), bytes.end(), 0xff);

	for (int i = 0; i < 50; ++i) {
		if (worker.metrics().writtenAccessUnits > 0) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	worker.Stop();
	ASSERT_TRUE(std::filesystem::exists(output));
	ASSERT_EQ(std::filesystem::file_size(output), 7u);

	std::ifstream input(output, std::ios::binary);
	std::vector<uint8_t> stored(
		(std::istreambuf_iterator<char>(input)),
		std::istreambuf_iterator<char>());
	EXPECT_EQ(stored, (std::vector<uint8_t>{0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84}));
	std::filesystem::remove(output);
}

TEST(WebRtcQosDecodeSinkTest, DecodedAuSinkWorkerRejectsWrongTrackAndReportsIdentity)
{
	webrtc_qos_plain::DecodedAuSinkWorker worker(
		true,
		"",
		false,
		nullptr,
		4,
		0,
		7,
		777777,
		"cam7");
	std::string error;
	ASSERT_TRUE(worker.Start(&error)) << error;

	std::vector<uint8_t> wrongBytes = {0x00, 0x00, 0x00, 0x01, 0x65};
	webrtc_qos::AnnexBAccessUnitView wrongView;
	wrongView.bytes = wrongBytes.data();
	wrongView.size = wrongBytes.size();
	wrongView.capture_time_us = 1000;
	wrongView.keyframe = true;
	wrongView.ids.track_id = 8;
	wrongView.ids.sender_ssrc = 888888;
	EXPECT_FALSE(worker.Enqueue(wrongView));

	std::vector<uint8_t> rightBytes = {0x00, 0x00, 0x00, 0x01, 0x65, 0x88};
	webrtc_qos::AnnexBAccessUnitView rightView;
	rightView.bytes = rightBytes.data();
	rightView.size = rightBytes.size();
	rightView.capture_time_us = 2000;
	rightView.keyframe = true;
	rightView.ids.track_id = 7;
	rightView.ids.sender_ssrc = 777777;
	ASSERT_TRUE(worker.Enqueue(rightView));

	for (int i = 0; i < 50; ++i) {
		if (worker.metrics().writtenAccessUnits > 0) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	worker.Stop();

	const auto metrics = worker.metrics();
	EXPECT_EQ(metrics.trackId, 7u);
	EXPECT_EQ(metrics.senderSsrc, 777777u);
	EXPECT_EQ(metrics.trackName, "cam7");
	EXPECT_EQ(metrics.enqueuedAccessUnits, 1u);
	EXPECT_EQ(metrics.writtenAccessUnits, 1u);
	EXPECT_EQ(metrics.enqueuedAccessUnitsByTrack.at(7), 1u);
	EXPECT_EQ(metrics.writtenAccessUnitsByTrack.at(7), 1u);
	EXPECT_EQ(metrics.enqueuedAccessUnitsByTrack.count(8), 0u);
	EXPECT_EQ(metrics.writtenAccessUnitsByTrack.count(8), 0u);
	EXPECT_EQ(metrics.sinkWriteFailures, 0u);
}
