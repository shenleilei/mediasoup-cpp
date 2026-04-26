#include "Recorder.h"

#include "ffmpeg/AvTime.h"
#include "media/h264/Avcc.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
}

#include <cmath>
#include <cstdio>

namespace mediasoup {

PeerRecorder::PeerRecorder(const std::string& peerId, const std::string& outputPath,
	uint8_t audioPT, uint8_t videoPT, uint32_t audioClockRate, uint32_t videoClockRate,
	VideoCodec videoCodec, const std::string& roomId)
	: peerId_(peerId)
	, roomId_(roomId)
	, logTag_("[" + roomId + " " + peerId + "]")
	, outputPath_(outputPath)
	, audioPT_(audioPT)
	, videoPT_(videoPT)
	, audioClockRate_(audioClockRate)
	, videoClockRate_(videoClockRate)
	, videoCodec_(videoCodec)
	, logger_(Logger::Get("Recorder"))
{
}

PeerRecorder::~PeerRecorder()
{
	stop();
}

int PeerRecorder::createSocket()
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) return -1;
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;
	if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
		::close(sock);
		return -1;
	}
	socklen_t addrLen = sizeof(addr);
	getsockname(sock, reinterpret_cast<sockaddr*>(&addr), &addrLen);
	port_ = ntohs(addr.sin_port);
	sock_.store(sock, std::memory_order_release);
	return port_;
}

bool PeerRecorder::start()
{
	if (sock_.load(std::memory_order_acquire) < 0) return false;
	if (!initMuxer()) return false;
	startTime_ = std::chrono::steady_clock::now();
	running_ = true;
	thread_ = std::thread([this] { recvLoop(); });
	return true;
}

void PeerRecorder::stop()
{
	running_ = false;
	int sock = sock_.exchange(-1, std::memory_order_acq_rel);
	if (sock >= 0) {
		::close(sock);
	}
	if (thread_.joinable()) {
		thread_.join();
	}
	finalizeMuxer();
	{
		std::lock_guard<std::mutex> lock(qosMutex_);
		if (qosFile_.is_open()) {
			qosFile_ << "\n]";
			qosFile_.close();
		} else if (!outputPath_.empty()) {
			std::string qosPath = outputPath_.substr(0, outputPath_.rfind('.')) + ".qos.json";
			std::ofstream output(qosPath);
			if (output.is_open()) {
				output << "[]";
				output.close();
			}
		}
		outputPath_.clear();
	}
}

void PeerRecorder::appendQosSnapshot(const json& stats)
{
	std::lock_guard<std::mutex> lock(qosMutex_);
	if (!running_) return;
	if (outputPath_.empty()) return;
	if (!qosFile_.is_open()) {
		std::string qosPath = outputPath_.substr(0, outputPath_.rfind('.')) + ".qos.json";
		qosFile_.open(qosPath, std::ios::out | std::ios::trunc);
		if (!qosFile_.is_open()) return;
		qosFile_ << "[\n";
		qosFirst_ = true;
	}
	auto baseTime = startTime_;
	const int64_t firstRtpTimeNs = firstRtpTimeNs_.load(std::memory_order_acquire);
	if (firstRtpTimeNs >= 0) {
		baseTime = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(firstRtpTimeNs));
	}
	auto elapsed = std::chrono::steady_clock::now() - baseTime;
	double secs = std::chrono::duration<double>(elapsed).count();
	if (secs < 0) secs = 0;
	if (!qosFirst_) qosFile_ << ",\n";
	qosFirst_ = false;
	json entry = {{"t", std::round(secs * 10.0) / 10.0}, {"stats", stats}};
	qosFile_ << entry.dump();
	qosFile_.flush();
}

