// RtcpHandler.h — RTCP SR, NACK retransmission, and TWCC for PlainTransport sender
// Modeled after mediasoup-worker's RTCP implementation.
#pragma once

#include <arpa/inet.h>
#include <sys/time.h>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <chrono>
#include <deque>
#include <functional>
#include <unordered_map>
#include <vector>

// ═══════════════════════════════════════════════════════════
// RTCP packet types (RFC 3550 / 4585 / draft-holmer-rmcat-transport-wide-cc)
// ═══════════════════════════════════════════════════════════
static constexpr uint8_t RTCP_PT_SR   = 200;
static constexpr uint8_t RTCP_PT_RR   = 201;
static constexpr uint8_t RTCP_PT_RTPFB = 205; // NACK (fmt=1)
static constexpr uint8_t RTCP_PT_PSFB  = 206; // PLI (fmt=1), FIR (fmt=4)

// ═══════════════════════════════════════════════════════════
// NTP helpers
// ═══════════════════════════════════════════════════════════
struct NtpTime {
	uint32_t sec;
	uint32_t frac;
};

inline NtpTime getNtpNow() {
	struct timeval tv;
	gettimeofday(&tv, nullptr);
	// NTP epoch offset from Unix epoch: 70 years in seconds
	static constexpr uint32_t kNtpEpochOffset = 2208988800u;
	NtpTime ntp;
	ntp.sec = static_cast<uint32_t>(tv.tv_sec) + kNtpEpochOffset;
	ntp.frac = static_cast<uint32_t>((double)tv.tv_usec / 1e6 * (1LL << 32));
	return ntp;
}

// ═══════════════════════════════════════════════════════════
// RTP retransmission buffer (for NACK)
// ═══════════════════════════════════════════════════════════
struct RtpPacketStore {
	static constexpr size_t kMaxPackets = 1024;

	void store(uint16_t seq, const uint8_t* data, size_t len) {
		if (len > sizeof(Entry::data)) return;
		const bool isNewEntry = buf_.find(seq) == buf_.end();
		auto& e = buf_[seq];
		memcpy(e.data, data, len);
		e.len = len;
		if (isNewEntry) order_.push_back(seq);
		while (order_.size() > kMaxPackets) {
			buf_.erase(order_.front());
			order_.pop_front();
		}
	}

	bool get(uint16_t seq, const uint8_t*& data, size_t& len) const {
		auto it = buf_.find(seq);
		if (it == buf_.end()) return false;
		data = it->second.data;
		len = it->second.len;
		return true;
	}

private:
	struct Entry { uint8_t data[1500]; size_t len = 0; };
	std::unordered_map<uint16_t, Entry> buf_;
	std::deque<uint16_t> order_;
};

// ═══════════════════════════════════════════════════════════
// RTCP Sender Report builder
// ═══════════════════════════════════════════════════════════
// Builds a compound RTCP SR packet (no RR blocks, just SR + SDES CNAME)
inline size_t buildSenderReport(uint8_t* buf, uint32_t ssrc,
	uint32_t rtpTs, uint32_t packetCount, uint32_t octetCount)
{
	auto ntp = getNtpNow();

	// RTCP header: V=2, P=0, RC=0, PT=200(SR), length in 32-bit words - 1
	buf[0] = 0x80; // V=2, RC=0
	buf[1] = RTCP_PT_SR;
	// Length = 6 words (header 1 + SSRC 1 + SR body 5) → length field = 6
	buf[2] = 0; buf[3] = 6;
	// SSRC
	buf[4] = (ssrc >> 24); buf[5] = (ssrc >> 16); buf[6] = (ssrc >> 8); buf[7] = ssrc;
	// NTP timestamp
	buf[8]  = (ntp.sec >> 24); buf[9]  = (ntp.sec >> 16); buf[10] = (ntp.sec >> 8); buf[11] = ntp.sec;
	buf[12] = (ntp.frac >> 24); buf[13] = (ntp.frac >> 16); buf[14] = (ntp.frac >> 8); buf[15] = ntp.frac;
	// RTP timestamp
	buf[16] = (rtpTs >> 24); buf[17] = (rtpTs >> 16); buf[18] = (rtpTs >> 8); buf[19] = rtpTs;
	// Packet count
	buf[20] = (packetCount >> 24); buf[21] = (packetCount >> 16); buf[22] = (packetCount >> 8); buf[23] = packetCount;
	// Octet count
	buf[24] = (octetCount >> 24); buf[25] = (octetCount >> 16); buf[26] = (octetCount >> 8); buf[27] = octetCount;

	return 28;
}

