#pragma once
#include "Logger.h"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>
#include <chrono>
#include <cmath>
#include <nlohmann/json.hpp>
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

using json = nlohmann::json;

struct RtpHeader {
	uint8_t  payloadType;
	uint16_t sequenceNumber;
	uint32_t timestamp;
	uint32_t ssrc;
	bool     marker;
	const uint8_t* payload;
	int payloadSize;

	static bool parse(const uint8_t* data, int len, RtpHeader& h) {
		if (len < 12) return false;
		h.marker = (data[1] & 0x80) != 0;
		h.payloadType = data[1] & 0x7F;
		h.sequenceNumber = (data[2] << 8) | data[3];
		h.timestamp = (uint32_t(data[4]) << 24) | (uint32_t(data[5]) << 16) |
			(uint32_t(data[6]) << 8) | data[7];
		h.ssrc = (uint32_t(data[8]) << 24) | (uint32_t(data[9]) << 16) |
			(uint32_t(data[10]) << 8) | data[11];
		int cc = data[0] & 0x0F;
		int headerLen = 12 + cc * 4;
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

enum class VideoCodec { VP8, H264 };

class PeerRecorder {
public:
	PeerRecorder(const std::string& peerId, const std::string& outputPath,
		uint8_t audioPT, uint8_t videoPT, uint32_t audioClockRate, uint32_t videoClockRate,
		VideoCodec videoCodec = VideoCodec::VP8, const std::string& roomId = "")
		: peerId_(peerId), roomId_(roomId), outputPath_(outputPath)
		, audioPT_(audioPT), videoPT_(videoPT)
		, audioClockRate_(audioClockRate), videoClockRate_(videoClockRate)
		, videoCodec_(videoCodec)
		, logTag_("[room:" + roomId + " " + peerId + "]")
		, logger_(Logger::Get("Recorder"))
	{}

	~PeerRecorder() { stop(); }

	int createSocket() {
		sock_ = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock_ < 0) return -1;
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = 0;
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
		startTime_ = std::chrono::steady_clock::now();
		running_ = true;
		thread_ = std::thread([this] { recvLoop(); });
		return true;
	}

	void stop() {
		running_ = false;
		if (sock_ >= 0) { ::close(sock_); sock_ = -1; }
		if (thread_.joinable()) thread_.join();
		finalizeMuxer();
		{
			std::lock_guard<std::mutex> lock(qosMutex_);
			if (qosFile_.is_open()) {
				qosFile_ << "\n]"; qosFile_.close();
			} else if (!outputPath_.empty()) {
				std::string qosPath = outputPath_.substr(0, outputPath_.rfind('.')) + ".qos.json";
				std::ofstream f(qosPath);
				if (f.is_open()) { f << "[]"; f.close(); }
			}
			outputPath_.clear();
		}
	}

	int port() const { return port_; }
	const std::string& peerId() const { return peerId_; }
	const std::string& outputPath() const { return outputPath_; }

	void appendQosSnapshot(const json& stats) {
		if (!running_ || outputPath_.empty()) return;
		std::lock_guard<std::mutex> lock(qosMutex_);
		if (!qosFile_.is_open()) {
			std::string qosPath = outputPath_.substr(0, outputPath_.rfind('.')) + ".qos.json";
			qosFile_.open(qosPath, std::ios::out | std::ios::trunc);
			if (!qosFile_.is_open()) return;
			qosFile_ << "[\n";
			qosFirst_ = true;
		}
		auto baseTime = firstRtpReceived_ ? firstRtpTime_ : startTime_;
		auto elapsed = std::chrono::steady_clock::now() - baseTime;
		double secs = std::chrono::duration<double>(elapsed).count();
		if (secs < 0) secs = 0;
		if (!qosFirst_) qosFile_ << ",\n";
		qosFirst_ = false;
		json entry = {{"t", std::round(secs * 10.0) / 10.0}, {"stats", stats}};
		qosFile_ << entry.dump();
		qosFile_.flush();
	}

private:
	bool initMuxer() {
		av_log_set_level(AV_LOG_ERROR);

		const char* fmt = (videoCodec_ == VideoCodec::H264) ? "matroska" : "webm";
		avformat_alloc_output_context2(&fmtCtx_, nullptr, fmt, outputPath_.c_str());
		if (!fmtCtx_) return false;

		// Audio: Opus
		audioStream_ = avformat_new_stream(fmtCtx_, nullptr);
		audioStream_->id = 0;
		audioStream_->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
		audioStream_->codecpar->codec_id = AV_CODEC_ID_OPUS;
		audioStream_->codecpar->sample_rate = 48000;
		audioStream_->codecpar->channels = 2;
		audioStream_->codecpar->channel_layout = AV_CH_LAYOUT_STEREO;
		audioStream_->time_base = {1, (int)audioClockRate_};
		static const uint8_t opusHead[] = {
			'O','p','u','s','H','e','a','d', 1, 2,
			0x38, 0x01, 0x80, 0xBB, 0x00, 0x00, 0x00, 0x00, 0
		};
		audioStream_->codecpar->extradata = (uint8_t*)av_mallocz(sizeof(opusHead) + AV_INPUT_BUFFER_PADDING_SIZE);
		memcpy(audioStream_->codecpar->extradata, opusHead, sizeof(opusHead));
		audioStream_->codecpar->extradata_size = sizeof(opusHead);

		// Video
		videoStream_ = avformat_new_stream(fmtCtx_, nullptr);
		videoStream_->id = 1;
		videoStream_->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
		videoStream_->codecpar->codec_id = (videoCodec_ == VideoCodec::H264)
			? AV_CODEC_ID_H264 : AV_CODEC_ID_VP8;
		videoStream_->codecpar->width = 640;
		videoStream_->codecpar->height = 480;
		videoStream_->time_base = {1, (int)videoClockRate_};

		if (avio_open(&fmtCtx_->pb, outputPath_.c_str(), AVIO_FLAG_WRITE) < 0) {
			avformat_free_context(fmtCtx_); fmtCtx_ = nullptr; return false;
		}

		// H264: defer write_header until we have SPS/PPS extradata from first IDR
		if (videoCodec_ == VideoCodec::H264) {
			headerDeferred_ = true;
			MS_DEBUG(logger_, "{} Recording deferred (waiting for H264 IDR): {}", logTag_, outputPath_);
		} else {
			if (avformat_write_header(fmtCtx_, nullptr) < 0) {
				avio_closep(&fmtCtx_->pb);
				avformat_free_context(fmtCtx_); fmtCtx_ = nullptr; return false;
			}
			MS_DEBUG(logger_, "{} Recording started: {} (VP8)", logTag_, outputPath_);
		}
		return true;
	}

	// Build AVCC extradata from cached SPS/PPS
	bool setH264Extradata() {
		if (h264Sps_.empty() || h264Pps_.empty()) return false;
		int spsLen = h264Sps_.size(), ppsLen = h264Pps_.size();
		int edSize = 6 + 2 + spsLen + 1 + 2 + ppsLen;
		uint8_t* ed = (uint8_t*)av_mallocz(edSize + AV_INPUT_BUFFER_PADDING_SIZE);
		ed[0] = 1;
		ed[1] = h264Sps_[1]; ed[2] = h264Sps_[2]; ed[3] = h264Sps_[3];
		ed[4] = 0xFF; ed[5] = 0xE1;
		ed[6] = (spsLen >> 8); ed[7] = spsLen & 0xFF;
		memcpy(ed + 8, h264Sps_.data(), spsLen);
		int off = 8 + spsLen;
		ed[off] = 1;
		ed[off+1] = (ppsLen >> 8); ed[off+2] = ppsLen & 0xFF;
		memcpy(ed + off + 3, h264Pps_.data(), ppsLen);
		videoStream_->codecpar->extradata = ed;
		videoStream_->codecpar->extradata_size = edSize;
		return true;
	}

	bool commitDeferredHeader() {
		if (avformat_write_header(fmtCtx_, nullptr) < 0) {
			MS_ERROR(logger_, "{} avformat_write_header failed for H264: {}", logTag_, outputPath_);
			avio_closep(&fmtCtx_->pb);
			avformat_free_context(fmtCtx_); fmtCtx_ = nullptr;
			return false;
		}
		headerDeferred_ = false;
		MS_DEBUG(logger_, "{} Recording started: {} (H264, header written)", logTag_, outputPath_);
		return true;
	}

	void finalizeMuxer() {
		std::lock_guard<std::mutex> lock(muxMutex_);
		if (!fmtCtx_) return;
		flushVideoFrame();
		if (!headerDeferred_) {
			av_write_trailer(fmtCtx_);
		}
		avio_closep(&fmtCtx_->pb);
		avformat_free_context(fmtCtx_);
		fmtCtx_ = nullptr;
		MS_DEBUG(logger_, "{} Recording finalized: {}", logTag_, outputPath_);
	}

	void recvLoop() {
		uint8_t buf[65536];
		int pktCount = 0;
		while (running_) {
			struct pollfd pfd = {sock_, POLLIN, 0};
			int ret = poll(&pfd, 1, 200);
			if (ret <= 0) continue;
			int n = recv(sock_, buf, sizeof(buf), 0);
			if (n <= 0) continue;
			pktCount++;
			RtpHeader rtp;
			if (!RtpHeader::parse(buf, n, rtp)) continue;
			if (pktCount <= 10 || pktCount % 500 == 0)
				MS_DEBUG(logger_, "{} RTP recv #{} size={} PT={} (audio={} video={})", logTag_, pktCount, n, rtp.payloadType, audioPT_, videoPT_);
			writePacket(rtp);
		}
		MS_DEBUG(logger_, "{} recvLoop exited, total packets={}", logTag_, pktCount);
	}

	// ── VP8 depacketization (public for testing) ──
public:
	static const uint8_t* stripVp8Descriptor(const uint8_t* data, int size,
		int& outSize, bool& isStart)
	{
		if (size < 1) { outSize = 0; isStart = false; return data; }
		isStart = (data[0] & 0x10) != 0; // S bit
		int skip = 1;
		if (data[0] & 0x80) { // X
			if (size < 2) { outSize = 0; return data; }
			skip++;
			uint8_t x = data[1];
			if (x & 0x80) { skip++; if (size > skip && data[skip-1] & 0x80) skip++; } // I
			if (x & 0x40) skip++; // L
			if (x & 0x20 || x & 0x10) skip++; // T/K
		}
		if (skip >= size) { outSize = 0; return data; }
		outSize = size - skip;
		return data + skip;
	}

	// ── H264 depacketization (RFC 6184) ──
	void depacketizeH264(const RtpHeader& rtp) {
		const uint8_t* data = rtp.payload;
		int size = rtp.payloadSize;
		if (size < 1) return;

		uint8_t nalType = data[0] & 0x1F;

		if (h264NalLog_ < 20) {
			h264NalLog_++;
			if (nalType == 28 && size >= 2) {
				uint8_t fuNalType = data[1] & 0x1F;
				bool fuStart = (data[1] & 0x80) != 0;
				MS_DEBUG(logger_, "{} H264 RTP: nalType={} FU-A fuNalType={} fuStart={} size={} marker={}",
					peerId_, nalType, fuNalType, fuStart, size, rtp.marker);
			} else {
				MS_DEBUG(logger_, "{} H264 RTP: nalType={} size={} marker={}", logTag_, nalType, size, rtp.marker);
			}
		}

		if (nalType >= 1 && nalType <= 23) {
			// Single NAL unit
			if (nalType == 7) { h264Sps_.assign(data, data + size); return; }
			if (nalType == 8) { h264Pps_.assign(data, data + size); return; }
			bool isIdr = (nalType == 5);
			if (videoFrameBuf_.empty()) { videoKeyframe_ = isIdr; videoFrameTs_ = rtp.timestamp; }
			static const uint8_t sc[] = {0,0,0,1};
			videoFrameBuf_.insert(videoFrameBuf_.end(), sc, sc + 4);
			videoFrameBuf_.insert(videoFrameBuf_.end(), data, data + size);
			if (rtp.marker) flushVideoFrame();
		} else if (nalType == 28) {
			// FU-A
			if (size < 2) return;
			uint8_t fuHeader = data[1];
			bool fuStart = (fuHeader & 0x80) != 0;
			uint8_t fuNalType = fuHeader & 0x1F;

			if (fuStart) {
				if (!videoFrameBuf_.empty()) flushVideoFrame();
				videoKeyframe_ = (fuNalType == 5);
				videoFrameTs_ = rtp.timestamp;
				uint8_t nalHeader = (data[0] & 0xE0) | fuNalType;
				static const uint8_t sc[] = {0,0,0,1};
				videoFrameBuf_.insert(videoFrameBuf_.end(), sc, sc + 4);
				videoFrameBuf_.push_back(nalHeader);
			}
			videoFrameBuf_.insert(videoFrameBuf_.end(), data + 2, data + size);
			if ((fuHeader & 0x40) || rtp.marker) flushVideoFrame();
		} else if (nalType == 24) {
			// STAP-A: extract SPS/PPS, pass other NALs to frame buffer
			int offset = 1;
			while (offset + 2 <= size) {
				uint16_t nalSize = (data[offset] << 8) | data[offset + 1];
				offset += 2;
				if (offset + nalSize > size) break;
				uint8_t nt = data[offset] & 0x1F;
				if (nt == 7) { h264Sps_.assign(data + offset, data + offset + nalSize); }
				else if (nt == 8) { h264Pps_.assign(data + offset, data + offset + nalSize); }
				else {
					if (nt == 5) videoKeyframe_ = true;
					if (videoFrameBuf_.empty()) videoFrameTs_ = rtp.timestamp;
					static const uint8_t sc[] = {0,0,0,1};
					videoFrameBuf_.insert(videoFrameBuf_.end(), sc, sc + 4);
					videoFrameBuf_.insert(videoFrameBuf_.end(), data + offset, data + offset + nalSize);
				}
				offset += nalSize;
			}
			if (rtp.marker && !videoFrameBuf_.empty()) flushVideoFrame();
		}
	}

	void writePacket(const RtpHeader& rtp) {
		std::lock_guard<std::mutex> lock(muxMutex_);
		if (!fmtCtx_) return;

		if (!firstRtpReceived_) {
			firstRtpTime_ = std::chrono::steady_clock::now();
			firstRtpReceived_ = true;
		}

		if (rtp.payloadType == audioPT_) {
			if (!audioBaseSet_) { audioBaseTs_ = rtp.timestamp; audioBaseSet_ = true; }
			if (headerDeferred_) {
				// Buffer audio until header is written
				pendingAudio_.push_back({rtp.timestamp,
					std::vector<uint8_t>(rtp.payload, rtp.payload + rtp.payloadSize)});
				return;
			}
			writeAudioPacket(rtp.timestamp, rtp.payload, rtp.payloadSize);
		} else if (rtp.payloadType == videoPT_) {
			if (!videoBaseSet_) { videoBaseTs_ = rtp.timestamp; videoBaseSet_ = true; }

			if (videoCodec_ == VideoCodec::H264) {
				depacketizeH264(rtp);
			} else {
				// VP8
				bool isStart = false;
				int stripped = 0;
				const uint8_t* vp8data = stripVp8Descriptor(rtp.payload, rtp.payloadSize, stripped, isStart);
				if (stripped <= 0) return;

				if (isStart && !videoFrameBuf_.empty()) flushVideoFrame();
				if (isStart) {
					videoFrameTs_ = rtp.timestamp;
					videoKeyframe_ = (stripped > 0 && (vp8data[0] & 0x01) == 0);
				}
				videoFrameBuf_.insert(videoFrameBuf_.end(), vp8data, vp8data + stripped);
				if (rtp.marker) flushVideoFrame();
			}
		}
	}

	void writeAudioPacket(uint32_t ts, const uint8_t* data, int size) {
		int64_t pts = av_rescale_q(ts - audioBaseTs_,
			{1, (int)audioClockRate_}, audioStream_->time_base);
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.stream_index = audioStream_->index;
		pkt.data = const_cast<uint8_t*>(data);
		pkt.size = size;
		pkt.pts = pts;
		pkt.dts = pts;
		av_interleaved_write_frame(fmtCtx_, &pkt);
	}

	void flushVideoFrame() {
		if (videoFrameBuf_.empty() || !fmtCtx_) return;

		// For H264: commit deferred header on first keyframe with SPS/PPS
		if (headerDeferred_) {
			if (!videoKeyframe_) { videoFrameBuf_.clear(); return; } // drop until IDR
			if (!setH264Extradata() || !commitDeferredHeader()) {
				videoFrameBuf_.clear(); return;
			}
			// Flush buffered audio
			for (auto& a : pendingAudio_)
				writeAudioPacket(a.first, a.second.data(), a.second.size());
			pendingAudio_.clear();
		}

		// H264 matroska needs AVCC format: convert Annex-B start codes to 4-byte length prefix
		std::vector<uint8_t>* writeData = &videoFrameBuf_;
		std::vector<uint8_t> avccBuf;
		if (videoCodec_ == VideoCodec::H264) {
			annexBToAvcc(videoFrameBuf_, avccBuf);
			writeData = &avccBuf;
		}

		int64_t pts = av_rescale_q(videoFrameTs_ - videoBaseTs_,
			{1, (int)videoClockRate_}, videoStream_->time_base);
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.stream_index = videoStream_->index;
		pkt.data = writeData->data();
		pkt.size = writeData->size();
		pkt.pts = pts;
		pkt.dts = pts;
		if (videoKeyframe_) pkt.flags |= AV_PKT_FLAG_KEY;
		av_interleaved_write_frame(fmtCtx_, &pkt);
		videoFrameBuf_.clear();
	}

	// Convert Annex-B (00 00 00 01 + NAL) to AVCC (4-byte length + NAL)
public:
	static void annexBToAvcc(const std::vector<uint8_t>& annexB, std::vector<uint8_t>& avcc) {
		avcc.clear();
		size_t i = 0;
		while (i < annexB.size()) {
			// Find start code
			if (i + 4 <= annexB.size() && annexB[i]==0 && annexB[i+1]==0 && annexB[i+2]==0 && annexB[i+3]==1) {
				size_t nalStart = i + 4;
				// Find next start code or end
				size_t nalEnd = annexB.size();
				for (size_t j = nalStart; j + 3 < annexB.size(); j++) {
					if (annexB[j]==0 && annexB[j+1]==0 && annexB[j+2]==0 && annexB[j+3]==1) {
						nalEnd = j; break;
					}
				}
				uint32_t nalLen = nalEnd - nalStart;
				avcc.push_back((nalLen >> 24) & 0xFF);
				avcc.push_back((nalLen >> 16) & 0xFF);
				avcc.push_back((nalLen >> 8) & 0xFF);
				avcc.push_back(nalLen & 0xFF);
				avcc.insert(avcc.end(), annexB.begin() + nalStart, annexB.begin() + nalEnd);
				i = nalEnd;
			} else {
				i++;
			}
		}
	}

private:
	std::string peerId_;
	std::string roomId_;
	std::string logTag_;
	std::string outputPath_;
	uint8_t audioPT_, videoPT_;
	uint32_t audioClockRate_, videoClockRate_;
	VideoCodec videoCodec_;
	int sock_ = -1;
	int port_ = 0;
	std::atomic<bool> running_{false};
	std::thread thread_;

	AVFormatContext* fmtCtx_ = nullptr;
	AVStream* audioStream_ = nullptr;
	AVStream* videoStream_ = nullptr;
	std::mutex muxMutex_;
	bool headerDeferred_ = false;
	std::vector<std::pair<uint32_t, std::vector<uint8_t>>> pendingAudio_;

	uint32_t audioBaseTs_ = 0, videoBaseTs_ = 0;
	bool audioBaseSet_ = false, videoBaseSet_ = false;
	std::vector<uint8_t> videoFrameBuf_;
	uint32_t videoFrameTs_ = 0;
	bool videoKeyframe_ = false;
	int h264NalLog_ = 0;
	std::vector<uint8_t> h264Sps_, h264Pps_;

	std::shared_ptr<spdlog::logger> logger_;
	std::chrono::steady_clock::time_point startTime_;
	std::atomic<bool> firstRtpReceived_{false};
	std::chrono::steady_clock::time_point firstRtpTime_;
	std::mutex qosMutex_;
	std::ofstream qosFile_;
	bool qosFirst_ = true;
};

} // namespace mediasoup