bool PeerRecorder::initMuxer()
{
	av_log_set_level(AV_LOG_ERROR);

	const char* fmt = (videoCodec_ == VideoCodec::H264) ? "matroska" : "webm";
	try {
		outputFormat_ = msff::OutputFormat::Create(fmt, outputPath_);
	} catch (...) {
		return false;
	}

	audioStream_ = outputFormat_->NewStream();
	if (!audioStream_) return false;
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
	audioStream_->codecpar->extradata =
		(uint8_t*)av_mallocz(sizeof(opusHead) + AV_INPUT_BUFFER_PADDING_SIZE);
	std::memcpy(audioStream_->codecpar->extradata, opusHead, sizeof(opusHead));
	audioStream_->codecpar->extradata_size = sizeof(opusHead);

	videoStream_ = outputFormat_->NewStream();
	if (!videoStream_) return false;
	videoStream_->id = 1;
	videoStream_->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	videoStream_->codecpar->codec_id = (videoCodec_ == VideoCodec::H264)
		? AV_CODEC_ID_H264
		: AV_CODEC_ID_VP8;
	videoStream_->codecpar->width = 640;
	videoStream_->codecpar->height = 480;
	videoStream_->time_base = {1, (int)videoClockRate_};

	try {
		outputFormat_->OpenIo();
	} catch (...) {
		outputFormat_.reset();
		audioStream_ = nullptr;
		videoStream_ = nullptr;
		return false;
	}

	if (videoCodec_ == VideoCodec::H264) {
		headerDeferred_ = true;
		MS_DEBUG(logger_, "{} Recording deferred (waiting for H264 IDR): {}", logTag_, outputPath_);
	} else {
		try {
			outputFormat_->WriteHeader();
		} catch (...) {
			outputFormat_.reset();
			audioStream_ = nullptr;
			videoStream_ = nullptr;
			return false;
		}
		MS_DEBUG(logger_, "{} Recording started: {} (VP8)", logTag_, outputPath_);
	}
	return true;
}

bool PeerRecorder::setH264Extradata()
{
	if (h264Sps_.empty() || h264Pps_.empty()) return false;
	if (h264Sps_.size() < 4 || !videoStream_ || !videoStream_->codecpar) return false;
	int spsLen = h264Sps_.size();
	int ppsLen = h264Pps_.size();
	int extradataSize = 6 + 2 + spsLen + 1 + 2 + ppsLen;
	uint8_t* extradata =
		(uint8_t*)av_mallocz(extradataSize + AV_INPUT_BUFFER_PADDING_SIZE);
	if (!extradata) return false;
	extradata[0] = 1;
	extradata[1] = h264Sps_[1];
	extradata[2] = h264Sps_[2];
	extradata[3] = h264Sps_[3];
	extradata[4] = 0xFF;
	extradata[5] = 0xE1;
	extradata[6] = (spsLen >> 8);
	extradata[7] = spsLen & 0xFF;
	std::memcpy(extradata + 8, h264Sps_.data(), spsLen);
	int offset = 8 + spsLen;
	extradata[offset] = 1;
	extradata[offset + 1] = (ppsLen >> 8);
	extradata[offset + 2] = ppsLen & 0xFF;
	std::memcpy(extradata + offset + 3, h264Pps_.data(), ppsLen);
	videoStream_->codecpar->extradata = extradata;
	videoStream_->codecpar->extradata_size = extradataSize;
	return true;
}

bool PeerRecorder::commitDeferredHeader()
{
	try {
		outputFormat_->WriteHeader();
	} catch (const std::exception& e) {
		MS_ERROR(logger_, "{} avformat_write_header failed for H264 {}: {}",
			logTag_, outputPath_, e.what());
		outputFormat_.reset();
		audioStream_ = nullptr;
		videoStream_ = nullptr;
		return false;
	}
	headerDeferred_ = false;
	MS_DEBUG(logger_, "{} Recording started: {} (H264, header written)", logTag_, outputPath_);
	return true;
}

