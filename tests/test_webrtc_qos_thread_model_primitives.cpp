#include <gtest/gtest.h>

#include <poll.h>

#include <chrono>
#include <filesystem>
#include <iterator>
#include <string>
#include <vector>

#include "common/ControlMailbox.h"
#include "common/LatestValue.h"
#include "common/ClientArgs.h"
#include "common/ClientIds.h"
#include "common/PlainUdpTransport.h"
#include "push/EncodedAccessUnit.h"
#include "push/PushSdkTransportThread.h"
#include "push/RawFrameEncodeWorker.h"
#include "push/RawVideoFrame.h"
#include "play/PlaySdkTransportThread.h"
#include "push/PushTrackSourceWorker.h"

TEST(WebRtcQosThreadModelPrimitivesTest, ControlMailboxSignalsFdAndTracksCapacity)
{
	webrtc_qos_plain::ControlMailbox<std::string> mailbox(2);
	ASSERT_TRUE(mailbox.valid());
	ASSERT_GE(mailbox.wakeFd(), 0);

	EXPECT_TRUE(mailbox.Post("start"));
	EXPECT_TRUE(mailbox.Post("stop"));
	EXPECT_FALSE(mailbox.Post("overflow"));
	EXPECT_EQ(mailbox.posted(), 2u);
	EXPECT_EQ(mailbox.dropped(), 1u);
	EXPECT_EQ(mailbox.depth(), 2u);

	pollfd fd{};
	fd.fd = mailbox.wakeFd();
	fd.events = POLLIN;
	ASSERT_GT(::poll(&fd, 1, 50), 0);
	EXPECT_NE(fd.revents & POLLIN, 0);
	mailbox.DrainWakeSignal();

	std::string command;
	ASSERT_TRUE(mailbox.TryPop(&command));
	EXPECT_EQ(command, "start");
	ASSERT_TRUE(mailbox.TryPop(&command));
	EXPECT_EQ(command, "stop");
	EXPECT_FALSE(mailbox.TryPop(&command));
	EXPECT_EQ(mailbox.popped(), 2u);
}

TEST(WebRtcQosThreadModelPrimitivesTest, ControlMailboxCloseRejectsPostsAndWakesOwner)
{
	webrtc_qos_plain::ControlMailbox<int> mailbox(1);
	ASSERT_TRUE(mailbox.valid());
	mailbox.Close();
	EXPECT_TRUE(mailbox.closed());
	EXPECT_FALSE(mailbox.Post(1));

	pollfd fd{};
	fd.fd = mailbox.wakeFd();
	fd.events = POLLIN;
	ASSERT_GT(::poll(&fd, 1, 50), 0);
	EXPECT_NE(fd.revents & POLLIN, 0);
	mailbox.DrainWakeSignal();
}

TEST(WebRtcQosThreadModelPrimitivesTest, LatestValueTracksVersions)
{
	struct Snapshot {
		int bitrate{0};
		bool keyframe{false};
	};

	webrtc_qos_plain::LatestValue<Snapshot> latest;
	Snapshot snapshot;
	uint64_t version = 0;
	EXPECT_FALSE(latest.Load(&snapshot, &version));
	EXPECT_FALSE(latest.hasValue());

	latest.Store({1000, false});
	ASSERT_TRUE(latest.Load(&snapshot, &version));
	EXPECT_EQ(snapshot.bitrate, 1000);
	EXPECT_FALSE(snapshot.keyframe);
	EXPECT_EQ(version, 1u);
	EXPECT_FALSE(latest.LoadIfNewer(version, &snapshot, &version));

	latest.Store({2000, true});
	ASSERT_TRUE(latest.LoadIfNewer(1, &snapshot, &version));
	EXPECT_EQ(snapshot.bitrate, 2000);
	EXPECT_TRUE(snapshot.keyframe);
	EXPECT_EQ(version, 2u);
}

TEST(WebRtcQosThreadModelPrimitivesTest, EncodedAccessUnitOwnsCopiedBytes)
{
	webrtc_qos_plain::AnnexBAccessUnit source;
	source.bytes = {0x00, 0x00, 0x00, 0x01, 0x65};
	source.mediaTimeUs = 9000;
	source.keyframe = true;

	webrtc_qos_plain::EncodedAccessUnit item;
	const webrtc_qos::TransportIds ids{};
	ASSERT_TRUE(webrtc_qos_plain::CopyEncodedAccessUnit(source, 12345, ids, &item));
	source.bytes.assign(source.bytes.size(), 0xff);

	ASSERT_EQ(item.bytes, (std::vector<uint8_t>{0x00, 0x00, 0x00, 0x01, 0x65}));
	EXPECT_EQ(item.captureTimeUs, 12345);
	EXPECT_TRUE(item.keyframe);

	const auto view = webrtc_qos_plain::ToAnnexBAccessUnitView(item);
	EXPECT_EQ(view.bytes, item.bytes.data());
	EXPECT_EQ(view.size, item.bytes.size());
	EXPECT_EQ(view.capture_time_us, item.captureTimeUs);
	EXPECT_TRUE(view.keyframe);
}

