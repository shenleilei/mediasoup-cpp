#pragma once
// Per-peer recorder: receives RTP via UDP, muxes audio+video into WebM using libav API.
#include "Logger.h"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
}

namespace mediasoup {

// Parses minimal RTP header to extract PT, SSRC, sequence, timestamp
struct RtpHeader {
	uint8_t  payloadType;
	uint16_t sequenceNumber;
	uint32_t timestamp;
	uint32_t ssrc;
	const uint8_t* payload;
	int payloadSize;

	static bool parse(const uint8_t* data, int len, RtpHeader& h) {
		if (len < 12) return false;
		h.payloadType = data[1] & 0x7F;
		h.sequenceNumber = (data[2] << 8) | data[3];
		h.timestamp = (uint32_t(data[4]) << 24) | (uint32_t(data[5]) << 16) |
			(uint32_t(data[6]) << 8) | data[7];
		h.ssrc = (uint32_t(data[8]) << 24) | (uint32_t(data[9]) << 16) |
			(uint32_t(data[10]) << 8) | data[11];
		int cc = data[0] & 0x0F;
		int headerLen = 12 + cc * 4;
		// Extension
		if (data[0] & 0x10) {
			if (len < headerLen + 4) return false;
			int extLen = (data[headerLen + 2] << 8) | data[headerLen + 3];
			headerLen += 4 + extLen * 4;
		}
		if (headerLen > len) return false;
		h.payload = data + headerLen;
		h.payloadSize = len - headerLen;
		return true;
	}
};

class PeerRecorder {
public:
	PeerRecorder(const std::string& peerId, const std::string& outputPath,
		uint8_t audioPT, uint8_t videoPT, uint32_t audioClockRate, uint32_t videoClockRate)
		: peerId_(peerId), outputPath_(outputPath)
		, audioPT_(audioPT), videoPT_(videoPT)
		, audioClockRate_(audioClockRate), videoClockRate_(videoClockRate)
		, logger_(Logger::Get("Recorder"))
	{}

	~PeerRecorder() { stop(); }

	// Create a UDP socket, return the port it's bound to
	int createSocket() {
		sock_ = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock_ < 0) return -1;
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = 0; // OS picks port
		if (bind(sock_, (sockaddr*)&addr, sizeof(addr)) < 0) {
			::close(sock_); sock_ = -1; return -1;
		}
		socklen_t alen = sizeof(addr);
		getsockname(sock_, (sockaddr*)&addr, &alen);
		port_ = ntohs(addr.sin_port);
		return port_;
	}

	bool start() {
		if (sock_ < 0) return false;
		if (!initMuxer()) return false;
		running_ = true;
		thread_ = std::thread([this] { recvLoop(); });
		return true;
	}

	void stop() {
		running_ = false;
		if (sock_ >= 0) { ::close(sock_); sock_ = -1; }
		if (thread_.joinable()) thread_.join();
		finalizeMuxer();
	}