void PeerRecorder::finalizeMuxer()
{
	std::lock_guard<std::mutex> lock(muxMutex_);
	if (!outputFormat_) return;
	bool wroteFrames = !headerDeferred_;
	flushVideoFrame();
	if (!headerDeferred_) {
		try {
			outputFormat_->WriteTrailer();
		} catch (const std::exception& e) {
			MS_WARN(logger_, "{} av_write_trailer failed: {}", logTag_, e.what());
		}
	}
	outputFormat_.reset();
	audioStream_ = nullptr;
	videoStream_ = nullptr;
	if (!wroteFrames && !outputPath_.empty()) {
		std::remove(outputPath_.c_str());
		MS_DEBUG(logger_, "{} Removed empty recording: {}", logTag_, outputPath_);
	} else {
		MS_DEBUG(logger_, "{} Recording finalized: {}", logTag_, outputPath_);
	}
}

void PeerRecorder::recvLoop()
{
	uint8_t buf[65536];
	int packetCount = 0;
	while (running_) {
		const int sock = sock_.load(std::memory_order_acquire);
		if (sock < 0) break;
		struct pollfd pfd = {sock, POLLIN, 0};
		int ret = poll(&pfd, 1, 200);
		if (ret <= 0) continue;
		int n = recv(sock, buf, sizeof(buf), 0);
		if (n <= 0) continue;
		packetCount++;
		media::rtp::RtpHeader rtp;
		if (!media::rtp::RtpHeader::Parse(buf, static_cast<size_t>(n), &rtp)) continue;
		if (packetCount <= 10 || packetCount % 500 == 0) {
			MS_DEBUG(logger_, "{} RTP recv #{} size={} PT={} (audio={} video={})",
				logTag_, packetCount, n, rtp.payloadType, audioPT_, videoPT_);
		}
		writePacket(rtp);
	}
	MS_DEBUG(logger_, "{} recvLoop exited, total packets={}", logTag_, packetCount);
}

const uint8_t* PeerRecorder::stripVp8Descriptor(const uint8_t* data, int size,
	int& outSize, bool& isStart)
{
	if (size < 1) {
		outSize = 0;
		isStart = false;
		return data;
	}
	isStart = (data[0] & 0x10) != 0;
	int skip = 1;
	if (data[0] & 0x80) {
		if (size < 2) {
			outSize = 0;
			return data;
		}
		skip++;
		uint8_t x = data[1];
		if (x & 0x80) {
			skip++;
			if (size > skip && data[skip - 1] & 0x80) skip++;
		}
		if (x & 0x40) skip++;
		if (x & 0x20 || x & 0x10) skip++;
	}
	if (skip >= size) {
		outSize = 0;
		return data;
	}
	outSize = size - skip;
	return data + skip;
}