TEST(WebRtcQosThreadModelPrimitivesTest, RawVideoFrameOwnsCopiedYuvBytes)
{
	webrtc_qos_plain::RawVideoFrame frame;
	frame.width = 4;
	frame.height = 4;
	frame.captureTimeUs = 1234;
	frame.mediaTimeUs = 5678;
	frame.frameIndex = 9;
	frame.yuv420p = std::vector<uint8_t>(webrtc_qos_plain::RawVideoFrameSize(frame.width, frame.height), 0x42);

	auto copy = frame;
	frame.yuv420p.assign(frame.yuv420p.size(), 0xff);

	EXPECT_EQ(copy.width, 4);
	EXPECT_EQ(copy.height, 4);
	EXPECT_EQ(copy.captureTimeUs, 1234);
	EXPECT_EQ(copy.mediaTimeUs, 5678);
	EXPECT_EQ(copy.frameIndex, 9u);
	EXPECT_EQ(copy.yuv420p.size(), 24u);
	EXPECT_EQ(copy.yuv420p.front(), 0x42);
	EXPECT_EQ(copy.yuv420p.back(), 0x42);
}

TEST(WebRtcQosThreadModelPrimitivesTest, PushOptionsParsePerTrackV4L2Devices)
{
	const char* argv[] = {
		"push",
		"--server-ip=127.0.0.1",
		"--server-port=3000",
		"--room=room",
		"--peer=peer",
		"--encoder=x264",
		"--track=id=cam0,ssrc=11111111,weight=100,source=v4l2,device=/dev/video0,width=640,height=360,fps=30,inputFormat=mjpeg",
		"--track=id=cam1,ssrc=22222222,weight=50,source=v4l2,device=/dev/video1,width=320,height=180,fps=15",
	};
	webrtc_qos_plain::PushOptions options;
	std::string error;
	ASSERT_TRUE(webrtc_qos_plain::ParsePushOptions(
		static_cast<int>(std::size(argv)),
		const_cast<char**>(argv),
		&options,
		&error)) << error;

	ASSERT_EQ(options.tracks.size(), 2u);
	EXPECT_EQ(options.encoder, "x264");
	EXPECT_EQ(options.tracks[0].id, "cam0");
	EXPECT_EQ(options.tracks[0].source, "v4l2");
	EXPECT_EQ(options.tracks[0].v4l2Device, "/dev/video0");
	EXPECT_EQ(options.tracks[0].v4l2Width, 640);
	EXPECT_EQ(options.tracks[0].v4l2Height, 360);
	EXPECT_EQ(options.tracks[0].v4l2Fps, 30);
	EXPECT_EQ(options.tracks[0].v4l2InputFormat, "mjpeg");
	EXPECT_EQ(options.tracks[1].id, "cam1");
	EXPECT_EQ(options.tracks[1].videoSsrc, 22222222u);
	EXPECT_EQ(options.tracks[1].weight, 50u);
	EXPECT_EQ(options.tracks[1].source, "v4l2");
	EXPECT_EQ(options.tracks[1].v4l2Device, "/dev/video1");
	EXPECT_EQ(options.tracks[1].v4l2Width, 320);
	EXPECT_EQ(options.tracks[1].v4l2Height, 180);
	EXPECT_EQ(options.tracks[1].v4l2Fps, 15);
}

TEST(WebRtcQosThreadModelPrimitivesTest, PushOptionsApplyGlobalV4L2ToDefaultTrack)
{
	const char* argv[] = {
		"push",
		"--server-ip=127.0.0.1",
		"--server-port=3000",
		"--room=room",
		"--peer=peer",
		"--encoder=x264",
		"--input-v4l2=/dev/video2",
		"--v4l2-width=800",
		"--v4l2-height=600",
		"--v4l2-fps=24",
	};
	webrtc_qos_plain::PushOptions options;
	std::string error;
	ASSERT_TRUE(webrtc_qos_plain::ParsePushOptions(
		static_cast<int>(std::size(argv)),
		const_cast<char**>(argv),
		&options,
		&error)) << error;

	ASSERT_EQ(options.tracks.size(), 1u);
	EXPECT_EQ(options.tracks[0].source, "v4l2");
	EXPECT_EQ(options.tracks[0].v4l2Device, "/dev/video2");
	EXPECT_EQ(options.tracks[0].v4l2Width, 800);
	EXPECT_EQ(options.tracks[0].v4l2Height, 600);
	EXPECT_EQ(options.tracks[0].v4l2Fps, 24);
}

