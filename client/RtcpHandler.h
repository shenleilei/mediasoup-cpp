// RtcpHandler.h — RTCP SR, NACK retransmission, and TWCC for PlainTransport sender
// Modeled after mediasoup-worker's RTCP implementation.
#pragma once

#include <arpa/inet.h>
#include <sys/time.h>
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
		auto& e = buf_[seq];
		memcpy(e.data, data, len);
		e.len = len;
		order_.push_back(seq);
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

// ═══════════════════════════════════════════════════════════
// Unified RTCP receiver: handles PLI, FIR, NACK from Worker
// ═══════════════════════════════════════════════════════════
struct RtcpContext {
	// Sender stats (for SR)
	uint32_t videoSsrc = 0;
	uint32_t audioSsrc = 0;
	uint32_t videoPacketCount = 0;
	uint32_t videoOctetCount = 0;
	uint32_t audioPacketCount = 0;
	uint32_t audioOctetCount = 0;
	uint32_t lastVideoRtpTs = 0;
	uint32_t lastAudioRtpTs = 0;

	// Retransmission buffer
	RtpPacketStore videoStore;
	RtpPacketStore audioStore;

	// Keyframe cache (for PLI)
	std::vector<uint8_t> lastKeyframe;
	uint32_t lastKeyframeTs = 0;

	// SR interval
	int64_t lastSrSentMs = 0;
	static constexpr int64_t kSrIntervalMs = 1000;

	// Stats
	int nackRetransmitted = 0;
	int pliResponded = 0;

	// Send function
	using SendH264Fn = std::function<void(int fd, const uint8_t* data, int size,
		uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq)>;
	SendH264Fn sendH264Fn;
	uint8_t videoPt = 0;
	uint16_t* videoSeqPtr = nullptr;

	// Record a sent video RTP packet
	void onVideoRtpSent(const uint8_t* pkt, size_t len, uint32_t rtpTs) {
		videoStore.store(videoPacketCount & 0xFFFF, pkt, len);
		// Note: we store by actual seq from the packet header
		uint16_t seq = (pkt[2] << 8) | pkt[3];
		videoStore.store(seq, pkt, len);
		videoPacketCount++;
		videoOctetCount += (len > 12) ? (len - 12) : 0;
		lastVideoRtpTs = rtpTs;
	}

	void onAudioRtpSent(size_t payloadLen, uint32_t rtpTs) {
		audioPacketCount++;
		audioOctetCount += payloadLen;
		lastAudioRtpTs = rtpTs;
	}

	// Cache keyframe for PLI response
	void cacheKeyframe(const uint8_t* data, size_t len, uint32_t rtpTs) {
		lastKeyframe.assign(data, data + len);
		lastKeyframeTs = rtpTs;
	}

	// Maybe send periodic SR
	void maybeSendSR(int udpFd) {
		auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
		if (now - lastSrSentMs < kSrIntervalMs) return;
		lastSrSentMs = now;

		uint8_t buf[64];
		// Video SR
		if (videoPacketCount > 0) {
			size_t len = buildSenderReport(buf, videoSsrc, lastVideoRtpTs,
				videoPacketCount, videoOctetCount);
			send(udpFd, buf, len, 0);
		}
		// Audio SR
		if (audioPacketCount > 0) {
			size_t len = buildSenderReport(buf, audioSsrc, lastAudioRtpTs,
				audioPacketCount, audioOctetCount);
			send(udpFd, buf, len, 0);
		}
	}

	// Process incoming RTCP from Worker
	void processIncomingRtcp(int udpFd) {
		uint8_t buf[1500];
		while (true) {
			int n = recv(udpFd, buf, sizeof(buf), MSG_DONTWAIT);
			if (n <= 0) break;

			// Parse compound RTCP: may contain multiple packets
			size_t offset = 0;
			while (offset + 4 <= (size_t)n) {
				uint8_t version = (buf[offset] >> 6) & 0x3;
				if (version != 2) break;

				uint8_t fmt = buf[offset] & 0x1F;
				uint8_t pt = buf[offset + 1];
				uint16_t lengthWords = (buf[offset + 2] << 8) | buf[offset + 3];
				size_t pktLen = (lengthWords + 1) * 4;
				if (offset + pktLen > (size_t)n) break;

				if (pt == RTCP_PT_PSFB) {
					// PLI (fmt=1) or FIR (fmt=4)
					if ((fmt == 1 || fmt == 4) && !lastKeyframe.empty() && sendH264Fn && videoSeqPtr) {
						sendH264Fn(udpFd, lastKeyframe.data(), lastKeyframe.size(),
							videoPt, lastKeyframeTs, videoSsrc, *videoSeqPtr);
						pliResponded++;
					}
				} else if (pt == RTCP_PT_RTPFB && fmt == 1) {
					// NACK
					int ret = handleNack(buf + offset, pktLen, videoStore, udpFd);
					nackRetransmitted += ret;
				}
				// RR (PT=201) — we receive but don't need to act on it
				// TWCC (PT=205, fmt=15) — for future BWE implementation

				offset += pktLen;
			}
		}
	}
};