void PeerRecorder::depacketizeH264(const media::rtp::RtpHeader& rtp)
{
	const uint8_t* data = rtp.payload;
	size_t size = rtp.payloadSize;
	if (size < 1) return;

	uint8_t nalType = data[0] & 0x1F;

	if (h264NalLog_ < kMaxH264NalDebugLogs) {
		h264NalLog_++;
		if (nalType == 28 && size >= 2) {
			uint8_t fuNalType = data[1] & 0x1F;
			bool fuStart = (data[1] & 0x80) != 0;
			MS_DEBUG(logger_, "{} H264 RTP: nalType={} FU-A fuNalType={} fuStart={} size={} marker={}",
				peerId_, nalType, fuNalType, fuStart, size, rtp.marker);
		} else {
			MS_DEBUG(logger_, "{} H264 RTP: nalType={} size={} marker={}",
				logTag_, nalType, size, rtp.marker);
		}
	}

	if (nalType >= 1 && nalType <= 23) {
		if (nalType == 7) {
			h264Sps_.assign(data, data + size);
			return;
		}
		if (nalType == 8) {
			h264Pps_.assign(data, data + size);
			return;
		}
		bool isIdr = (nalType == 5);
		if (videoFrameBuf_.empty()) {
			videoKeyframe_ = isIdr;
			videoFrameTs_ = rtp.timestamp;
		}
		static const uint8_t startCode[] = {0, 0, 0, 1};
		videoFrameBuf_.insert(videoFrameBuf_.end(), startCode, startCode + 4);
		videoFrameBuf_.insert(videoFrameBuf_.end(), data, data + size);
		if (rtp.marker) flushVideoFrame();
	} else if (nalType == 28) {
		if (size < 2) return;
		uint8_t fuHeader = data[1];
		bool fuStart = (fuHeader & 0x80) != 0;
		uint8_t fuNalType = fuHeader & 0x1F;

		if (fuStart) {
			if (!videoFrameBuf_.empty()) flushVideoFrame();
			videoKeyframe_ = (fuNalType == 5);
			videoFrameTs_ = rtp.timestamp;
			uint8_t nalHeader = (data[0] & 0xE0) | fuNalType;
			static const uint8_t startCode[] = {0, 0, 0, 1};
			videoFrameBuf_.insert(videoFrameBuf_.end(), startCode, startCode + 4);
			videoFrameBuf_.push_back(nalHeader);
		}
		videoFrameBuf_.insert(videoFrameBuf_.end(), data + 2, data + size);
		if ((fuHeader & 0x40) || rtp.marker) flushVideoFrame();
	} else if (nalType == 24) {
		size_t offset = 1;
		while (offset + 2 <= size) {
			size_t nalSize = (static_cast<size_t>(data[offset]) << 8) | data[offset + 1];
			offset += 2;
			if (offset + nalSize > size) break;
			uint8_t nt = data[offset] & 0x1F;
			if (nt == 7) {
				h264Sps_.assign(data + offset, data + offset + nalSize);
			} else if (nt == 8) {
				h264Pps_.assign(data + offset, data + offset + nalSize);
			} else {
				if (nt == 5) videoKeyframe_ = true;
				if (videoFrameBuf_.empty()) videoFrameTs_ = rtp.timestamp;
				static const uint8_t startCode[] = {0, 0, 0, 1};
				videoFrameBuf_.insert(videoFrameBuf_.end(), startCode, startCode + 4);
				videoFrameBuf_.insert(videoFrameBuf_.end(), data + offset, data + offset + nalSize);
			}
			offset += nalSize;
		}
		if (rtp.marker && !videoFrameBuf_.empty()) flushVideoFrame();
	}
}

void PeerRecorder::writePacket(const media::rtp::RtpHeader& rtp)
{
	std::lock_guard<std::mutex> lock(muxMutex_);
	if (!outputFormat_) return;

	const auto now = std::chrono::steady_clock::now();
	const auto nowNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
		now.time_since_epoch()).count();
	int64_t expectedFirstRtpTimeNs = -1;
	(void)firstRtpTimeNs_.compare_exchange_strong(
		expectedFirstRtpTimeNs,
		nowNs,
		std::memory_order_acq_rel,
		std::memory_order_acquire);

	if (rtp.payloadType == audioPT_) {
		if (!audioBaseSet_) {
			audioBaseTs_ = rtp.timestamp;
			audioLastTs_ = rtp.timestamp;
			audioBaseSet_ = true;
		}
		if (headerDeferred_) {
			if (pendingAudio_.size() < kMaxPendingAudioPackets) {
				pendingAudio_.push_back({
					rtp.timestamp,
					std::vector<uint8_t>(rtp.payload, rtp.payload + rtp.payloadSize)
				});
			}
			return;
		}
		writeAudioPacket(rtp.timestamp, rtp.payload, static_cast<int>(rtp.payloadSize));
	} else if (rtp.payloadType == videoPT_) {
		if (!videoBaseSet_) {
			videoBaseTs_ = rtp.timestamp;
			videoLastTs_ = rtp.timestamp;
			videoBaseSet_ = true;
		}

		if (videoCodec_ == VideoCodec::H264) {
			depacketizeH264(rtp);
		} else {
			bool isStart = false;
			int stripped = 0;
			const uint8_t* vp8Data =
				stripVp8Descriptor(rtp.payload, static_cast<int>(rtp.payloadSize), stripped, isStart);
			if (stripped <= 0) return;

			if (isStart && !videoFrameBuf_.empty()) flushVideoFrame();
			if (isStart) {
				videoFrameTs_ = rtp.timestamp;
				videoKeyframe_ = (stripped > 0 && (vp8Data[0] & 0x01) == 0);
			}
			videoFrameBuf_.insert(videoFrameBuf_.end(), vp8Data, vp8Data + stripped);
			if (rtp.marker) flushVideoFrame();
		}
	}
}

