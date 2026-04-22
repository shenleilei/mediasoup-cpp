#pragma once

#include "PlainClientSupport.h"
#include "WsClient.h"
#include "ffmpeg/Decoder.h"
#include "ffmpeg/Encoder.h"
#include "ffmpeg/InputFormat.h"

extern "C" {
#include <libavformat/avformat.h>
}

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class PlainClientApp {
public:
	static int RunPlainClientApp(int argc, char* argv[]);
	static std::atomic<bool>& RunningFlag() { return running_; }

	friend class ThreadedQosRuntime;

private:
	struct VideoTrackRuntime {
		size_t index = 0;
		std::string trackId;
		std::string producerId;
		uint32_t ssrc = 0;
		uint8_t payloadType = 0;
		uint8_t transportCcExtensionId = 0;
		double weight = 1.0;
		bool videoSuppressed = false;
		int encBitrate = 900000;
		int configuredVideoFps = 25;
		double scaleResolutionDownBy = 1.0;
		std::unique_ptr<qos::PublisherQosController> qosCtrl;
		bool snapshotRequested = false;
	};

	bool Initialize(int argc, char* argv[]);
	int Run();

	bool ParseArguments(int argc, char* argv[]);
	void InstallSignalHandlers();
	bool InitializeMediaBootstrap();
	bool InitializeSession();
	void ConfigureNotifications();
	void Cleanup();

	int RunThreadedMode();

	static void OnSignal(int);
	static int ResolveVideoFps(const AVStream* stream, int fallbackFps = 25);
	static std::optional<qos::EncodingParameters> GetEncodingParametersForLevel(
		const qos::PublisherQosController& controller);
	static uint32_t GetTrackDesiredBitrateBps(const VideoTrackRuntime& track);
	static uint32_t GetTrackMinBitrateBps(const VideoTrackRuntime& track);

	static std::atomic<bool> running_;

	std::string serverIp_;
	int serverPort_{0};
	std::string roomId_;
	std::string peerId_;
	std::string mp4Path_;
	size_t videoTrackCount_{1};
	std::vector<double> videoTrackWeights_;
	std::vector<std::string> videoSourcePaths_;
	bool hasExplicitVideoSourcesForAllTracks_{false};
	bool transportControllerEnabled_{true};
	bool transportEstimateEnabled_{true};

	std::optional<mediasoup::ffmpeg::InputFormat> inputFormat_;
	int vidIdx_{-1};
	int audIdx_{-1};
	std::optional<mediasoup::ffmpeg::Decoder> audioDecoder_;
	std::optional<mediasoup::ffmpeg::Encoder> audioEncoder_;
	mediasoup::ffmpeg::FramePtr audioFrame_;
	std::vector<VideoTrackRuntime> videoTracks_;

	WsClient ws_;
	int udpFd_{-1};
	uint32_t audioSsrc_{22222222u};
	uint8_t audioPt_{0};
	uint8_t audioTransportCcExtensionId_{0};
};
