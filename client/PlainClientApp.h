#pragma once

#include "PlainClientSupport.h"
#include "RtcpHandler.h"
#include "WsClient.h"
#include "ffmpeg/BitstreamFilter.h"
#include "ffmpeg/Decoder.h"
#include "ffmpeg/Encoder.h"
#include "ffmpeg/InputFormat.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

class PlainClientApp {
public:
	static int RunPlainClientApp(int argc, char* argv[]);
	static std::atomic<bool>& RunningFlag() { return running_; }
	static RtcpContext*& RtcpContextSlot() { return rtcpContext_; }

private:
	enum class LossCounterSource {
		None,
		LocalRtcp,
		ServerStats
	};

	struct VideoTrackRuntime {
		size_t index = 0;
		std::string trackId;
		std::string producerId;
		uint32_t ssrc = 0;
		uint8_t payloadType = 0;
		uint8_t transportCcExtensionId = 0;
		uint16_t seq = 0;
		double weight = 1.0;
		bool lossBaseInitialized = false;
		uint64_t lossBase = 0;
		LossCounterSource lossCounterSource = LossCounterSource::None;
		bool videoSuppressed = false;
		bool forceNextVideoKeyframe = false;
		double nextVideoEncodePtsSec = -1.0;
		int encBitrate = 900000;
		int configuredVideoFps = 25;
		double scaleResolutionDownBy = 1.0;
		std::optional<mediasoup::ffmpeg::Encoder> encoder;
		mediasoup::ffmpeg::FramePtr scaledFrame;
		SwsContext* swsCtx = nullptr;
		std::unique_ptr<qos::PublisherQosController> qosCtrl;
		bool snapshotRequested = false;
	};

	bool Initialize(int argc, char* argv[]);
	int Run();

	bool ParseArguments(int argc, char* argv[]);
	void InstallSignalHandlers();
	bool InitializeMediaBootstrap();
	bool InitializeSession();
	bool RecreateVideoEncoder(VideoTrackRuntime& track, int width, int height, int fps, int bitrate);
	void ConfigureQosControllers();
	void ConfigureNotifications();
	void RequestServerProducerStats();
	void StartTestHelperThreads();
	void StopTestHelperThreads();
	void Cleanup();

	int RunThreadedMode();
	int RunLegacyMode();

	static void OnSignal(int);
	static void SendH264ViaSharedPacketizer(int fd, const uint8_t* data, int size,
		uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq);
	static bool SendOpus(int fd, const uint8_t* data, int size,
		uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq);
	static int ResolveScaledDimension(int sourceDimension, double scaleDownBy);
	static int ResolveVideoFps(const AVStream* stream, int fallbackFps = 25);
	static std::optional<size_t> LoadOptionalTrackIndexEnv(const char* name, size_t maxTrackCount);
	static std::optional<qos::EncodingParameters> GetEncodingParametersForLevel(
		const qos::PublisherQosController& controller);
	static uint32_t GetTrackDesiredBitrateBps(const VideoTrackRuntime& track);
	static uint32_t GetTrackMinBitrateBps(const VideoTrackRuntime& track);

	static std::atomic<bool> running_;
	static RtcpContext* rtcpContext_;

	std::string serverIp_;
	int serverPort_{0};
	std::string roomId_;
	std::string peerId_;
	std::string mp4Path_;
	bool copyMode_{false};
	size_t videoTrackCount_{1};
	std::vector<double> videoTrackWeights_;
	std::optional<MatrixTestProfile> matrixTestProfile_;
	bool matrixLocalOnly_{false};
	std::vector<TestClientStatsPayloadEntry> testClientStatsPayloads_;
	std::vector<TestWsRequestEntry> testWsRequests_;
	std::optional<size_t> forcedStaleTrackIndex_;
	std::vector<std::string> videoSourcePaths_;
	MatrixTestRuntimeState matrixTestRuntime_;
	bool threadedRequested_{false};
	bool threadedMode_{false};
	bool threadedOwnsAllVideoInputs_{false};
	bool transportControllerEnabled_{true};

	std::optional<mediasoup::ffmpeg::InputFormat> inputFormat_;
	int vidIdx_{-1};
	int audIdx_{-1};
	std::optional<mediasoup::ffmpeg::BitstreamFilter> h264BitstreamFilter_;
	std::optional<mediasoup::ffmpeg::Decoder> audioDecoder_;
	std::optional<mediasoup::ffmpeg::Encoder> audioEncoder_;
	mediasoup::ffmpeg::FramePtr audioFrame_;
	std::optional<mediasoup::ffmpeg::Decoder> videoDecoder_;
	mediasoup::ffmpeg::FramePtr videoFrame_;
	int sourceVideoWidth_{0};
	int sourceVideoHeight_{0};
	std::vector<VideoTrackRuntime> videoTracks_;

	WsClient ws_;
	int udpFd_{-1};
	uint32_t audioSsrc_{22222222u};
	uint8_t audioPt_{0};
	uint8_t audioTransportCcExtensionId_{0};
	uint16_t audioSeq_{0};

	std::shared_ptr<CachedServerStatsResponse> cachedServerStatsResponse_;
	int64_t lastPeerQosSampleMs_{0};
	int peerSnapshotSeq_{0};

	std::thread testClientStatsThread_;
	std::thread testWsRequestsThread_;
};