TEST(WebRtcQosThreadModelPrimitivesTest, VideoSessionConfigBuildsMultipleTracks)
{
	webrtc_qos_plain::VideoSessionParams params;
	params.roomId = "multi-track-room";
	params.transportId = "multi-track-transport";
	params.sourceId = "multi-track-source";
	params.debugName = "multi-track-test";
	params.tracks.push_back({"cam0", 1, 11111111u, 102, 5, 100, true});
	params.tracks.push_back({"cam1", 2, 22222222u, 102, 5, 50, false});

	const auto session = webrtc_qos_plain::MakeVideoSessionConfig(params);
	ASSERT_EQ(session.video_tracks.size(), 2u);
	EXPECT_EQ(session.ids.sender_ssrc, 11111111u);
	EXPECT_EQ(session.ids.track_id, 1u);
	EXPECT_EQ(session.video_tracks[0].ids.sender_ssrc, 11111111u);
	EXPECT_EQ(session.video_tracks[0].ids.track_id, 1u);
	EXPECT_TRUE(session.video_tracks[0].base_track);
	EXPECT_EQ(session.video_tracks[0].weight, 100u);
	EXPECT_EQ(session.video_tracks[1].ids.sender_ssrc, 22222222u);
	EXPECT_EQ(session.video_tracks[1].ids.track_id, 2u);
	EXPECT_FALSE(session.video_tracks[1].base_track);
	EXPECT_EQ(session.video_tracks[1].weight, 50u);
	EXPECT_EQ(session.video_tracks[1].h264.payload_type, 102);
}