	int port() const { return port_; }
	const std::string& peerId() const { return peerId_; }

private:
	bool initMuxer() {
		av_log_set_level(AV_LOG_ERROR); // suppress verbose output
		avformat_alloc_output_context2(&fmtCtx_, nullptr, "webm", outputPath_.c_str());
		if (!fmtCtx_) return false;

		// Audio stream (Opus)
		audioStream_ = avformat_new_stream(fmtCtx_, nullptr);
		audioStream_->id = 0;
		audioStream_->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
		audioStream_->codecpar->codec_id = AV_CODEC_ID_OPUS;
		audioStream_->codecpar->sample_rate = 48000;
		audioStream_->codecpar->channels = 2;
		audioStream_->codecpar->channel_layout = AV_CH_LAYOUT_STEREO;
		audioStream_->time_base = {1, (int)audioClockRate_};
		// OpusHead required for WebM: version(1) channels(1) pre-skip(2) samplerate(4) gain(2) mapping(1)
		static const uint8_t opusHead[] = {
			'O','p','u','s','H','e','a','d',
			1,    // version
			2,    // channels
			0x38, 0x01, // pre-skip (312 = 0x0138 LE)
			0x80, 0xBB, 0x00, 0x00, // sample rate 48000 LE
			0x00, 0x00, // output gain
			0     // channel mapping family
		};
		audioStream_->codecpar->extradata = (uint8_t*)av_mallocz(sizeof(opusHead) + AV_INPUT_BUFFER_PADDING_SIZE);
		memcpy(audioStream_->codecpar->extradata, opusHead, sizeof(opusHead));
		audioStream_->codecpar->extradata_size = sizeof(opusHead);

		// Video stream (VP8)
		videoStream_ = avformat_new_stream(fmtCtx_, nullptr);
		videoStream_->id = 1;
		videoStream_->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
		videoStream_->codecpar->codec_id = AV_CODEC_ID_VP8;
		videoStream_->codecpar->width = 640;
		videoStream_->codecpar->height = 480;
		videoStream_->time_base = {1, (int)videoClockRate_};

		if (avio_open(&fmtCtx_->pb, outputPath_.c_str(), AVIO_FLAG_WRITE) < 0) {
			avformat_free_context(fmtCtx_); fmtCtx_ = nullptr;
			return false;
		}

		if (avformat_write_header(fmtCtx_, nullptr) < 0) {
			avio_closep(&fmtCtx_->pb);
			avformat_free_context(fmtCtx_); fmtCtx_ = nullptr;
			return false;
		}

		MS_DEBUG(logger_, "Recording started: {}", outputPath_);
		return true;
	}

	void finalizeMuxer() {
		std::lock_guard<std::mutex> lock(muxMutex_);
		if (!fmtCtx_) return;
		av_write_trailer(fmtCtx_);
		avio_closep(&fmtCtx_->pb);
		avformat_free_context(fmtCtx_);
		fmtCtx_ = nullptr;
		MS_DEBUG(logger_, "Recording finalized: {}", outputPath_);
	}

	void recvLoop() {
		uint8_t buf[65536];
		while (running_) {
			struct pollfd pfd = {sock_, POLLIN, 0};
			int ret = poll(&pfd, 1, 200); // 200ms timeout
			if (ret <= 0) continue;

			int n = recv(sock_, buf, sizeof(buf), 0);
			if (n <= 0) continue;

			RtpHeader rtp;
			if (!RtpHeader::parse(buf, n, rtp)) continue;

			writePacket(rtp);
		}
	}

	void writePacket(const RtpHeader& rtp) {
		std::lock_guard<std::mutex> lock(muxMutex_);
		if (!fmtCtx_) return;

		AVPacket pkt;
		av_init_packet(&pkt);

		int streamIdx;
		int64_t pts;

		if (rtp.payloadType == audioPT_) {
			streamIdx = audioStream_->index;
			if (!audioBaseSet_) { audioBaseTs_ = rtp.timestamp; audioBaseSet_ = true; }
			pts = rtp.timestamp - audioBaseTs_;
		} else if (rtp.payloadType == videoPT_) {
			streamIdx = videoStream_->index;
			if (!videoBaseSet_) { videoBaseTs_ = rtp.timestamp; videoBaseSet_ = true; }
			pts = rtp.timestamp - videoBaseTs_;
		} else {
			return;
		}

		pkt.stream_index = streamIdx;
		pkt.data = const_cast<uint8_t*>(rtp.payload);
		pkt.size = rtp.payloadSize;
		pkt.pts = pts;
		pkt.dts = pts;

		av_interleaved_write_frame(fmtCtx_, &pkt);
	}

	std::string peerId_;
	std::string outputPath_;
	uint8_t audioPT_, videoPT_;
	uint32_t audioClockRate_, videoClockRate_;
	int sock_ = -1;
	int port_ = 0;
	std::atomic<bool> running_{false};
	std::thread thread_;

	AVFormatContext* fmtCtx_ = nullptr;
	AVStream* audioStream_ = nullptr;
	AVStream* videoStream_ = nullptr;
	std::mutex muxMutex_;

	uint32_t audioBaseTs_ = 0, videoBaseTs_ = 0;
	bool audioBaseSet_ = false, videoBaseSet_ = false;

	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