uint64_t PeerRecorder::unwrapTimestamp(uint32_t ts, uint32_t baseTs, uint32_t& lastTs, uint64_t& wrapCount)
{
	if (ts < lastTs) {
		if (lastTs - ts > 0x80000000) {
			wrapCount++;
		}
	} else {
		if (ts - lastTs > 0x80000000 && wrapCount > 0) {
			uint64_t ticks = ((wrapCount - 1) << 32) + ts;
			return ticks >= baseTs ? ticks - baseTs : 0;
		}
	}
	lastTs = ts;
	uint64_t ticks = (wrapCount << 32) + ts;
	return ticks >= baseTs ? ticks - baseTs : 0;
}

void PeerRecorder::writeAudioPacket(uint32_t ts, const uint8_t* data, int size)
{
	const auto ticks = unwrapTimestamp(ts, audioBaseTs_, audioLastTs_, audioWrapCount_);
	int64_t pts = msff::ClampNonNegativePts(msff::RescaleQ(
		static_cast<int64_t>(ticks),
		{1, (int)audioClockRate_},
		audioStream_->time_base));
	AVPacket pkt{};
	pkt.stream_index = audioStream_->index;
	pkt.data = const_cast<uint8_t*>(data);
	pkt.size = size;
	pkt.pts = pts;
	pkt.dts = pts;
	outputFormat_->WriteInterleavedFrame(&pkt);
}

void PeerRecorder::flushVideoFrame()
{
	if (videoFrameBuf_.empty() || !outputFormat_) return;

	if (headerDeferred_) {
		if (!videoKeyframe_) {
			videoFrameBuf_.clear();
			return;
		}
		if (!setH264Extradata() || !commitDeferredHeader()) {
			videoFrameBuf_.clear();
			return;
		}
		for (auto& audio : pendingAudio_)
			writeAudioPacket(audio.first, audio.second.data(), static_cast<int>(audio.second.size()));
		pendingAudio_.clear();
	}

	std::vector<uint8_t>* writeData = &videoFrameBuf_;
	std::vector<uint8_t> avccBuf;
	if (videoCodec_ == VideoCodec::H264) {
		media::h264::AnnexBToAvcc(videoFrameBuf_, &avccBuf);
		writeData = &avccBuf;
	}

	const auto ticks = unwrapTimestamp(videoFrameTs_, videoBaseTs_, videoLastTs_, videoWrapCount_);
	int64_t pts = msff::ClampNonNegativePts(msff::RescaleQ(
		static_cast<int64_t>(ticks),
		{1, static_cast<int>(videoClockRate_)},
		videoStream_->time_base));
	AVPacket pkt{};
	pkt.stream_index = videoStream_->index;
	pkt.data = writeData->data();
	pkt.size = static_cast<int>(writeData->size());
	pkt.pts = pts;
	pkt.dts = pts;
	if (videoKeyframe_) pkt.flags |= AV_PKT_FLAG_KEY;
	outputFormat_->WriteInterleavedFrame(&pkt);
	videoFrameBuf_.clear();
}

} // namespace mediasoup