// ═══════════════════════════════════════════════════════════
// RTCP NACK parser + retransmitter
// ═══════════════════════════════════════════════════════════
// Parse NACK items from RTPFB (PT=205, fmt=1) and retransmit from store.
// Returns number of packets retransmitted.
inline int handleNack(const uint8_t* rtcpData, size_t rtcpLen,
	const RtpPacketStore& store, int udpFd)
{
	// RTPFB header: 4 bytes common + 4 bytes sender SSRC + 4 bytes media SSRC = 12 bytes
	// Then NACK items: each 4 bytes (PID + BLP)
	if (rtcpLen < 12) return 0;

	int retransmitted = 0;
	size_t offset = 12; // skip header + sender SSRC + media SSRC
	while (offset + 4 <= rtcpLen) {
		uint16_t pid = (rtcpData[offset] << 8) | rtcpData[offset + 1];
		uint16_t blp = (rtcpData[offset + 2] << 8) | rtcpData[offset + 3];
		offset += 4;

		// Retransmit PID
		const uint8_t* pkt; size_t len;
		if (store.get(pid, pkt, len)) {
			send(udpFd, pkt, len, 0);
			retransmitted++;
		}
		// Retransmit each bit in BLP
		for (int i = 0; i < 16; i++) {
			if (blp & (1 << i)) {
				uint16_t seq = pid + 1 + i;
				if (store.get(seq, pkt, len)) {
					send(udpFd, pkt, len, 0);
					retransmitted++;
				}
			}
		}
	}
	return retransmitted;
}

inline int32_t parseSignedRtcpTotalLost(const uint8_t* data)
{
	uint32_t value = ((uint32_t)data[0] << 16) |
		((uint32_t)data[1] << 8) |
		(uint32_t)data[2];

	if (((value >> 23) & 1u) == 0u)
		return static_cast<int32_t>(value);

	if (value != 0x0800000u)
		value &= ~(1u << 23);

	return -static_cast<int32_t>(value);
}

inline uint32_t parseClampedRtcpTotalLost(const uint8_t* data)
{
	return static_cast<uint32_t>(std::max<int32_t>(0, parseSignedRtcpTotalLost(data)));
}

// ═══════════════════════════════════════════════════════════
// Unified RTCP receiver: handles PLI, FIR, NACK from Worker
// ═══════════════════════════════════════════════════════════
struct VideoStreamRtcpState {
	uint32_t ssrc = 0;
	uint8_t payloadType = 0;
	uint32_t packetCount = 0;
	uint32_t octetCount = 0;
	uint32_t lastRtpTs = 0;
	RtpPacketStore store;
	std::vector<uint8_t> lastKeyframe;
	uint32_t lastKeyframeTs = 0;
	double rrLossFraction = 0;      // 0..1, from last RR
	uint32_t rrCumulativeLost = 0;  // total packets lost
	double rrJitterMs = 0;          // video RR interarrival jitter in ms (current client consumes 90kHz video clock only)
	double rrRttMs = -1;            // round-trip time from SR/RR exchange, -1 = unknown
	int64_t lastRrReceivedMs = 0;
	std::unordered_map<uint32_t, int64_t> srNtpToSendTime;
	std::deque<uint32_t> srNtpOrder;
	uint16_t* seqPtr = nullptr;
	int nackRetransmitted = 0;
	int pliResponded = 0;
};

struct RtcpContext {
	uint32_t audioSsrc = 0;
	uint32_t audioPacketCount = 0;
	uint32_t audioOctetCount = 0;
	uint32_t lastAudioRtpTs = 0;
	RtpPacketStore audioStore;

	std::unordered_map<uint32_t, VideoStreamRtcpState> videoStreams;

	int64_t lastSrSentMs = 0;
	static constexpr int64_t kSrIntervalMs = 1000;

	int nackRetransmitted = 0;
	int pliResponded = 0;

	using SendH264Fn = std::function<void(int fd, const uint8_t* data, int size,
		uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq)>;
	SendH264Fn sendH264Fn;
	using RequestKeyframeFn = std::function<void(uint32_t ssrc)>;
	RequestKeyframeFn requestKeyframeFn;
	using CanSendVideoFn = std::function<bool(uint32_t ssrc)>;
	CanSendVideoFn canSendVideoFn;