TEST(WebRtcQosThreadModelPrimitivesTest, PushSdkTransportThreadOwnsSdkAndUdpLoop)
{
	webrtc_qos_plain::SingleVideoSessionParams params;
	params.roomId = "thread-model-test-room";
	params.transportId = "thread-model-test-transport";
	params.sourceId = "thread-model-test-source";
	params.senderSsrc = 12345678;
	params.payloadType = 102;
	params.transportCcExtId = 5;
	params.debugName = "thread-model-test";
	auto session = webrtc_qos_plain::MakeSingleVideoSessionConfig(params);

	webrtc_qos_plain::PlainUdpTransport receiver;
	std::string error;
	ASSERT_TRUE(receiver.Bind("127.0.0.1", 0, &error)) << error;

	webrtc_qos_plain::PushSdkTransportThreadConfig config;
	config.session = session;
	config.mediaRemoteIp = "127.0.0.1";
	config.mediaRemotePort = receiver.localEndpoint().port;
	config.logDir = (std::filesystem::temp_directory_path() / "webrtc-qos-push-sdk-thread-test").string();
	config.processTickMs = 1;
	config.encodedQueueCapacity = 8;
	std::filesystem::create_directories(config.logDir);

	webrtc_qos_plain::PushSdkTransportThread thread(std::move(config), nullptr);
	ASSERT_EQ(thread.Start(&error), 0) << error;

	webrtc_qos_plain::EncodedAccessUnit item;
	item.bytes = {0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84};
	item.captureTimeUs = 1000;
	item.keyframe = true;
	item.ids = session.video_tracks.front().ids;
	ASSERT_TRUE(thread.Enqueue(std::move(item)));

	for (int i = 0; i < 100; ++i) {
		const auto metrics = thread.metrics();
		if (metrics.pushedAccessUnits > 0 && metrics.loopIterations > 0) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	thread.Stop();
	const auto metrics = thread.metrics();
	EXPECT_TRUE(metrics.started);
	EXPECT_TRUE(metrics.stopped);
	EXPECT_EQ(metrics.stopReason, "stopped");
	EXPECT_TRUE(metrics.fatalError.empty());
	EXPECT_GT(metrics.lastHeartbeatUs, 0);
	EXPECT_GT(metrics.loopIterations, 0u);
	EXPECT_GE(metrics.loopGapMaxUs, 0);
	EXPECT_EQ(metrics.enqueuedAccessUnits, 1u);
	EXPECT_EQ(metrics.pushedAccessUnits, 1u);
	EXPECT_EQ(metrics.pushFailures, 0u);
	EXPECT_GT(metrics.localEndpoint.port, 0);
	EXPECT_EQ(metrics.remoteEndpoint.ip, "127.0.0.1");
	EXPECT_EQ(metrics.remoteEndpoint.port, receiver.localEndpoint().port);
	std::filesystem::remove_all((std::filesystem::temp_directory_path() / "webrtc-qos-push-sdk-thread-test"));
}

TEST(WebRtcQosThreadModelPrimitivesTest, PushSdkTransportThreadDrainsPerTrackQueues)
{
	webrtc_qos_plain::VideoSessionParams params;
	params.roomId = "thread-model-multi-track-room";
	params.transportId = "thread-model-multi-track-transport";
	params.sourceId = "thread-model-multi-track-source";
	params.debugName = "thread-model-multi-track-test";
	params.tracks.push_back({"cam0", 1, 11111111u, 102, 5, 100, true});
	params.tracks.push_back({"cam1", 2, 22222222u, 102, 5, 100, false});
	auto session = webrtc_qos_plain::MakeVideoSessionConfig(params);

	webrtc_qos_plain::PlainUdpTransport receiver;
	std::string error;
	ASSERT_TRUE(receiver.Bind("127.0.0.1", 0, &error)) << error;

	webrtc_qos_plain::PushSdkTransportThreadConfig config;
	config.session = session;
	config.mediaRemoteIp = "127.0.0.1";
	config.mediaRemotePort = receiver.localEndpoint().port;
	config.logDir = (std::filesystem::temp_directory_path() / "webrtc-qos-push-sdk-thread-multi-track-test").string();
	config.processTickMs = 1;
	config.encodedQueueCapacity = 2;
	std::filesystem::create_directories(config.logDir);

	webrtc_qos_plain::PushSdkTransportThread thread(std::move(config), nullptr);
	ASSERT_EQ(thread.Start(&error), 0) << error;

	for (size_t index = 0; index < session.video_tracks.size(); ++index) {
		webrtc_qos_plain::EncodedAccessUnit item;
		item.bytes = {0x00, 0x00, 0x00, 0x01, 0x65, static_cast<uint8_t>(0x80 + index)};
		item.captureTimeUs = 1000 + static_cast<int64_t>(index) * 33333;
		item.keyframe = true;
		item.ids = session.video_tracks[index].ids;
		ASSERT_TRUE(thread.Enqueue(std::move(item)));
	}

	for (int i = 0; i < 100; ++i) {
		const auto metrics = thread.metrics();
		if (metrics.pushedAccessUnits >= 2 && metrics.tracks.size() == 2) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	thread.Stop();
	const auto metrics = thread.metrics();
	ASSERT_EQ(metrics.tracks.size(), 2u);
	EXPECT_EQ(metrics.enqueuedAccessUnits, 2u);
	EXPECT_EQ(metrics.droppedAccessUnits, 0u);
	EXPECT_EQ(metrics.pushedAccessUnits, 2u);
	EXPECT_EQ(metrics.pushFailures, 0u);

	for (const auto& track : metrics.tracks) {
		EXPECT_EQ(track.enqueuedAccessUnits, 1u);
		EXPECT_EQ(track.droppedAccessUnits, 0u);
		EXPECT_EQ(track.pushedAccessUnits, 1u);
		EXPECT_EQ(track.pushFailures, 0u);
		EXPECT_TRUE(track.adaptationAvailable);
		EXPECT_TRUE(track.snapshotAvailable);
		EXPECT_TRUE(track.trackId == 1u || track.trackId == 2u);
		EXPECT_TRUE(track.senderSsrc == 11111111u || track.senderSsrc == 22222222u);
	}
	std::filesystem::remove_all((std::filesystem::temp_directory_path() / "webrtc-qos-push-sdk-thread-multi-track-test"));
}

TEST(WebRtcQosThreadModelPrimitivesTest, PushTrackSourceWorkerEncodesAndQueuesOnWorkerThread)
{
	webrtc_qos_plain::SingleVideoSessionParams params;
	params.roomId = "thread-model-source-worker-room";
	params.transportId = "thread-model-source-worker-transport";
	params.sourceId = "thread-model-source-worker-source";
	params.senderSsrc = 33333333;
	params.payloadType = 102;
	params.transportCcExtId = 5;
	params.debugName = "thread-model-source-worker-test";
	auto session = webrtc_qos_plain::MakeSingleVideoSessionConfig(params);

	webrtc_qos_plain::PlainUdpTransport receiver;
	std::string error;
	ASSERT_TRUE(receiver.Bind("127.0.0.1", 0, &error)) << error;

	webrtc_qos_plain::PushSdkTransportThreadConfig sdkConfig;
	sdkConfig.session = session;
	sdkConfig.mediaRemoteIp = "127.0.0.1";
	sdkConfig.mediaRemotePort = receiver.localEndpoint().port;
	sdkConfig.logDir = (std::filesystem::temp_directory_path() / "webrtc-qos-push-source-worker-sdk-test").string();
	sdkConfig.processTickMs = 1;
	sdkConfig.encodedQueueCapacity = 8;
	std::filesystem::create_directories(sdkConfig.logDir);

	webrtc_qos_plain::PushSdkTransportThread sdkThread(std::move(sdkConfig), nullptr);
	ASSERT_EQ(sdkThread.Start(&error), 0) << error;

	webrtc_qos_plain::PushTrackSourceWorkerConfig sourceConfig;
	sourceConfig.ids = session.video_tracks.front().ids;
	sourceConfig.trackName = "cam0";
	sourceConfig.mode = webrtc_qos_plain::PushTrackSourceMode::kSynthetic;
	sourceConfig.encoder = "x264";
	sourceConfig.processTickMs = 1;
	sourceConfig.syntheticWidth = 64;
	sourceConfig.syntheticHeight = 48;
	sourceConfig.syntheticFps = 15;
	sourceConfig.startBitrateBps = 300000;
	sourceConfig.minBitrateBps = 150000;
	sourceConfig.maxBitrateBps = 600000;

	webrtc_qos_plain::PushTrackSourceWorker worker(sourceConfig, &sdkThread, nullptr);
	ASSERT_EQ(worker.Start(&error), 0) << error;
	webrtc_qos::EncoderAdaptation adaptation;
	adaptation.target_bitrate_bps = 250000;
	adaptation.max_fps = 10;
	adaptation.request_keyframe = true;
	worker.StoreEncoderAdaptation(adaptation);

	for (int i = 0; i < 100; ++i) {
		const auto workerMetrics = worker.metrics();
		const auto sdkMetrics = sdkThread.metrics();
		if (workerMetrics.queuedAu > 0 && sdkMetrics.pushedAccessUnits > 0) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	worker.Stop();
	sdkThread.Stop();

	const auto workerMetrics = worker.metrics();
	const auto sdkMetrics = sdkThread.metrics();
	EXPECT_TRUE(workerMetrics.started);
	EXPECT_TRUE(workerMetrics.stopped);
	EXPECT_TRUE(workerMetrics.fatalError.empty());
	EXPECT_GT(workerMetrics.lastHeartbeatUs, 0);
	EXPECT_GT(workerMetrics.loopIterations, 0u);
	EXPECT_GT(workerMetrics.queuedAu, 0u);
	EXPECT_EQ(workerMetrics.enqueueFailures, 0u);
	EXPECT_GT(workerMetrics.sourceMetrics.accessUnits, 0u);
	EXPECT_GT(workerMetrics.sourceMetrics.keyframes, 0u);
	EXPECT_TRUE(sdkMetrics.started);
	EXPECT_TRUE(sdkMetrics.stopped);
	EXPECT_EQ(sdkMetrics.pushFailures, 0u);
	EXPECT_GT(sdkMetrics.pushedAccessUnits, 0u);
	ASSERT_EQ(sdkMetrics.tracks.size(), 1u);
	EXPECT_GT(sdkMetrics.tracks.front().pushedAccessUnits, 0u);
	std::filesystem::remove_all((std::filesystem::temp_directory_path() / "webrtc-qos-push-source-worker-sdk-test"));
}

TEST(WebRtcQosThreadModelPrimitivesTest, PushTrackSourceWorkersEncodeTwoTracksIndependently)
{
	webrtc_qos_plain::VideoSessionParams params;
	params.roomId = "thread-model-source-worker-two-track-room";
	params.transportId = "thread-model-source-worker-two-track-transport";
	params.sourceId = "thread-model-source-worker-two-track-source";
	params.debugName = "thread-model-source-worker-two-track-test";
	params.tracks.push_back({"cam0", 1, 44444441u, 102, 5, 100, true});
	params.tracks.push_back({"cam1", 2, 44444442u, 102, 5, 100, false});
	auto session = webrtc_qos_plain::MakeVideoSessionConfig(params);

	webrtc_qos_plain::PlainUdpTransport receiver;
	std::string error;
	ASSERT_TRUE(receiver.Bind("127.0.0.1", 0, &error)) << error;

	webrtc_qos_plain::PushSdkTransportThreadConfig sdkConfig;
	sdkConfig.session = session;
	sdkConfig.mediaRemoteIp = "127.0.0.1";
	sdkConfig.mediaRemotePort = receiver.localEndpoint().port;
	sdkConfig.logDir = (std::filesystem::temp_directory_path() / "webrtc-qos-push-source-worker-two-track-sdk-test").string();
	sdkConfig.processTickMs = 1;
	sdkConfig.encodedQueueCapacity = 16;
	std::filesystem::create_directories(sdkConfig.logDir);

	webrtc_qos_plain::PushSdkTransportThread sdkThread(std::move(sdkConfig), nullptr);
	ASSERT_EQ(sdkThread.Start(&error), 0) << error;

	std::vector<std::unique_ptr<webrtc_qos_plain::PushTrackSourceWorker>> workers;
	for (const auto& track : session.video_tracks) {
		webrtc_qos_plain::PushTrackSourceWorkerConfig sourceConfig;
		sourceConfig.ids = track.ids;
		sourceConfig.trackName = "cam" + std::to_string(track.ids.track_id);
		sourceConfig.mode = webrtc_qos_plain::PushTrackSourceMode::kSynthetic;
		sourceConfig.encoder = "x264";
		sourceConfig.processTickMs = 1;
		sourceConfig.syntheticWidth = 64;
		sourceConfig.syntheticHeight = 48;
		sourceConfig.syntheticFps = 15;
		sourceConfig.startBitrateBps = 300000;
		sourceConfig.minBitrateBps = 150000;
		sourceConfig.maxBitrateBps = 600000;
		auto worker = std::make_unique<webrtc_qos_plain::PushTrackSourceWorker>(sourceConfig, &sdkThread, nullptr);
		ASSERT_EQ(worker->Start(&error), 0) << error;
		workers.push_back(std::move(worker));
	}

	for (int i = 0; i < 150; ++i) {
		bool workersReady = true;
		for (const auto& worker : workers) {
			workersReady = workersReady && worker->metrics().queuedAu > 0;
		}
		const auto sdkMetrics = sdkThread.metrics();
		bool sdkReady = sdkMetrics.pushedAccessUnits >= 2 && sdkMetrics.tracks.size() == 2;
		for (const auto& track : sdkMetrics.tracks) {
			sdkReady = sdkReady && track.pushedAccessUnits > 0;
		}
		if (workersReady && sdkReady) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	for (auto& worker : workers) worker->Stop();
	sdkThread.Stop();

	const auto sdkMetrics = sdkThread.metrics();
	ASSERT_EQ(sdkMetrics.tracks.size(), 2u);
	EXPECT_EQ(sdkMetrics.pushFailures, 0u);
	EXPECT_GE(sdkMetrics.pushedAccessUnits, 2u);
	for (const auto& worker : workers) {
		const auto workerMetrics = worker->metrics();
		EXPECT_TRUE(workerMetrics.started);
		EXPECT_TRUE(workerMetrics.stopped);
		EXPECT_TRUE(workerMetrics.fatalError.empty());
		EXPECT_GT(workerMetrics.queuedAu, 0u);
		EXPECT_EQ(workerMetrics.enqueueFailures, 0u);
		EXPECT_GT(workerMetrics.sourceMetrics.accessUnits, 0u);
		EXPECT_GT(workerMetrics.sourceMetrics.keyframes, 0u);
	}
	for (const auto& track : sdkMetrics.tracks) {
		EXPECT_GT(track.pushedAccessUnits, 0u);
		EXPECT_EQ(track.pushFailures, 0u);
		EXPECT_TRUE(track.trackId == 1u || track.trackId == 2u);
		EXPECT_TRUE(track.senderSsrc == 44444441u || track.senderSsrc == 44444442u);
	}
	std::filesystem::remove_all((std::filesystem::temp_directory_path() / "webrtc-qos-push-source-worker-two-track-sdk-test"));
}

TEST(WebRtcQosThreadModelPrimitivesTest, RawFrameEncodeWorkerConsumesRawQueueAndPushesSdk)
{
	webrtc_qos_plain::SingleVideoSessionParams params;
	params.roomId = "thread-model-raw-encode-room";
	params.transportId = "thread-model-raw-encode-transport";
	params.sourceId = "thread-model-raw-encode-source";
	params.senderSsrc = 55555555;
	params.payloadType = 102;
	params.transportCcExtId = 5;
	params.debugName = "thread-model-raw-encode-test";
	auto session = webrtc_qos_plain::MakeSingleVideoSessionConfig(params);

	webrtc_qos_plain::PlainUdpTransport receiver;
	std::string error;
	ASSERT_TRUE(receiver.Bind("127.0.0.1", 0, &error)) << error;

	webrtc_qos_plain::PushSdkTransportThreadConfig sdkConfig;
	sdkConfig.session = session;
	sdkConfig.mediaRemoteIp = "127.0.0.1";
	sdkConfig.mediaRemotePort = receiver.localEndpoint().port;
	sdkConfig.logDir = (std::filesystem::temp_directory_path() / "webrtc-qos-raw-encode-worker-sdk-test").string();
	sdkConfig.processTickMs = 1;
	sdkConfig.encodedQueueCapacity = 8;
	std::filesystem::create_directories(sdkConfig.logDir);

	webrtc_qos_plain::PushSdkTransportThread sdkThread(std::move(sdkConfig), nullptr);
	ASSERT_EQ(sdkThread.Start(&error), 0) << error;

	auto rawQueue = std::make_shared<webrtc_qos_plain::BoundedQueue<webrtc_qos_plain::RawVideoFrame>>(3);
	webrtc_qos_plain::RawFrameEncodeWorkerConfig encodeConfig;
	encodeConfig.ids = session.video_tracks.front().ids;
	encodeConfig.trackName = "cam0";
	encodeConfig.width = 64;
	encodeConfig.height = 48;
	encodeConfig.fps = 15;
	encodeConfig.processTickMs = 1;
	encodeConfig.startBitrateBps = 300000;
	encodeConfig.minBitrateBps = 150000;
	encodeConfig.maxBitrateBps = 600000;

	webrtc_qos_plain::RawFrameEncodeWorker worker(encodeConfig, rawQueue, &sdkThread, nullptr);
	ASSERT_EQ(worker.Start(&error), 0) << error;

	for (int frameIndex = 0; frameIndex < 3; ++frameIndex) {
		webrtc_qos_plain::RawVideoFrame raw;
		raw.width = 64;
		raw.height = 48;
		raw.captureTimeUs = 1000000 + frameIndex * 33333;
		raw.mediaTimeUs = frameIndex * 33333;
		raw.frameIndex = static_cast<uint64_t>(frameIndex);
		raw.yuv420p.resize(webrtc_qos_plain::RawVideoFrameSize(raw.width, raw.height));
		std::fill(raw.yuv420p.begin(), raw.yuv420p.begin() + raw.width * raw.height, static_cast<uint8_t>(16 + frameIndex));
		std::fill(raw.yuv420p.begin() + raw.width * raw.height, raw.yuv420p.end(), 128);
		ASSERT_TRUE(rawQueue->PushDropOldest(std::move(raw)));
	}

	for (int i = 0; i < 150; ++i) {
		const auto workerMetrics = worker.metrics();
		const auto sdkMetrics = sdkThread.metrics();
		if (workerMetrics.queuedAu > 0 && sdkMetrics.pushedAccessUnits > 0) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	worker.Stop();
	sdkThread.Stop();

	const auto workerMetrics = worker.metrics();
	const auto sdkMetrics = sdkThread.metrics();
	EXPECT_TRUE(workerMetrics.started);
	EXPECT_TRUE(workerMetrics.stopped);
	EXPECT_TRUE(workerMetrics.fatalError.empty());
	EXPECT_GT(workerMetrics.queuedAu, 0u);
	EXPECT_EQ(workerMetrics.enqueueFailures, 0u);
	EXPECT_GT(workerMetrics.sourceMetrics.accessUnits, 0u);
	EXPECT_GT(workerMetrics.sourceMetrics.keyframes, 0u);
	EXPECT_EQ(workerMetrics.rawQueueDroppedFrames, 0u);
	EXPECT_GT(sdkMetrics.pushedAccessUnits, 0u);
	EXPECT_EQ(sdkMetrics.pushFailures, 0u);
	std::filesystem::remove_all((std::filesystem::temp_directory_path() / "webrtc-qos-raw-encode-worker-sdk-test"));
}

TEST(WebRtcQosThreadModelPrimitivesTest, PlaySdkTransportThreadOwnsSdkAndUdpLoop)
{
	webrtc_qos_plain::SingleVideoSessionParams params;
	params.roomId = "thread-model-play-room";
	params.transportId = "thread-model-play-transport";
	params.sourceId = "thread-model-play-source";
	params.receiverId = "thread-model-play-receiver";
	params.senderSsrc = 87654321;
	params.payloadType = 102;
	params.transportCcExtId = 5;
	params.debugName = "thread-model-play-test";
	auto session = webrtc_qos_plain::MakeSingleVideoSessionConfig(params);

	webrtc_qos_plain::PlainUdpTransport receiver;
	std::string error;
	ASSERT_TRUE(receiver.Bind("127.0.0.1", 0, &error)) << error;

	webrtc_qos_plain::PlaySdkTransportThreadConfig config;
	config.session = session;
	config.udp = std::move(receiver);
	config.mediaRemoteIp = "127.0.0.1";
	config.mediaRemotePort = 9;
	config.logDir = (std::filesystem::temp_directory_path() / "webrtc-qos-play-sdk-thread-test").string();
	config.processTickMs = 1;
	config.decodedAccessUnitOutput = [](const webrtc_qos::AnnexBAccessUnitView&) {
		return webrtc_qos::Status::Ok();
	};
	std::filesystem::create_directories(config.logDir);

	webrtc_qos_plain::PlaySdkTransportThread thread(std::move(config), nullptr);
	ASSERT_EQ(thread.Start(&error), 0) << error;

	for (int i = 0; i < 100; ++i) {
		const auto metrics = thread.metrics();
		if (metrics.loopIterations > 0) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	thread.Stop();
	const auto metrics = thread.metrics();
	EXPECT_TRUE(metrics.started);
	EXPECT_TRUE(metrics.stopped);
	EXPECT_EQ(metrics.stopReason, "stopped");
	EXPECT_TRUE(metrics.fatalError.empty());
	EXPECT_GT(metrics.lastHeartbeatUs, 0);
	EXPECT_GT(metrics.loopIterations, 0u);
	EXPECT_GE(metrics.loopGapMaxUs, 0);
	EXPECT_GT(metrics.localEndpoint.port, 0);
	EXPECT_EQ(metrics.remoteEndpoint.ip, "127.0.0.1");
	EXPECT_EQ(metrics.remoteEndpoint.port, 9);
	std::filesystem::remove_all((std::filesystem::temp_directory_path() / "webrtc-qos-play-sdk-thread-test"));
}

TEST(WebRtcQosThreadModelPrimitivesTest, PlaySdkTransportThreadExposesPerTrackSnapshots)
{
	webrtc_qos_plain::VideoSessionParams params;
	params.roomId = "thread-model-play-two-track-room";
	params.transportId = "thread-model-play-two-track-transport";
	params.sourceId = "thread-model-play-two-track-source";
	params.receiverId = "thread-model-play-two-track-receiver";
	params.debugName = "thread-model-play-two-track-test";
	params.tracks.push_back({"cam0", 1, 87654331u, 102, 5, 100, true});
	params.tracks.push_back({"cam1", 2, 87654332u, 102, 5, 100, false});
	auto session = webrtc_qos_plain::MakeVideoSessionConfig(params);

	webrtc_qos_plain::PlainUdpTransport receiver;
	std::string error;
	ASSERT_TRUE(receiver.Bind("127.0.0.1", 0, &error)) << error;

	webrtc_qos_plain::PlaySdkTransportThreadConfig config;
	config.session = session;
	config.udp = std::move(receiver);
	config.mediaRemoteIp = "127.0.0.1";
	config.mediaRemotePort = 9;
	config.logDir = (std::filesystem::temp_directory_path() / "webrtc-qos-play-sdk-thread-two-track-test").string();
	config.processTickMs = 1;
	config.decodedAccessUnitOutput = [](const webrtc_qos::AnnexBAccessUnitView&) {
		return webrtc_qos::Status::Ok();
	};
	std::filesystem::create_directories(config.logDir);

	webrtc_qos_plain::PlaySdkTransportThread thread(std::move(config), nullptr);
	ASSERT_EQ(thread.Start(&error), 0) << error;

	for (int i = 0; i < 100; ++i) {
		const auto metrics = thread.metrics();
		bool ready = metrics.tracks.size() == 2;
		for (const auto& track : metrics.tracks) {
			ready = ready && track.snapshotAvailable;
		}
		if (ready) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	thread.Stop();
	const auto metrics = thread.metrics();
	ASSERT_EQ(metrics.tracks.size(), 2u);
	EXPECT_TRUE(metrics.started);
	EXPECT_TRUE(metrics.stopped);
	for (const auto& track : metrics.tracks) {
		EXPECT_TRUE(track.snapshotAvailable);
		EXPECT_TRUE(track.trackId == 1u || track.trackId == 2u);
		EXPECT_TRUE(track.senderSsrc == 87654331u || track.senderSsrc == 87654332u);
		EXPECT_EQ(track.snapshot.ids.track_id, track.trackId);
		EXPECT_EQ(track.snapshot.ids.sender_ssrc, track.senderSsrc);
	}
	std::filesystem::remove_all((std::filesystem::temp_directory_path() / "webrtc-qos-play-sdk-thread-two-track-test"));
}
