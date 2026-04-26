#pragma once

#include "Constants.h"
#include "Logger.h"
#include "ffmpeg/OutputFormat.h"
#include "media/rtp/RtpHeader.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

struct AVStream;
struct SwsContext;

namespace mediasoup {

using json = nlohmann::json;
namespace msff = mediasoup::ffmpeg;

enum class VideoCodec { VP8, H264 };

class PeerRecorder {
	friend class PeerRecorderTestAccess;

public:
	PeerRecorder(const std::string& peerId, const std::string& outputPath,
		uint8_t audioPT, uint8_t videoPT, uint32_t audioClockRate, uint32_t videoClockRate,
		VideoCodec videoCodec = VideoCodec::VP8, const std::string& roomId = "");
	~PeerRecorder();

	int createSocket();
	bool start();
	void stop();

	int port() const { return port_; }
	const std::string& peerId() const { return peerId_; }
	const std::string& outputPath() const { return outputPath_; }

	void appendQosSnapshot(const json& stats);

private:
	bool initMuxer();
	bool setH264Extradata();
	bool commitDeferredHeader();
	void finalizeMuxer();
	void recvLoop();

	static const uint8_t* stripVp8Descriptor(const uint8_t* data, int size,
		int& outSize, bool& isStart);
	static uint64_t unwrapTimestamp(uint32_t ts, uint32_t baseTs, uint32_t& lastTs, uint64_t& wrapCount);
	void depacketizeH264(const media::rtp::RtpHeader& rtp);
	void writePacket(const media::rtp::RtpHeader& rtp);
	void writeAudioPacket(uint32_t ts, const uint8_t* data, int size);
	void flushVideoFrame();

private:
	std::string peerId_;
	std::string roomId_;
	std::string logTag_;
	std::string outputPath_;
	uint8_t audioPT_;
	uint8_t videoPT_;
	uint32_t audioClockRate_;
	uint32_t videoClockRate_;
	VideoCodec videoCodec_;
	std::atomic<int> sock_{-1};
	int port_{0};
	std::atomic<bool> running_{false};
	std::thread thread_;

	std::optional<msff::OutputFormat> outputFormat_;
	AVStream* audioStream_{nullptr};
	AVStream* videoStream_{nullptr};
	std::mutex muxMutex_;
	bool headerDeferred_{false};
	std::vector<std::pair<uint32_t, std::vector<uint8_t>>> pendingAudio_;

	uint32_t audioBaseTs_{0};
	uint32_t videoBaseTs_{0};
	uint32_t audioLastTs_{0};
	uint32_t videoLastTs_{0};
	uint64_t audioWrapCount_{0};
	uint64_t videoWrapCount_{0};
	bool audioBaseSet_{false};
	bool videoBaseSet_{false};
	std::vector<uint8_t> videoFrameBuf_;
	uint32_t videoFrameTs_{0};
	bool videoKeyframe_{false};
	int h264NalLog_{0};
	std::vector<uint8_t> h264Sps_;
	std::vector<uint8_t> h264Pps_;

	std::shared_ptr<spdlog::logger> logger_;
	std::chrono::steady_clock::time_point startTime_;
	std::atomic<int64_t> firstRtpTimeNs_{-1};
	std::mutex qosMutex_;
	std::ofstream qosFile_;
	bool qosFirst_{true};
};

} // namespace mediasoup