	void registerVideoStream(uint32_t ssrc, uint8_t payloadType, uint16_t* seqPtr) {
		auto& stream = videoStreams[ssrc];
		stream.ssrc = ssrc;
		stream.payloadType = payloadType;
		stream.seqPtr = seqPtr;
	}

	VideoStreamRtcpState* getVideoStream(uint32_t ssrc) {
		auto it = videoStreams.find(ssrc);
		return it != videoStreams.end() ? &it->second : nullptr;
	}

	const VideoStreamRtcpState* getVideoStream(uint32_t ssrc) const {
		auto it = videoStreams.find(ssrc);
		return it != videoStreams.end() ? &it->second : nullptr;
	}

	VideoStreamRtcpState* getFirstVideoStream() {
		return videoStreams.empty() ? nullptr : &videoStreams.begin()->second;
	}

	const VideoStreamRtcpState* getFirstVideoStream() const {
		return videoStreams.empty() ? nullptr : &videoStreams.begin()->second;
	}

	void onVideoRtpSent(const uint8_t* pkt, size_t len) {
		if (len < 12) return;
		uint32_t ssrc = ((uint32_t)pkt[8] << 24) | ((uint32_t)pkt[9] << 16)
			| ((uint32_t)pkt[10] << 8) | pkt[11];
		auto* stream = getVideoStream(ssrc);
		if (!stream) return;
		uint16_t seq = (pkt[2] << 8) | pkt[3];
		uint32_t rtpTs = ((uint32_t)pkt[4] << 24) | ((uint32_t)pkt[5] << 16)
			| ((uint32_t)pkt[6] << 8) | pkt[7];
		stream->store.store(seq, pkt, len);
		stream->packetCount++;
		stream->octetCount += (len > 12) ? (len - 12) : 0;
		stream->lastRtpTs = rtpTs;
	}

	void onAudioRtpSent(size_t payloadLen, uint32_t rtpTs) {
		audioPacketCount++;
		audioOctetCount += payloadLen;
		lastAudioRtpTs = rtpTs;
	}

	void cacheKeyframe(uint32_t ssrc, const uint8_t* data, size_t len, uint32_t rtpTs) {
		auto* stream = getVideoStream(ssrc);
		if (!stream) return;
		stream->lastKeyframe.assign(data, data + len);
		stream->lastKeyframeTs = rtpTs;
	}

	void maybeSendSR(int udpFd) {
		auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
		if (now - lastSrSentMs < kSrIntervalMs) return;
		lastSrSentMs = now;

		uint8_t buf[64];
		for (auto& entry : videoStreams) {
			auto& stream = entry.second;
			if (stream.packetCount == 0) continue;
			size_t len = buildSenderReport(buf, stream.ssrc, stream.lastRtpTs,
				stream.packetCount, stream.octetCount);
			send(udpFd, buf, len, 0);
			uint32_t ntpMid = ((uint32_t)buf[10] << 24) | ((uint32_t)buf[11] << 16)
				| ((uint32_t)buf[12] << 8) | buf[13];
			if (stream.srNtpToSendTime.find(ntpMid) == stream.srNtpToSendTime.end())
				stream.srNtpOrder.push_back(ntpMid);
			stream.srNtpToSendTime[ntpMid] = now;
			while (stream.srNtpOrder.size() > 32) {
				auto oldest = stream.srNtpOrder.front();
				stream.srNtpOrder.pop_front();
				stream.srNtpToSendTime.erase(oldest);
			}
		}
		if (audioPacketCount > 0) {
			size_t len = buildSenderReport(buf, audioSsrc, lastAudioRtpTs,
				audioPacketCount, audioOctetCount);
			send(udpFd, buf, len, 0);
		}
	}

