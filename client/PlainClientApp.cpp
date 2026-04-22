#include "PlainClientApp.h"

extern "C" {
}

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <set>

namespace msff = mediasoup::ffmpeg;

std::atomic<bool> PlainClientApp::running_{true};

namespace {

bool envFlagDefaultTrue(const char* name)
{
	const char* raw = std::getenv(name);
	if (!raw || std::strlen(raw) == 0) return true;
	std::string value(raw);
	std::transform(value.begin(), value.end(), value.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	if (value == "0" || value == "false" || value == "no" || value == "off")
		return false;
	if (value == "1" || value == "true" || value == "yes" || value == "on")
		return true;
	return true;
}

} // namespace

int PlainClientApp::RunPlainClientApp(int argc, char* argv[])
{
	PlainClientApp app;
	if (!app.Initialize(argc, argv))
		return 1;
	return app.Run();
}

void PlainClientApp::OnSignal(int)
{
	running_.store(false, std::memory_order_relaxed);
}

int PlainClientApp::ResolveVideoFps(const AVStream* stream, int fallbackFps)
{
	if (!stream) return fallbackFps;
	const AVRational preferredRates[] = {stream->avg_frame_rate, stream->r_frame_rate};
	for (const auto& rate : preferredRates) {
		if (rate.num > 0 && rate.den > 0) {
			const double fps = av_q2d(rate);
			if (fps > 0.0) return std::max(1, static_cast<int>(fps + 0.5));
		}
	}
	return fallbackFps;
}

std::optional<qos::EncodingParameters> PlainClientApp::GetEncodingParametersForLevel(
	const qos::PublisherQosController& controller)
{
	const auto& profile = controller.profileConfig();
	for (const auto& step : profile.ladder) {
		if (step.level == controller.currentLevel()) return step.encodingParameters;
	}
	return std::nullopt;
}

uint32_t PlainClientApp::GetTrackDesiredBitrateBps(const VideoTrackRuntime& track)
{
	if (!track.qosCtrl) return track.encBitrate > 0 ? static_cast<uint32_t>(track.encBitrate) : 0;
	auto encodingParameters = GetEncodingParametersForLevel(*track.qosCtrl);
	if (encodingParameters.has_value() && encodingParameters->maxBitrateBps.has_value())
		return *encodingParameters->maxBitrateBps;
	return track.encBitrate > 0 ? static_cast<uint32_t>(track.encBitrate) : 0;
}

uint32_t PlainClientApp::GetTrackMinBitrateBps(const VideoTrackRuntime& track)
{
	const uint32_t desiredBitrateBps = GetTrackDesiredBitrateBps(track);
	if (desiredBitrateBps == 0) return 0;
	return std::min<uint32_t>(desiredBitrateBps, 120000u);
}

bool PlainClientApp::ParseArguments(int argc, char* argv[])
{
	if (argc < 6) {
		std::fprintf(stderr, "Usage: %s SERVER_IP SERVER_PORT ROOM PEER file.mp4\n", argv[0]);
		return false;
	}

	serverIp_ = argv[1];
	serverPort_ = std::atoi(argv[2]);
	roomId_ = argv[3];
	peerId_ = argv[4];
	mp4Path_ = argv[5];
	if (argc != 6) {
		std::fprintf(stderr, "plain-client no longer supports --copy; use the threaded runtime only\n");
		return false;
	}

	videoTrackCount_ = loadVideoTrackCountFromEnv();
	videoTrackWeights_ = loadVideoTrackWeightsFromEnv(videoTrackCount_);
	videoSourcePaths_ = loadVideoSourcePathsFromEnv();
	const auto distinctSourceCount = videoSourcePaths_.size();
	transportControllerEnabled_ =
		envFlagDefaultTrue("PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER");
	transportEstimateEnabled_ =
		envFlagDefaultTrue("PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE");
	if (!transportControllerEnabled_) {
		std::printf(
			"[threaded] transport controller disabled via PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=0 (fallback pacing path)\n");
	} else if (!transportEstimateEnabled_) {
		std::printf(
			"[threaded] transport estimate disabled via PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE=0 (controller still enabled)\n");
	}
	hasExplicitVideoSourcesForAllTracks_ = distinctSourceCount >= videoTrackCount_;
	return true;
}

void PlainClientApp::InstallSignalHandlers()
{
	struct sigaction sigintAction {};
	sigemptyset(&sigintAction.sa_mask);
	sigintAction.sa_handler = PlainClientApp::OnSignal;
	sigintAction.sa_flags = 0;
	sigaction(SIGINT, &sigintAction, nullptr);

	struct sigaction sigpipeAction {};
	sigemptyset(&sigpipeAction.sa_mask);
	sigpipeAction.sa_handler = SIG_IGN;
	sigpipeAction.sa_flags = 0;
	sigaction(SIGPIPE, &sigpipeAction, nullptr);
}

bool PlainClientApp::InitializeMediaBootstrap()
{
	const bool canSkipBootstrap = hasExplicitVideoSourcesForAllTracks_;
	if (!canSkipBootstrap) {
		try {
			inputFormat_ = msff::InputFormat::Open(mp4Path_);
			inputFormat_->FindStreamInfo();
		} catch (const std::exception& e) {
			std::fprintf(stderr, "Cannot open %s: %s\n", mp4Path_.c_str(), e.what());
			return false;
		}
		vidIdx_ = inputFormat_->FindFirstStreamIndex(AVMEDIA_TYPE_VIDEO);
		audIdx_ = inputFormat_->FindFirstStreamIndex(AVMEDIA_TYPE_AUDIO);
	} else {
		std::printf("[threaded] skipping MP4 bootstrap for %zu explicit video source(s)\n",
			videoSourcePaths_.size());
	}
	std::printf("MP4: video=%d audio=%d\n", vidIdx_, audIdx_);

	if (!hasExplicitVideoSourcesForAllTracks_ && vidIdx_ < 0) {
		std::fprintf(stderr, "Input %s does not contain a video stream\n", mp4Path_.c_str());
		return false;
	}

	if (inputFormat_ && audIdx_ >= 0) {
		auto* params = inputFormat_->StreamAt(audIdx_)->codecpar;
		try {
			audioDecoder_ = msff::Decoder::OpenFromParameters(params);
			audioEncoder_ = msff::Encoder::Create(AV_CODEC_ID_OPUS, [](AVCodecContext* ctx) {
				ctx->sample_rate = 48000;
				ctx->channels = 2;
				ctx->channel_layout = AV_CH_LAYOUT_STEREO;
				ctx->bit_rate = 64000;
				ctx->sample_fmt = AV_SAMPLE_FMT_FLT;
			});
			audioFrame_ = msff::MakeFrame();
		} catch (const std::exception& e) {
			audioDecoder_.reset();
			audioEncoder_.reset();
			audioFrame_.reset();
			std::printf("WARN: audio FFmpeg init failed, audio disabled: %s\n", e.what());
		}
	}

	videoTracks_.resize(videoTrackCount_);
	for (size_t i = 0; i < videoTrackCount_; ++i) {
		videoTracks_[i].index = i;
		videoTracks_[i].trackId = i == 0 ? "video" : ("video-" + std::to_string(i));
		videoTracks_[i].weight = i < videoTrackWeights_.size() ? videoTrackWeights_[i] : 1.0;
	}

	if (inputFormat_ && vidIdx_ >= 0) {
		const int initialVideoFps = ResolveVideoFps(inputFormat_->StreamAt(vidIdx_), 25);
		for (auto& track : videoTracks_) {
			track.configuredVideoFps = initialVideoFps;
		}
	}

	std::printf("Mode: threaded (sender QoS enabled)\n");
	return true;
}

void PlainClientApp::ConfigureNotifications()
{
	ws_.setNotificationHandler([this](const json& msg) {
		std::string method = msg.value("method", "");
		auto data = msg.value("data", json::object());
		if (method == "qosPolicy") {
			try {
				auto policy = qos::parseQosPolicy(data);
				for (auto& track : videoTracks_)
					if (track.qosCtrl) track.qosCtrl->handlePolicy(policy);
				std::printf("[QoS] policy updated: sampleMs=%d snapshotMs=%d\n",
					policy.sampleIntervalMs, policy.snapshotIntervalMs);
			} catch (const std::exception& e) {
				std::printf("[QoS] policy parse error: %s\n", e.what());
			}
		} else if (method == "qosOverride") {
			try {
				auto override = qos::parseQosOverride(data);
				for (auto& track : videoTracks_)
					if (track.qosCtrl) track.qosCtrl->handleOverride(override);
				std::printf("[QoS] override: reason=%s ttl=%d\n",
					override.reason.c_str(), override.ttlMs);
			} catch (const std::exception& e) {
				std::printf("[QoS] override parse error: %s\n", e.what());
			}
		}
	});
}

bool PlainClientApp::InitializeSession()
{
	if (!ws_.connect(serverIp_, serverPort_, "/ws")) {
		std::fprintf(stderr, "WS connect failed\n");
		return false;
	}
	std::printf("WS connected to %s:%d\n", serverIp_.c_str(), serverPort_);

	ws_.request("join", {
		{"roomId", roomId_},
		{"peerId", peerId_},
		{"displayName", peerId_},
		{"rtpCapabilities", json::object()}
	});
	std::printf("Joined room=%s peer=%s\n", roomId_.c_str(), peerId_.c_str());

	std::vector<uint32_t> videoSsrcs(videoTracks_.size(), 0);
	for (size_t i = 0; i < videoTracks_.size(); ++i)
		videoSsrcs[i] = static_cast<uint32_t>(11111111u + (i * 1111u));

	json plainPublishRequest{{"audioSsrc", audioSsrc_}};
	if (videoSsrcs.size() == 1) plainPublishRequest["videoSsrc"] = videoSsrcs.front();
	else plainPublishRequest["videoSsrcs"] = videoSsrcs;
	auto pub = ws_.request("plainPublish", plainPublishRequest);

	const std::string announcedIp = pub["ip"];
	const uint16_t serverPort = pub["port"];
	const uint8_t defaultVideoPt = pub["videoPt"];
	audioPt_ = pub["audioPt"];
	const uint8_t defaultVideoTransportCcExtensionId =
		static_cast<uint8_t>(pub.value("videoTransportCcExtId", 0));
	audioTransportCcExtensionId_ =
		static_cast<uint8_t>(pub.value("audioTransportCcExtId", 0));
	std::printf("Publish → %s:%d (announced=%s) videoTracks=%zu videoPt=%d aPt=%d\n",
		serverIp_.c_str(),
		serverPort,
		announcedIp.c_str(),
		videoTracks_.size(),
		defaultVideoPt,
		audioPt_);

	udpFd_ = socket(AF_INET, SOCK_DGRAM, 0);
	sockaddr_in dst{};
	dst.sin_family = AF_INET;
	dst.sin_port = htons(serverPort);
	inet_pton(AF_INET, serverIp_.c_str(), &dst.sin_addr);
	::connect(udpFd_, reinterpret_cast<sockaddr*>(&dst), sizeof(dst));

	{
		int flags = fcntl(udpFd_, F_GETFL, 0);
		fcntl(udpFd_, F_SETFL, flags | O_NONBLOCK);
	}

	auto videoTracksJson = pub.value("videoTracks", json::array());
	for (size_t i = 0; i < videoTracks_.size(); ++i) {
		const auto& trackJson = (videoTracksJson.is_array() && i < videoTracksJson.size())
			? videoTracksJson.at(i)
			: json::object();
		videoTracks_[i].ssrc = trackJson.value("ssrc", videoSsrcs[i]);
		videoTracks_[i].payloadType = static_cast<uint8_t>(trackJson.value("pt", defaultVideoPt));
		videoTracks_[i].transportCcExtensionId = static_cast<uint8_t>(
			trackJson.value("transportCcExtId", defaultVideoTransportCcExtensionId));
		videoTracks_[i].producerId = trackJson.value("producerId", i == 0 ? pub.value("videoProdId", "") : "");
	}

	ConfigureNotifications();
	return true;
}

void PlainClientApp::Cleanup()
{
	if (udpFd_ >= 0) {
		::close(udpFd_);
		udpFd_ = -1;
	}
}

bool PlainClientApp::Initialize(int argc, char* argv[])
{
	running_.store(true, std::memory_order_relaxed);
	if (!ParseArguments(argc, argv))
		return false;
	InstallSignalHandlers();
	return InitializeMediaBootstrap();
}

int PlainClientApp::Run()
{
	int exitCode = 0;
	try {
		if (!InitializeSession())
			return 1;
		exitCode = RunThreadedMode();
	} catch (const std::exception& e) {
		std::fprintf(stderr, "Error: %s\n", e.what());
		exitCode = 1;
	}

	ws_.close();
	Cleanup();
	return exitCode;
}
