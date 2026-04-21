#include "PlainClientApp.h"

#include "NetworkThread.h"
#include "UdpSendHelpers.h"
#include "media/rtp/H264Packetizer.h"

extern "C" {
#include <libavutil/opt.h>
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
RtcpContext* PlainClientApp::rtcpContext_ = nullptr;

namespace {

class LegacyUdpPacketSink final : public mediasoup::media::rtp::H264PacketSink {
public:
	explicit LegacyUdpPacketSink(int fd)
		: fd_(fd)
	{
	}

	void OnPacket(const uint8_t* packet, size_t packetLen) override
	{
		const auto result = mediasoup::plainclient::SendUdpDatagram(fd_, packet, packetLen);
		if (result.status == mediasoup::plainclient::SendStatus::Sent &&
			PlainClientApp::RtcpContextSlot() &&
			packetLen >= 12)
			PlainClientApp::RtcpContextSlot()->onVideoRtpSent(packet, packetLen);
	}

private:
	int fd_{-1};
};

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

void PlainClientApp::SendH264ViaSharedPacketizer(
	int fd, const uint8_t* data, int size,
	uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq)
{
	LegacyUdpPacketSink sink(fd);
	mediasoup::media::rtp::H264Packetizer::PacketizeAnnexB(
		data,
		static_cast<size_t>(std::max(0, size)),
		pt,
		ts,
		ssrc,
		&seq,
		&sink);
}

bool PlainClientApp::SendOpus(
	int fd, const uint8_t* data, int size,
	uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq)
{
	if (size <= 0 || size > 4096) return false;
	uint8_t pkt[12 + 4096];
	rtpHeader(pkt, pt, seq++, ts, ssrc, true);
	std::memcpy(pkt + 12, data, size);
	const auto result = mediasoup::plainclient::SendUdpDatagram(fd, pkt, 12 + size);
	return result.status == mediasoup::plainclient::SendStatus::Sent;
}

int PlainClientApp::ResolveScaledDimension(int sourceDimension, double scaleDownBy)
{
	double safeScale = scaleDownBy >= 1.0 ? scaleDownBy : 1.0;
	int scaled = static_cast<int>(sourceDimension / safeScale);
	if (scaled < 2) scaled = 2;
	if (scaled % 2 != 0) scaled -= 1;
	return std::max(2, scaled);
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

std::optional<size_t> PlainClientApp::LoadOptionalTrackIndexEnv(const char* name, size_t maxTrackCount)
{
	const char* raw = std::getenv(name);
	if (!raw || std::strlen(raw) == 0) return std::nullopt;
	char* end = nullptr;
	long parsed = std::strtol(raw, &end, 10);
	if (!end || *end != '\0' || parsed < 0) return std::nullopt;
	size_t index = static_cast<size_t>(parsed);
	if (index >= maxTrackCount) return std::nullopt;
	return index;
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
	copyMode_ = (argc > 6 && std::string(argv[6]) == "--copy");

	videoTrackCount_ = loadVideoTrackCountFromEnv();
	videoTrackWeights_ = loadVideoTrackWeightsFromEnv(videoTrackCount_);
	matrixTestProfile_ = loadMatrixTestProfileFromEnv();
	matrixLocalOnly_ = envFlagEnabled("QOS_TEST_MATRIX_LOCAL_ONLY");
	testClientStatsPayloads_ = loadTestClientStatsPayloadsFromEnv();
	testWsRequests_ = loadTestWsRequestsFromEnv();
	forcedStaleTrackIndex_ =
		LoadOptionalTrackIndexEnv("QOS_TEST_FORCE_STALE_TRACK_INDEX", videoTrackCount_);
	videoSourcePaths_ = loadVideoSourcePathsFromEnv();
	matrixTestRuntime_.startMs = steadyNowMs();
	threadedRequested_ = envFlagEnabled("PLAIN_CLIENT_THREADED") && !copyMode_;
	const auto distinctSourceCount = videoSourcePaths_.size();
	threadedMode_ =
		threadedRequested_ &&
		(videoTrackCount_ == 1 || distinctSourceCount >= videoTrackCount_);
	transportControllerEnabled_ =
		envFlagDefaultTrue("PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER");
	if (threadedRequested_ && !threadedMode_) {
		std::printf(
			"WARN: PLAIN_CLIENT_THREADED ignored — need %zu distinct PLAIN_CLIENT_VIDEO_SOURCES (got %zu), using legacy path\n",
			videoTrackCount_,
			distinctSourceCount);
	}
	if (threadedMode_ && !transportControllerEnabled_) {
		std::printf(
			"[threaded] transport controller disabled via PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=0 (fallback pacing path)\n");
	}
	threadedOwnsAllVideoInputs_ = threadedMode_ && videoSourcePaths_.size() >= videoTrackCount_;
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

bool PlainClientApp::RecreateVideoEncoder(
	VideoTrackRuntime& track, int width, int height, int fps, int bitrate)
{
	const int safeFps = std::max(1, fps);
	try {
		auto newEncoder = msff::Encoder::Create(AV_CODEC_ID_H264, [&](AVCodecContext* ctx) {
			ctx->width = width;
			ctx->height = height;
			ctx->pix_fmt = AV_PIX_FMT_YUV420P;
			ctx->time_base = {1, safeFps};
			ctx->framerate = {safeFps, 1};
			ctx->bit_rate = bitrate;
			ctx->rc_max_rate = bitrate;
			ctx->rc_buffer_size = bitrate;
			ctx->gop_size = safeFps;
			ctx->max_b_frames = 0;
			av_opt_set(ctx->priv_data, "preset", "ultrafast", 0);
			av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
			av_opt_set(ctx->priv_data, "profile", "baseline", 0);
		});

		auto newScaledFrame = msff::MakeFrame();
		if (!newScaledFrame) return false;
		newScaledFrame->format = AV_PIX_FMT_YUV420P;
		newScaledFrame->width = width;
		newScaledFrame->height = height;
		msff::FrameGetBuffer(newScaledFrame.get(), 32);

		track.encoder = std::move(newEncoder);
		track.scaledFrame = std::move(newScaledFrame);
	} catch (...) {
		return false;
	}

	if (track.swsCtx) {
		sws_freeContext(track.swsCtx);
		track.swsCtx = nullptr;
	}
	track.configuredVideoFps = safeFps;
	return true;
}

bool PlainClientApp::InitializeMediaBootstrap()
{
	if (!threadedOwnsAllVideoInputs_) {
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

	if (inputFormat_ && vidIdx_ >= 0) {
		auto* videoStream = inputFormat_->StreamAt(vidIdx_);
		if (videoStream) {
			h264BitstreamFilter_ = msff::BitstreamFilter::Create(
				"h264_mp4toannexb",
				videoStream->codecpar,
				videoStream->time_base);
		}
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

	if (!copyMode_ && inputFormat_ && vidIdx_ >= 0) {
		auto* params = inputFormat_->StreamAt(vidIdx_)->codecpar;
		try {
			videoDecoder_ = msff::Decoder::OpenFromParameters(params);
			sourceVideoWidth_ = params->width;
			sourceVideoHeight_ = params->height;
			const int initialVideoFps = ResolveVideoFps(inputFormat_->StreamAt(vidIdx_), 25);
			for (auto& track : videoTracks_) {
				track.configuredVideoFps = initialVideoFps;
				if (!RecreateVideoEncoder(
						track,
						sourceVideoWidth_,
						sourceVideoHeight_,
						initialVideoFps,
						track.encBitrate)) {
					std::printf(
						"WARN: x264 encoder init failed for %s, falling back to copy mode\n",
						track.trackId.c_str());
					copyMode_ = true;
					break;
				}
			}
			videoFrame_ = msff::MakeFrame();
		} catch (const std::exception& e) {
			std::printf("WARN: video FFmpeg init failed, falling back to copy mode: %s\n", e.what());
			videoDecoder_.reset();
			videoFrame_.reset();
			copyMode_ = true;
		}
	}

	std::printf("Mode: %s\n", copyMode_ ? "copy (no QoS bitrate control)" : "re-encode (QoS enabled)");
	return true;
}

void PlainClientApp::ConfigureQosControllers()
{
	cachedServerStatsResponse_ = std::make_shared<CachedServerStatsResponse>();
	for (auto& track : videoTracks_) {
		if (copyMode_ || !track.encoder) continue;
		auto* trackPtr = &track;
		qos::PublisherQosController::Options qosOptions;
		qosOptions.source = qos::Source::Camera;
		qosOptions.trackId = track.trackId;
		qosOptions.producerId = track.producerId;
		qosOptions.initialLevel = 0;
		qosOptions.peerHasAudioTrack =
			(audIdx_ >= 0 && audioDecoder_.has_value() && audioEncoder_.has_value());
		qosOptions.actionSink =
			[this, trackPtr](const qos::PlannedAction& action) -> bool {
				auto& track = *trackPtr;
				if (action.type == qos::ActionType::SetEncodingParameters &&
					action.encodingParameters.has_value()) {
					auto& ep = *action.encodingParameters;
					const int nextBitrate = ep.maxBitrateBps.has_value()
						? static_cast<int>(*ep.maxBitrateBps)
						: track.encBitrate;
					const int nextFps = ep.maxFramerate.has_value()
						? std::max(1, static_cast<int>(*ep.maxFramerate))
						: track.configuredVideoFps;
					const double nextScale = ep.scaleResolutionDownBy.has_value()
						? std::max(1.0, *ep.scaleResolutionDownBy)
						: track.scaleResolutionDownBy;
					const int nextWidth = ResolveScaledDimension(sourceVideoWidth_, nextScale);
					const int nextHeight = ResolveScaledDimension(sourceVideoHeight_, nextScale);
					auto* encoderCtx = track.encoder ? track.encoder->get() : nullptr;
					const bool requiresRecreate = !track.encoder ||
						nextWidth != encoderCtx->width ||
						nextHeight != encoderCtx->height ||
						nextFps != track.configuredVideoFps;

					if (requiresRecreate) {
						if (!RecreateVideoEncoder(track, nextWidth, nextHeight, nextFps, nextBitrate)) {
							std::printf(
								"[QoS:%s] encoder reconfigure failed (br=%d fps=%d scale=%.2f size=%dx%d)\n",
								track.trackId.c_str(), nextBitrate, nextFps, nextScale, nextWidth, nextHeight);
							return false;
						}
						track.encBitrate = nextBitrate;
						track.scaleResolutionDownBy = nextScale;
						track.nextVideoEncodePtsSec = -1.0;
						track.forceNextVideoKeyframe = true;
					} else if (track.encoder) {
						track.encBitrate = nextBitrate;
						track.scaleResolutionDownBy = nextScale;
						track.nextVideoEncodePtsSec = -1.0;
						encoderCtx->bit_rate = track.encBitrate;
						encoderCtx->rc_max_rate = track.encBitrate;
						encoderCtx->rc_buffer_size = track.encBitrate;
					}
					std::printf("[QoS:%s] encoder params → br=%d fps=%d scale=%.2f size=%dx%d\n",
						track.trackId.c_str(),
						track.encBitrate,
						track.configuredVideoFps,
						track.scaleResolutionDownBy,
						encoderCtx ? encoderCtx->width : nextWidth,
						encoderCtx ? encoderCtx->height : nextHeight);
				} else if (action.type == qos::ActionType::SetMaxSpatialLayer) {
					std::printf("[QoS:%s] spatial layer request ignored in single-layer plain client\n",
						track.trackId.c_str());
				} else if (action.type == qos::ActionType::EnterAudioOnly ||
					action.type == qos::ActionType::PauseUpstream) {
					if (!track.videoSuppressed) {
						track.videoSuppressed = true;
						std::printf("[QoS:%s] video suppressed (%s)\n",
							track.trackId.c_str(), actionStr(action.type));
					}
				} else if (action.type == qos::ActionType::ExitAudioOnly ||
					action.type == qos::ActionType::ResumeUpstream) {
					if (track.videoSuppressed) {
						track.videoSuppressed = false;
						track.forceNextVideoKeyframe = true;
						track.nextVideoEncodePtsSec = -1.0;
						std::printf("[QoS:%s] video resumed (%s)\n",
							track.trackId.c_str(), actionStr(action.type));
					}
				}
				return true;
			};
		qosOptions.sendSnapshot =
			[this, trackPtr](const nlohmann::json&) {
				if (!testClientStatsPayloads_.empty()) return;
				if (matrixLocalOnly_) return;
				trackPtr->snapshotRequested = true;
			};
		track.qosCtrl = std::make_unique<qos::PublisherQosController>(qosOptions);
	}
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
		} else if (method == "qosOverride" && !matrixLocalOnly_) {
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

void PlainClientApp::RequestServerProducerStats()
{
	bool hasVideoQos = false;
	for (const auto& track : videoTracks_) {
		if (track.qosCtrl && !track.producerId.empty()) {
			hasVideoQos = true;
			break;
		}
	}
	if (!hasVideoQos || !cachedServerStatsResponse_) return;

	const int64_t now = steadyNowMs();
	{
		std::lock_guard<std::mutex> lock(cachedServerStatsResponse_->mutex);
		if (cachedServerStatsResponse_->requestInFlight ||
			now - cachedServerStatsResponse_->requestedAtSteadyMs < 500) {
			return;
		}
		cachedServerStatsResponse_->requestInFlight = true;
		cachedServerStatsResponse_->requestedAtSteadyMs = now;
	}

	bool queued = ws_.requestAsync("getStats", json::object(),
		[cached = cachedServerStatsResponse_](bool ok, const json& data, const std::string& error) {
			{
				std::lock_guard<std::mutex> lock(cached->mutex);
				cached->requestInFlight = false;
				if (ok && data.is_object()) {
					cached->latestPeerStats = data;
					cached->updatedAtSteadyMs = steadyNowMs();
				}
			}
			if (!ok) std::printf("[QoS] getStats failed: %s\n", error.c_str());
		});

	if (!queued) {
		std::lock_guard<std::mutex> lock(cachedServerStatsResponse_->mutex);
		cachedServerStatsResponse_->requestInFlight = false;
	}
}

void PlainClientApp::StartTestHelperThreads()
{
	if (!testClientStatsPayloads_.empty()) {
		testClientStatsThread_ = std::thread([this]() {
			for (const auto& entry : testClientStatsPayloads_) {
				if (!running_.load()) break;
				if (entry.delayMs > 0)
					std::this_thread::sleep_for(std::chrono::milliseconds(entry.delayMs));
				if (!running_.load()) break;
				ws_.requestAsync("clientStats", entry.payload,
					[](bool ok, const json&, const std::string& error) {
						if (!ok) std::printf("[QoS] test clientStats failed: %s\n", error.c_str());
					});
			}
		});
	}

	if (!testWsRequests_.empty()) {
		testWsRequestsThread_ = std::thread([this]() {
			for (const auto& entry : testWsRequests_) {
				if (!running_.load()) break;
				if (entry.delayMs > 0)
					std::this_thread::sleep_for(std::chrono::milliseconds(entry.delayMs));
				if (!running_.load()) break;
				ws_.requestAsync(entry.method, entry.data,
					[method = entry.method](bool ok, const json&, const std::string& error) {
						if (!ok) std::printf("[QoS] test %s failed: %s\n", method.c_str(), error.c_str());
					});
			}
		});
	}
}

void PlainClientApp::StopTestHelperThreads()
{
	if (testClientStatsThread_.joinable())
		testClientStatsThread_.join();
	if (testWsRequestsThread_.joinable())
		testWsRequestsThread_.join();
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
		videoTracks_[i].producerId = trackJson.value("producerId", i == 0 ? pub.value("videoProdId", "") : "");
	}

	ConfigureQosControllers();
	ConfigureNotifications();
	ws_.dispatchNotifications();
	RequestServerProducerStats();
	StartTestHelperThreads();
	return true;
}

void PlainClientApp::Cleanup()
{
	rtcpContext_ = nullptr;
	if (udpFd_ >= 0) {
		::close(udpFd_);
		udpFd_ = -1;
	}
	for (auto& track : videoTracks_) {
		if (track.swsCtx) {
			sws_freeContext(track.swsCtx);
			track.swsCtx = nullptr;
		}
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
		exitCode = threadedMode_ ? RunThreadedMode() : RunLegacyMode();
	} catch (const std::exception& e) {
		std::fprintf(stderr, "Error: %s\n", e.what());
		exitCode = 1;
	}

	StopTestHelperThreads();
	ws_.close();
	Cleanup();
	return exitCode;
}