	void processIncomingRtcp(int udpFd) {
		uint8_t buf[1500];
		while (true) {
			int n = recv(udpFd, buf, sizeof(buf), MSG_DONTWAIT);
			if (n <= 0) break;

			size_t offset = 0;
			while (offset + 4 <= (size_t)n) {
				uint8_t version = (buf[offset] >> 6) & 0x3;
				if (version != 2) break;

				uint8_t fmt = buf[offset] & 0x1F;
				uint8_t pt = buf[offset + 1];
				uint16_t lengthWords = (buf[offset + 2] << 8) | buf[offset + 3];
				size_t pktLen = (lengthWords + 1) * 4;
				if (offset + pktLen > (size_t)n) break;

				uint32_t mediaSsrc = 0;
				if (pktLen >= 12) {
					mediaSsrc = ((uint32_t)buf[offset + 8] << 24) | ((uint32_t)buf[offset + 9] << 16)
						| ((uint32_t)buf[offset + 10] << 8) | buf[offset + 11];
				}

				if (pt == RTCP_PT_PSFB) {
					uint32_t targetSsrc = mediaSsrc;
					if (fmt == 4 && targetSsrc == 0 && pktLen >= 20) {
						targetSsrc = ((uint32_t)buf[offset + 12] << 24) | ((uint32_t)buf[offset + 13] << 16)
							| ((uint32_t)buf[offset + 14] << 8) | buf[offset + 15];
					}
					auto* stream = targetSsrc != 0 ? getVideoStream(targetSsrc) : getFirstVideoStream();
					if (stream && canSendVideoFn && !canSendVideoFn(stream->ssrc)) {
						offset += pktLen;
						continue;
					}
					if ((fmt == 1 || fmt == 4) && stream) {
						if (requestKeyframeFn) {
							requestKeyframeFn(stream->ssrc);
							pliResponded++;
							stream->pliResponded++;
						} else if (!stream->lastKeyframe.empty() && sendH264Fn && stream->seqPtr) {
							const uint32_t resendTs =
								stream->lastRtpTs != 0 ? stream->lastRtpTs : stream->lastKeyframeTs;
							sendH264Fn(udpFd, stream->lastKeyframe.data(), stream->lastKeyframe.size(),
								stream->payloadType, resendTs, stream->ssrc, *stream->seqPtr);
							pliResponded++;
							stream->pliResponded++;
						}
					}
				} else if (pt == RTCP_PT_RTPFB && fmt == 1) {
					auto* stream = mediaSsrc != 0 ? getVideoStream(mediaSsrc) : getFirstVideoStream();
					if (stream) {
						if (canSendVideoFn && !canSendVideoFn(stream->ssrc)) {
							offset += pktLen;
							continue;
						}
						int ret = handleNack(buf + offset, pktLen, stream->store, udpFd);
						nackRetransmitted += ret;
						stream->nackRetransmitted += ret;
					}
				} else if (pt == RTCP_PT_RR && pktLen >= 32) {
					size_t rbOff = offset + 8;
					while (rbOff + 24 <= offset + pktLen) {
						uint32_t reportSsrc = ((uint32_t)buf[rbOff] << 24) | ((uint32_t)buf[rbOff + 1] << 16)
							| ((uint32_t)buf[rbOff + 2] << 8) | buf[rbOff + 3];
						auto* stream = getVideoStream(reportSsrc);
						if (!stream) {
							rbOff += 24;
							continue;
						}

						uint8_t lossFrac = buf[rbOff + 4];
						uint32_t cumLost = parseClampedRtcpTotalLost(buf + rbOff + 5);
						uint32_t jitter = ((uint32_t)buf[rbOff + 12] << 24) | ((uint32_t)buf[rbOff + 13] << 16)
							| ((uint32_t)buf[rbOff + 14] << 8) | buf[rbOff + 15];
						uint32_t lsr = ((uint32_t)buf[rbOff + 16] << 24) | ((uint32_t)buf[rbOff + 17] << 16)
							| ((uint32_t)buf[rbOff + 18] << 8) | buf[rbOff + 19];
						uint32_t dlsr = ((uint32_t)buf[rbOff + 20] << 24) | ((uint32_t)buf[rbOff + 21] << 16)
							| ((uint32_t)buf[rbOff + 22] << 8) | buf[rbOff + 23];

						stream->rrLossFraction = lossFrac / 256.0;
						stream->rrCumulativeLost = cumLost;
						stream->rrJitterMs = (double)jitter / 90.0; // video clock 90kHz → ms

						if (lsr != 0) {
							auto it = stream->srNtpToSendTime.find(lsr);
							if (it != stream->srNtpToSendTime.end()) {
								auto nowRtcp = std::chrono::duration_cast<std::chrono::milliseconds>(
									std::chrono::steady_clock::now().time_since_epoch()).count();
								double dlsrMs = (double)dlsr / 65536.0 * 1000.0;
								double elapsed = (double)(nowRtcp - it->second);
								stream->rrRttMs = std::max(0.0, elapsed - dlsrMs);
							}
						}

						stream->lastRrReceivedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
							std::chrono::steady_clock::now().time_since_epoch()).count();
						rbOff += 24;
					}
				}

				offset += pktLen;
			}
		}
	}
};
