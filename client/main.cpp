// plain-client: PlainTransport MP4 推流客户端 (zero external WS dependency)
// Usage: ./plain-client SERVER_IP SERVER_PORT ROOM PEER file.mp4 [--copy]
// --copy: skip re-encoding, use original H264 (no QoS bitrate control)

#include <nlohmann/json.hpp>
#include "RtcpHandler.h"
#include "qos/QosController.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>

#include <atomic>
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <random>

using json = nlohmann::json;
static std::atomic<bool> g_running{true};
static void onSignal(int) { g_running = false; }

// ═══════════════════════════════════════════════════════════
// Minimal WebSocket client (RFC 6455, text frames only)
// ═══════════════════════════════════════════════════════════

static std::string base64Encode(const unsigned char* data, int len) {
	BIO* b64 = BIO_new(BIO_f_base64());
	BIO* mem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, mem);
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	BIO_write(b64, data, len);
	BIO_flush(b64);
	BUF_MEM* bptr;
	BIO_get_mem_ptr(b64, &bptr);
	std::string result(bptr->data, bptr->length);
	BIO_free_all(b64);
	return result;
}

struct WsClient {
	struct PendingRequest {
		std::mutex mutex;
		std::condition_variable cv;
		bool done = false;
		bool ok = false;
		json data = json::object();
		std::string error;
		std::function<void(bool, const json&, const std::string&)> completion;
	};

	int fd = -1;
	std::function<void(const json&)> onNotification;
	std::atomic<bool> running_{false};
	std::atomic<uint32_t> nextId_{1};
	std::thread readerThread_;
	mutable std::mutex stateMutex_;
	std::mutex sendMutex_;
	std::unordered_map<uint32_t, std::shared_ptr<PendingRequest>> pendingRequests_;
	std::deque<json> pendingNotifications_;

	bool connect(const std::string& host, int port, const std::string& path) {
		struct addrinfo hints{}, *res;
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) return false;
		fd = socket(res->ai_family, SOCK_STREAM, 0);
		if (fd < 0) {
			freeaddrinfo(res);
			fd = -1;
			return false;
		}
		if (::connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
			freeaddrinfo(res);
			::close(fd);
			fd = -1;
			return false;
		}
		freeaddrinfo(res);
		int flag = 1;
		setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

		unsigned char keyBytes[16];
		std::random_device rd;
		for (auto& b : keyBytes) b = rd() & 0xFF;
		std::string key = base64Encode(keyBytes, 16);

		std::string req = "GET " + path + " HTTP/1.1\r\n"
			"Host: " + host + ":" + std::to_string(port) + "\r\n"
			"Upgrade: websocket\r\nConnection: Upgrade\r\n"
			"Sec-WebSocket-Key: " + key + "\r\n"
			"Sec-WebSocket-Version: 13\r\n\r\n";
		::send(fd, req.data(), req.size(), 0);

		char buf[4096];
		int n = ::recv(fd, buf, sizeof(buf) - 1, 0);
		if (n <= 0) {
			::close(fd);
			fd = -1;
			return false;
		}
		buf[n] = 0;
		if (strstr(buf, "101") == nullptr) {
			::close(fd);
			fd = -1;
			return false;
		}

		running_ = true;
		readerThread_ = std::thread([this]() { readerLoop(); });
		return true;
	}

	void sendText(const std::string& msg) {
		std::lock_guard<std::mutex> lock(sendMutex_);
		if (fd < 0) return;
		std::vector<uint8_t> frame;
		frame.push_back(0x81);
		uint8_t maskKey[4];
		std::random_device rd;
		for (auto& b : maskKey) b = rd() & 0xFF;

		if (msg.size() < 126) {
			frame.push_back(0x80 | (uint8_t)msg.size());
		} else if (msg.size() < 65536) {
			frame.push_back(0x80 | 126);
			frame.push_back((msg.size() >> 8) & 0xFF);
			frame.push_back(msg.size() & 0xFF);
		} else {
			frame.push_back(0x80 | 127);
			for (int i = 7; i >= 0; i--)
				frame.push_back((msg.size() >> (i * 8)) & 0xFF);
		}
		frame.insert(frame.end(), maskKey, maskKey + 4);
		for (size_t i = 0; i < msg.size(); i++)
			frame.push_back(msg[i] ^ maskKey[i % 4]);
		::send(fd, frame.data(), frame.size(), 0);
	}

	std::string recvText(int timeoutMs = 10000) {
		struct pollfd pfd{fd, POLLIN, 0};
		if (poll(&pfd, 1, timeoutMs) <= 0) return "";

		uint8_t hdr[2];
		if (::recv(fd, hdr, 2, MSG_WAITALL) != 2) return "";
		bool masked = hdr[1] & 0x80;
		uint64_t len = hdr[1] & 0x7F;
		if (len == 126) {
			uint8_t ext[2];
			::recv(fd, ext, 2, MSG_WAITALL);
			len = (ext[0] << 8) | ext[1];
		} else if (len == 127) {
			uint8_t ext[8];
			::recv(fd, ext, 8, MSG_WAITALL);
			len = 0;
			for (int i = 0; i < 8; i++) len = (len << 8) | ext[i];
		}
		uint8_t mask[4] = {};
		if (masked) ::recv(fd, mask, 4, MSG_WAITALL);

		std::string data(len, 0);
		size_t got = 0;
		while (got < len) {
			int n = ::recv(fd, &data[got], len - got, 0);
			if (n <= 0) return "";
			got += n;
		}
		if (masked) for (size_t i = 0; i < len; i++) data[i] ^= mask[i % 4];
		return data;
	}

	void readerLoop() {
		while (running_) {
			auto text = recvText(100);
			if (text.empty()) continue;
			auto msg = json::parse(text, nullptr, false);
			if (msg.is_discarded()) continue;
			if (msg.value("response", false)) {
				handleResponse(msg);
			} else if (msg.contains("method")) {
				std::lock_guard<std::mutex> lock(stateMutex_);
				pendingNotifications_.push_back(std::move(msg));
			}
		}

		failAllPendingRequests("connection closed");
	}

	json request(const std::string& method, const json& reqData, int timeoutMs = 5000) {
		uint32_t id = nextId_.fetch_add(1);
		auto pending = std::make_shared<PendingRequest>();
		{
			std::lock_guard<std::mutex> lock(stateMutex_);
			pendingRequests_[id] = pending;
		}

		json msg = {{"request", true}, {"id", id}, {"method", method}, {"data", reqData}};
		sendText(msg.dump());

		std::unique_lock<std::mutex> lock(pending->mutex);
		bool completed = pending->cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] {
			return pending->done || !running_;
		});
		if (!completed) {
			std::lock_guard<std::mutex> stateLock(stateMutex_);
			pendingRequests_.erase(id);
			throw std::runtime_error("timeout: " + method);
		}
		if (!pending->done) {
			throw std::runtime_error(method + ": connection closed");
		}
		if (!pending->ok) {
			throw std::runtime_error(method + ": " + pending->error);
		}
		return pending->data;
	}

	bool requestAsync(const std::string& method, const json& reqData,
		std::function<void(bool, const json&, const std::string&)> completion)
	{
		if (!running_) {
			if (completion) completion(false, json::object(), "connection closed");
			return false;
		}

		uint32_t id = nextId_.fetch_add(1);
		auto pending = std::make_shared<PendingRequest>();
		pending->completion = std::move(completion);
		{
			std::lock_guard<std::mutex> lock(stateMutex_);
			pendingRequests_[id] = pending;
		}

		json msg = {{"request", true}, {"id", id}, {"method", method}, {"data", reqData}};
		sendText(msg.dump());
		return true;
	}

	size_t pendingRequestCount() const {
		std::lock_guard<std::mutex> lock(stateMutex_);
		return pendingRequests_.size();
	}

	void dispatchNotifications() {
		std::deque<json> notifications;
		{
			std::lock_guard<std::mutex> lock(stateMutex_);
			notifications.swap(pendingNotifications_);
		}
		while (!notifications.empty()) {
			auto msg = std::move(notifications.front());
			notifications.pop_front();
			if (onNotification) onNotification(msg);
		}
	}

	void handleResponse(const json& response) {
		const uint32_t id = response.value("id", 0u);
		std::shared_ptr<PendingRequest> pending;
		{
			std::lock_guard<std::mutex> lock(stateMutex_);
			auto it = pendingRequests_.find(id);
			if (it == pendingRequests_.end()) return;
			pending = it->second;
			pendingRequests_.erase(it);
		}

		std::function<void(bool, const json&, const std::string&)> completion;
		bool ok = response.value("ok", false);
		json data = response.value("data", json::object());
		std::string error = response.value("error", "unknown");
		{
			std::lock_guard<std::mutex> lock(pending->mutex);
			pending->done = true;
			pending->ok = ok;
			pending->data = data;
			pending->error = error;
			completion = pending->completion;
		}
		pending->cv.notify_all();

		if (completion) {
			completion(ok, data, error);
		}
	}

	void failAllPendingRequests(const std::string& error) {
		std::unordered_map<uint32_t, std::shared_ptr<PendingRequest>> pending;
		{
			std::lock_guard<std::mutex> lock(stateMutex_);
			pending.swap(pendingRequests_);
		}

		for (auto& entry : pending) {
			auto& req = entry.second;
			std::function<void(bool, const json&, const std::string&)> completion;
			{
				std::lock_guard<std::mutex> lock(req->mutex);
				if (req->done) continue;
				req->done = true;
				req->ok = false;
				req->error = error;
				completion = req->completion;
			}
			req->cv.notify_all();
			if (completion) completion(false, json::object(), error);
		}
	}

	void close() {
		if (running_.exchange(false)) {
			if (fd >= 0) {
				::shutdown(fd, SHUT_RDWR);
			}
			if (readerThread_.joinable()) {
				readerThread_.join();
			}
		}
		if (fd >= 0) {
			::close(fd);
			fd = -1;
		}
		failAllPendingRequests("connection closed");
	}
};

// ═══════════════════════════════════════════════════════════
// RTP packetizer
// ═══════════════════════════════════════════════════════════

static constexpr int kMaxRtpPayload = 1200;

static void rtpHeader(uint8_t* buf, uint8_t pt, uint16_t seq, uint32_t ts, uint32_t ssrc, bool marker) {
	buf[0] = 0x80;
	buf[1] = (marker ? 0x80 : 0) | (pt & 0x7F);
	buf[2] = seq >> 8; buf[3] = seq & 0xFF;
	buf[4] = ts >> 24; buf[5] = ts >> 16; buf[6] = ts >> 8; buf[7] = ts;
	buf[8] = ssrc >> 24; buf[9] = ssrc >> 16; buf[10] = ssrc >> 8; buf[11] = ssrc;
}

// Global RTCP context for RTP packet store (NACK retransmission)
static RtcpContext* g_rtcp = nullptr;

static inline void rtpSendAndStore(int fd, const uint8_t* pkt, size_t len) {
	send(fd, pkt, len, 0);
	if (g_rtcp && len >= 12) g_rtcp->onVideoRtpSent(pkt, len);
}

static void sendH264(int fd, const uint8_t* data, int size,
	uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq)
{
	std::vector<std::pair<const uint8_t*, int>> nalus;
	int i = 0;
	while (i < size) {
		int scLen = 0;
		if (i + 3 <= size && data[i]==0 && data[i+1]==0 && data[i+2]==1) scLen = 3;
		else if (i + 4 <= size && data[i]==0 && data[i+1]==0 && data[i+2]==0 && data[i+3]==1) scLen = 4;
		if (scLen) { i += scLen; continue; }
		int start = i;
		i++;
		while (i < size) {
			if (i + 3 <= size && data[i]==0 && data[i+1]==0 && (data[i+2]==1 || (data[i+2]==0 && i+3<size && data[i+3]==1)))
				break;
			i++;
		}
		if (i - start > 0) {
			nalus.push_back({data + start, i - start});
		}
	}

	for (size_t n = 0; n < nalus.size(); n++) {
		auto [nalu, nLen] = nalus[n];
		bool last = (n == nalus.size() - 1);
		if (nLen <= kMaxRtpPayload) {
			uint8_t pkt[12 + 1400];
			rtpHeader(pkt, pt, seq++, ts, ssrc, last);
			memcpy(pkt + 12, nalu, nLen);
			rtpSendAndStore(fd, pkt, 12 + nLen);
		} else {
			uint8_t naluHdr = nalu[0];
			int off = 1;
			bool first = true;
			while (off < nLen) {
				int chunk = std::min(kMaxRtpPayload - 2, nLen - off);
				bool end = (off + chunk >= nLen);
				uint8_t pkt[12 + 2 + 1400];
				rtpHeader(pkt, pt, seq++, ts, ssrc, end && last);
				pkt[12] = (naluHdr & 0x60) | 28;
				pkt[13] = (naluHdr & 0x1F) | (first ? 0x80 : 0) | (end ? 0x40 : 0);
				memcpy(pkt + 14, nalu + off, chunk);
				rtpSendAndStore(fd, pkt, 14 + chunk);
				off += chunk;
				first = false;
			}
		}
	}
}

static void sendOpus(int fd, const uint8_t* data, int size,
	uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq)
{
	if (size <= 0 || size > 4096) return;
	uint8_t pkt[12 + 4096];
	rtpHeader(pkt, pt, seq++, ts, ssrc, true);
	memcpy(pkt + 12, data, size);
	send(fd, pkt, 12 + size, 0);
}

static int resolveScaledDimension(int sourceDimension, double scaleDownBy) {
	double safeScale = scaleDownBy >= 1.0 ? scaleDownBy : 1.0;
	int scaled = static_cast<int>(sourceDimension / safeScale);
	if (scaled < 2) scaled = 2;
	if (scaled % 2 != 0) scaled -= 1;
	return std::max(2, scaled);
}

static int resolveVideoFps(const AVStream* stream, int fallbackFps = 25) {
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

static int64_t steadyNowMs() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

static int64_t wallNowMs() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
}

static std::optional<double> readFiniteNumber(const json& obj, const char* key) {
	auto it = obj.find(key);
	if (it == obj.end() || !it->is_number()) return std::nullopt;
	double value = it->get<double>();
	if (!std::isfinite(value)) return std::nullopt;
	return value;
}

static std::optional<uint64_t> readUint64Metric(const json& obj, const char* key) {
	auto value = readFiniteNumber(obj, key);
	if (!value.has_value() || *value < 0) return std::nullopt;
	return static_cast<uint64_t>(*value);
}

static double normalizeStatsTimeToMs(double value) {
	return value;
}

static double producerJitterToMs(const json& stat, double rawJitter) {
	std::string kind = stat.value("kind", "");
	std::string mimeType = stat.value("mimeType", "");
	// mediasoup producer getStats currently exposes jitter in RTP clock units.
	// If a future version includes an explicit clockRate in the stat, prefer it.
	double clockRate = readFiniteNumber(stat, "clockRate").value_or(0.0);
	if (clockRate <= 0.0) {
		clockRate = 90000.0;
		if (kind == "audio" || mimeType.rfind("audio/", 0) == 0) clockRate = 48000.0;
	}
	if (clockRate <= 0.0) return -1.0;
	return rawJitter >= 0 ? (rawJitter * 1000.0 / clockRate) : -1.0;
}

struct ServerProducerStats {
	uint64_t packetCount = 0;
	uint64_t byteCount = 0;
	uint64_t packetsLost = 0;
	double bitrateBps = 0;
	double roundTripTimeMs = -1;
	double jitterMs = -1;
};

struct CachedServerProducerStats {
	std::mutex mutex;
	std::optional<ServerProducerStats> latest;
	int64_t updatedAtSteadyMs = 0;
	int64_t requestedAtSteadyMs = 0;
	bool requestInFlight = false;
	bool lossBaseInitialized = false;
	uint64_t lossBase = 0;
};

struct CachedServerStatsResponse {
	std::mutex mutex;
	std::optional<json> latestPeerStats;
	int64_t updatedAtSteadyMs = 0;
	int64_t requestedAtSteadyMs = 0;
	bool requestInFlight = false;
};

struct VideoTrackRuntime {
	size_t index = 0;
	std::string trackId;
	std::string producerId;
	uint32_t ssrc = 0;
	uint8_t payloadType = 0;
	uint16_t seq = 0;
	double weight = 1.0;
	bool lossBaseInitialized = false;
	uint64_t lossBase = 0;
	bool videoSuppressed = false;
	bool forceNextVideoKeyframe = false;
	double nextVideoEncodePtsSec = -1.0;
	int encBitrate = 900000;
	int configuredVideoFps = 25;
	double scaleResolutionDownBy = 1.0;
	AVCodecContext* encoder = nullptr;
	AVFrame* scaledFrame = nullptr;
	SwsContext* swsCtx = nullptr;
	std::unique_ptr<qos::PublisherQosController> qosCtrl;
	bool snapshotRequested = false;
};

struct JoinThreadGuard {
	explicit JoinThreadGuard(std::thread& thread) : thread(thread) {}
	~JoinThreadGuard() {
		if (thread.joinable()) thread.join();
	}

	JoinThreadGuard(const JoinThreadGuard&) = delete;
	JoinThreadGuard& operator=(const JoinThreadGuard&) = delete;

private:
	std::thread& thread;
};

static std::optional<ServerProducerStats> parseServerProducerStats(
	const json& peerStats, const std::string& producerId, const std::string& expectedKind)
{
	auto producersIt = peerStats.find("producers");
	if (producersIt == peerStats.end() || !producersIt->is_object()) return std::nullopt;

	auto parseStatArray = [&](const json& producerEntry) -> std::optional<ServerProducerStats> {
		auto statsIt = producerEntry.find("stats");
		if (statsIt == producerEntry.end() || !statsIt->is_array()) return std::nullopt;

		std::optional<ServerProducerStats> best;
		for (const auto& stat : *statsIt) {
			if (!stat.is_object()) continue;
			if (stat.value("type", "") != "inbound-rtp") continue;
			if (!expectedKind.empty() && stat.value("kind", "") != expectedKind) continue;

			ServerProducerStats parsed;
			parsed.packetCount = readUint64Metric(stat, "packetCount").value_or(0);
			parsed.byteCount = readUint64Metric(stat, "byteCount").value_or(0);
			parsed.packetsLost = readUint64Metric(stat, "packetsLost").value_or(0);
			parsed.bitrateBps = readFiniteNumber(stat, "bitrate").value_or(0.0);
			if (auto rtt = readFiniteNumber(stat, "roundTripTime")) {
				parsed.roundTripTimeMs = normalizeStatsTimeToMs(*rtt);
			}
			if (auto jitter = readFiniteNumber(stat, "jitter")) {
				parsed.jitterMs = producerJitterToMs(stat, *jitter);
			}

			if (!best.has_value()
				|| parsed.byteCount > best->byteCount
				|| parsed.packetCount > best->packetCount
				|| parsed.bitrateBps > best->bitrateBps) {
				best = parsed;
			}
		}
		return best;
	};

	if (!producerId.empty()) {
		auto producerIt = producersIt->find(producerId);
		if (producerIt != producersIt->end() && producerIt->is_object()) {
			if (auto parsed = parseStatArray(*producerIt)) return parsed;
		}
	}

	for (auto it = producersIt->begin(); it != producersIt->end(); ++it) {
		if (!it.value().is_object()) continue;
		if (auto parsed = parseStatArray(it.value())) return parsed;
	}

	return std::nullopt;
}

struct MatrixTestPhase {
	std::string name;
	int64_t durationMs = 0;
	double sendCeilingBps = 0.0;
	double lossRate = 0.0;
	double rttMs = -1.0;
	double jitterMs = -1.0;
	qos::QualityLimitationReason qualityLimitationReason = qos::QualityLimitationReason::None;
};

struct MatrixTestProfile {
	int64_t warmupMs = 0;
	std::vector<MatrixTestPhase> phases;
};

struct MatrixTestRuntimeState {
	int64_t startMs = 0;
	int64_t lastSampleMs = 0;
	uint64_t lastPacketsSent = 0;
	uint64_t syntheticBytesSent = 0;
	uint64_t syntheticPacketsLost = 0;
	bool initialized = false;
};

static qos::QualityLimitationReason parseQualityLimitationReason(const std::string& value) {
	if (value == "bandwidth") return qos::QualityLimitationReason::Bandwidth;
	if (value == "cpu") return qos::QualityLimitationReason::Cpu;
	if (value == "other") return qos::QualityLimitationReason::Other;
	if (value == "none") return qos::QualityLimitationReason::None;
	return qos::QualityLimitationReason::Unknown;
}

static std::optional<MatrixTestProfile> loadMatrixTestProfileFromEnv() {
	const char* raw = std::getenv("QOS_TEST_MATRIX_PROFILE");
	if (!raw || std::strlen(raw) == 0) return std::nullopt;

	try {
		const auto payload = json::parse(raw);
		if (!payload.is_object()) return std::nullopt;

		MatrixTestProfile profile;
		profile.warmupMs = static_cast<int64_t>(readFiniteNumber(payload, "warmupMs").value_or(0.0));
		auto phasesIt = payload.find("phases");
		if (phasesIt == payload.end() || !phasesIt->is_array()) return std::nullopt;

		for (const auto& phaseJson : *phasesIt) {
			if (!phaseJson.is_object()) continue;
			MatrixTestPhase phase;
			phase.name = phaseJson.value("name", "");
			phase.durationMs = static_cast<int64_t>(readFiniteNumber(phaseJson, "durationMs").value_or(0.0));
			phase.sendCeilingBps = readFiniteNumber(phaseJson, "sendCeilingBps").value_or(0.0);
			phase.lossRate = readFiniteNumber(phaseJson, "lossRate").value_or(0.0);
			phase.rttMs = readFiniteNumber(phaseJson, "rttMs").value_or(-1.0);
			phase.jitterMs = readFiniteNumber(phaseJson, "jitterMs").value_or(-1.0);
			phase.qualityLimitationReason =
				parseQualityLimitationReason(phaseJson.value("qualityLimitationReason", "unknown"));
			if (phase.durationMs > 0) profile.phases.push_back(std::move(phase));
		}

		if (profile.phases.empty()) return std::nullopt;
		return profile;
	} catch (...) {
		return std::nullopt;
	}
}

static bool envFlagEnabled(const char* name) {
	const char* raw = std::getenv(name);
	if (!raw) return false;
	std::string value(raw);
	return value == "1" || value == "true" || value == "yes" || value == "on";
}

static size_t loadVideoTrackCountFromEnv() {
	const char* raw = std::getenv("PLAIN_CLIENT_VIDEO_TRACK_COUNT");
	if (!raw || std::strlen(raw) == 0) return 1;
	char* end = nullptr;
	long parsed = std::strtol(raw, &end, 10);
	if (!end || *end != '\0') return 1;
	return static_cast<size_t>(std::clamp<long>(parsed, 1, 8));
}

static std::vector<double> loadVideoTrackWeightsFromEnv(size_t trackCount) {
	std::vector<double> weights(trackCount, 1.0);
	const char* raw = std::getenv("PLAIN_CLIENT_VIDEO_TRACK_WEIGHTS");
	if (!raw || std::strlen(raw) == 0) return weights;

	std::stringstream ss(raw);
	std::string item;
	size_t index = 0;
	while (index < trackCount && std::getline(ss, item, ',')) {
		try {
			double weight = std::stod(item);
			if (std::isfinite(weight) && weight > 0.0) weights[index] = weight;
		} catch (...) {
		}
		++index;
	}

	return weights;
}

static std::optional<qos::EncodingParameters> getEncodingParametersForLevel(
	const qos::PublisherQosController& controller)
{
	const auto& profile = controller.profileConfig();
	for (const auto& step : profile.ladder) {
		if (step.level == controller.currentLevel()) return step.encodingParameters;
	}
	return std::nullopt;
}

static uint32_t getTrackDesiredBitrateBps(const VideoTrackRuntime& track) {
	if (!track.qosCtrl) return track.encBitrate > 0 ? static_cast<uint32_t>(track.encBitrate) : 0;
	auto encodingParameters = getEncodingParametersForLevel(*track.qosCtrl);
	if (encodingParameters.has_value() && encodingParameters->maxBitrateBps.has_value())
		return *encodingParameters->maxBitrateBps;
	return track.encBitrate > 0 ? static_cast<uint32_t>(track.encBitrate) : 0;
}

static uint32_t getTrackMinBitrateBps(const VideoTrackRuntime& track) {
	const uint32_t desiredBitrateBps = getTrackDesiredBitrateBps(track);
	if (desiredBitrateBps == 0) return 0;
	return std::min<uint32_t>(desiredBitrateBps, 120000u);
}

static const MatrixTestPhase* resolveMatrixTestPhase(
	const MatrixTestProfile& profile, int64_t startMs, int64_t nowMs)
{
	const int64_t elapsedMs = nowMs - startMs;
	if (elapsedMs < profile.warmupMs) return nullptr;

	int64_t phaseElapsedMs = elapsedMs - profile.warmupMs;
	for (const auto& phase : profile.phases) {
		if (phaseElapsedMs < phase.durationMs) return &phase;
		phaseElapsedMs -= phase.durationMs;
	}

	if (!profile.phases.empty()) return &profile.phases.back();
	return nullptr;
}

static std::optional<double> applyMatrixTestProfile(
	qos::RawSenderSnapshot& snap,
	int encBitrate,
	const MatrixTestProfile& profile,
	MatrixTestRuntimeState& runtime,
	int64_t nowMs)
{
	const MatrixTestPhase* phase = resolveMatrixTestPhase(profile, runtime.startMs, nowMs);
	if (!phase) return std::nullopt;

	if (!runtime.initialized) {
		runtime.initialized = true;
		runtime.lastSampleMs = nowMs;
		runtime.lastPacketsSent = snap.packetsSent;
		runtime.syntheticBytesSent = snap.bytesSent;
		runtime.syntheticPacketsLost = snap.packetsLost;
	}

	const int64_t deltaMs = std::max<int64_t>(0, nowMs - runtime.lastSampleMs);
	const uint64_t sentDelta = snap.packetsSent > runtime.lastPacketsSent
		? snap.packetsSent - runtime.lastPacketsSent
		: 0;
	const double sendCeilingBps = phase->sendCeilingBps > 0.0 ? phase->sendCeilingBps : static_cast<double>(encBitrate);
	const double mergedSendBps = std::min(static_cast<double>(encBitrate), sendCeilingBps);

	if (deltaMs > 0) {
		const uint64_t bytesDelta = static_cast<uint64_t>(std::llround(mergedSendBps * static_cast<double>(deltaMs) / 8000.0));
		runtime.syntheticBytesSent += bytesDelta;
	}

	if (sentDelta > 0 && phase->lossRate > 0.0 && phase->lossRate < 1.0) {
		const double lostDelta = (phase->lossRate / std::max(1e-9, 1.0 - phase->lossRate)) * static_cast<double>(sentDelta);
		runtime.syntheticPacketsLost += static_cast<uint64_t>(std::llround(lostDelta));
	}

	runtime.lastSampleMs = nowMs;
	runtime.lastPacketsSent = snap.packetsSent;

	snap.bytesSent = runtime.syntheticBytesSent;
	snap.packetsLost = runtime.syntheticPacketsLost;
	snap.roundTripTimeMs = phase->rttMs;
	snap.jitterMs = phase->jitterMs;
	snap.qualityLimitationReason = phase->qualityLimitationReason;

	return mergedSendBps;
}

struct TestClientStatsPayloadEntry {
	int delayMs = 0;
	json payload = json::object();
};

static std::vector<TestClientStatsPayloadEntry> loadTestClientStatsPayloadsFromEnv() {
	std::vector<TestClientStatsPayloadEntry> entries;
	const char* raw = std::getenv("QOS_TEST_CLIENT_STATS_PAYLOADS");
	if (!raw || std::strlen(raw) == 0) return entries;

	try {
		const auto payload = json::parse(raw);
		if (!payload.is_array()) return entries;

		for (const auto& item : payload) {
			if (!item.is_object()) continue;
			TestClientStatsPayloadEntry entry;
			entry.delayMs = static_cast<int>(readFiniteNumber(item, "delayMs").value_or(0.0));
			if (auto payloadIt = item.find("payload"); payloadIt != item.end() && payloadIt->is_object()) {
				entry.payload = *payloadIt;
			} else {
				entry.payload = item;
				entry.payload.erase("delayMs");
			}
			if (!entry.payload.empty()) entries.push_back(std::move(entry));
		}
	} catch (...) {
	}

	return entries;
}

struct TestWsRequestEntry {
	int delayMs = 0;
	std::string method;
	json data = json::object();
};

static std::vector<TestWsRequestEntry> loadTestWsRequestsFromEnv() {
	std::vector<TestWsRequestEntry> entries;
	const char* raw = std::getenv("QOS_TEST_SELF_REQUESTS");
	if (!raw || std::strlen(raw) == 0) return entries;

	try {
		const auto payload = json::parse(raw);
		if (!payload.is_array()) return entries;

		for (const auto& item : payload) {
			if (!item.is_object()) continue;
			TestWsRequestEntry entry;
			entry.delayMs = static_cast<int>(readFiniteNumber(item, "delayMs").value_or(0.0));
			entry.method = item.value("method", "");
			entry.data = item.value("data", json::object());
			if (!entry.method.empty()) entries.push_back(std::move(entry));
		}
	} catch (...) {
	}

	return entries;
}

// ═══════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════

int main(int argc, char* argv[]) {
	if (argc < 6) {
		fprintf(stderr, "Usage: %s SERVER_IP SERVER_PORT ROOM PEER file.mp4\n", argv[0]);
		return 1;
	}
	const char* serverIp = argv[1];
	int serverPort = atoi(argv[2]);
	const char* roomId = argv[3];
	const char* peerId = argv[4];
	const char* mp4Path = argv[5];
	bool copyMode = (argc > 6 && std::string(argv[6]) == "--copy");
	const size_t videoTrackCount = loadVideoTrackCountFromEnv();
	const auto videoTrackWeights = loadVideoTrackWeightsFromEnv(videoTrackCount);
	const auto matrixTestProfile = loadMatrixTestProfileFromEnv();
	const bool matrixLocalOnly = envFlagEnabled("QOS_TEST_MATRIX_LOCAL_ONLY");
	const auto testClientStatsPayloads = loadTestClientStatsPayloadsFromEnv();
	const auto testWsRequests = loadTestWsRequestsFromEnv();
	MatrixTestRuntimeState matrixTestRuntime;
	matrixTestRuntime.startMs = steadyNowMs();

	signal(SIGINT, onSignal);
	signal(SIGPIPE, SIG_IGN);

	// Open MP4
	AVFormatContext* fmtCtx = nullptr;
	if (avformat_open_input(&fmtCtx, mp4Path, nullptr, nullptr) < 0) {
		fprintf(stderr, "Cannot open %s\n", mp4Path); return 1;
	}
	avformat_find_stream_info(fmtCtx, nullptr);
	int vidIdx = -1, audIdx = -1;
	for (unsigned i = 0; i < fmtCtx->nb_streams; i++) {
		if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && vidIdx < 0) vidIdx = i;
		if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audIdx < 0) audIdx = i;
	}
	printf("MP4: video=%d audio=%d\n", vidIdx, audIdx);

	// Setup H264 bitstream filter (MP4 AVCC → Annex-B for RTP)
	const AVBitStreamFilter* bsf = av_bsf_get_by_name("h264_mp4toannexb");
	AVBSFContext* bsfCtx = nullptr;
	if (vidIdx >= 0 && bsf) {
		av_bsf_alloc(bsf, &bsfCtx);
		avcodec_parameters_copy(bsfCtx->par_in, fmtCtx->streams[vidIdx]->codecpar);
		bsfCtx->time_base_in = fmtCtx->streams[vidIdx]->time_base;
		av_bsf_init(bsfCtx);
	}

	// Audio transcode setup (AAC→Opus)
	AVCodecContext* adec = nullptr, *aenc = nullptr;
	AVFrame* aframe = nullptr;
	if (audIdx >= 0) {
		auto* par = fmtCtx->streams[audIdx]->codecpar;
		auto* dec = avcodec_find_decoder(par->codec_id);
		if (dec) {
			adec = avcodec_alloc_context3(dec);
			avcodec_parameters_to_context(adec, par);
			avcodec_open2(adec, dec, nullptr);
			auto* enc = avcodec_find_encoder(AV_CODEC_ID_OPUS);
			if (enc) {
				aenc = avcodec_alloc_context3(enc);
				aenc->sample_rate = 48000;
				aenc->channels = 2;
				aenc->channel_layout = AV_CH_LAYOUT_STEREO;
				aenc->bit_rate = 64000;
				aenc->sample_fmt = AV_SAMPLE_FMT_FLT;
				if (avcodec_open2(aenc, enc, nullptr) < 0) {
					avcodec_free_context(&aenc); aenc = nullptr;
					printf("WARN: Opus encoder init failed, audio disabled\n");
				}
			}
			aframe = av_frame_alloc();
		}
	}

	// Video decode → re-encode setup (for QoS bitrate control)
	AVCodecContext* vdec = nullptr;
	AVFrame* vframe = nullptr;
	int sourceVideoWidth = 0;
	int sourceVideoHeight = 0;
	std::vector<VideoTrackRuntime> videoTracks(videoTrackCount);
	for (size_t i = 0; i < videoTrackCount; ++i) {
		videoTracks[i].index = i;
		videoTracks[i].trackId = i == 0 ? "video" : ("video-" + std::to_string(i));
		videoTracks[i].weight = i < videoTrackWeights.size() ? videoTrackWeights[i] : 1.0;
	}
	auto recreateVideoEncoder = [&](VideoTrackRuntime& track, int width, int height, int fps, int bitrate) -> bool {
		auto* enc = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!enc) return false;

		auto* newEncoder = avcodec_alloc_context3(enc);
		if (!newEncoder) return false;

		const int safeFps = std::max(1, fps);
		newEncoder->width = width;
		newEncoder->height = height;
		newEncoder->pix_fmt = AV_PIX_FMT_YUV420P;
		newEncoder->time_base = {1, safeFps};
		newEncoder->framerate = {safeFps, 1};
		newEncoder->bit_rate = bitrate;
		newEncoder->rc_max_rate = bitrate;
		newEncoder->rc_buffer_size = bitrate;
		newEncoder->gop_size = safeFps;
		newEncoder->max_b_frames = 0;
		av_opt_set(newEncoder->priv_data, "preset", "ultrafast", 0);
		av_opt_set(newEncoder->priv_data, "tune", "zerolatency", 0);
		av_opt_set(newEncoder->priv_data, "profile", "baseline", 0);
		if (avcodec_open2(newEncoder, enc, nullptr) < 0) {
			avcodec_free_context(&newEncoder);
			return false;
		}

		auto* newScaledFrame = av_frame_alloc();
		if (!newScaledFrame) {
			avcodec_free_context(&newEncoder);
			return false;
		}
		newScaledFrame->format = AV_PIX_FMT_YUV420P;
		newScaledFrame->width = width;
		newScaledFrame->height = height;
		if (av_frame_get_buffer(newScaledFrame, 32) < 0) {
			av_frame_free(&newScaledFrame);
			avcodec_free_context(&newEncoder);
			return false;
		}

		if (track.encoder) avcodec_free_context(&track.encoder);
		track.encoder = newEncoder;
		if (track.scaledFrame) av_frame_free(&track.scaledFrame);
		track.scaledFrame = newScaledFrame;
		if (track.swsCtx) {
			sws_freeContext(track.swsCtx);
			track.swsCtx = nullptr;
		}
		track.configuredVideoFps = safeFps;
		return true;
	};
	if (!copyMode && vidIdx >= 0) {
		auto* par = fmtCtx->streams[vidIdx]->codecpar;
		auto* dec = avcodec_find_decoder(par->codec_id);
		if (dec) {
			vdec = avcodec_alloc_context3(dec);
			avcodec_parameters_to_context(vdec, par);
			avcodec_open2(vdec, dec, nullptr);
			sourceVideoWidth = par->width;
			sourceVideoHeight = par->height;
			const int initialVideoFps = resolveVideoFps(fmtCtx->streams[vidIdx], 25);
			for (auto& track : videoTracks) {
				track.configuredVideoFps = initialVideoFps;
				if (!recreateVideoEncoder(track, sourceVideoWidth, sourceVideoHeight, initialVideoFps, track.encBitrate)) {
					printf("WARN: x264 encoder init failed for %s, falling back to copy mode\n",
						track.trackId.c_str());
					copyMode = true;
					break;
				}
			}
			vframe = av_frame_alloc();
		}
	}
	printf("Mode: %s\n", copyMode ? "copy (no QoS bitrate control)" : "re-encode (QoS enabled)");

	// WebSocket connect
	WsClient ws;
	if (!ws.connect(serverIp, serverPort, "/ws")) {
		fprintf(stderr, "WS connect failed\n"); return 1;
	}
	printf("WS connected to %s:%d\n", serverIp, serverPort);

	try {
		// Join
		ws.request("join", {
			{"roomId", roomId}, {"peerId", peerId},
			{"displayName", peerId}, {"rtpCapabilities", json::object()}
		});
		printf("Joined room=%s peer=%s\n", roomId, peerId);

			// PlainPublish
			const uint32_t aSsrc = 22222222;
			std::vector<uint32_t> videoSsrcs(videoTracks.size(), 0);
			for (size_t i = 0; i < videoTracks.size(); ++i)
				videoSsrcs[i] = static_cast<uint32_t>(11111111u + (i * 1111u));
			json plainPublishRequest{{"audioSsrc", aSsrc}};
			if (videoSsrcs.size() == 1) plainPublishRequest["videoSsrc"] = videoSsrcs.front();
			else plainPublishRequest["videoSsrcs"] = videoSsrcs;
			auto pub = ws.request("plainPublish", plainPublishRequest);
			std::string srvIp = pub["ip"];
			uint16_t srvPort = pub["port"];
			uint8_t defaultVideoPt = pub["videoPt"], aPt = pub["audioPt"];
			std::string udpIp = serverIp;
			printf("Publish → %s:%d (announced=%s) videoTracks=%zu videoPt=%d aPt=%d\n",
				udpIp.c_str(), srvPort, srvIp.c_str(), videoTracks.size(), defaultVideoPt, aPt);

			int udp = socket(AF_INET, SOCK_DGRAM, 0);
			sockaddr_in dst{};
			dst.sin_family = AF_INET;
			dst.sin_port = htons(srvPort);
			inet_pton(AF_INET, udpIp.c_str(), &dst.sin_addr);
			::connect(udp, (sockaddr*)&dst, sizeof(dst));

			{
				int flags = fcntl(udp, F_GETFL, 0);
				fcntl(udp, F_SETFL, flags | O_NONBLOCK);
			}

			uint16_t aSeq = 0;

				RtcpContext rtcp;
				rtcp.audioSsrc = aSsrc;
				rtcp.sendH264Fn = sendH264;
				rtcp.canSendVideoFn = [&](uint32_t ssrc) {
					for (const auto& track : videoTracks) {
						if (track.ssrc == ssrc) return !track.videoSuppressed;
					}
					return true;
				};
				g_rtcp = &rtcp;

			auto videoTracksJson = pub.value("videoTracks", json::array());
			for (size_t i = 0; i < videoTracks.size(); ++i) {
				const auto& trackJson = (videoTracksJson.is_array() && i < videoTracksJson.size())
					? videoTracksJson.at(i)
					: json::object();
				videoTracks[i].ssrc = trackJson.value("ssrc", videoSsrcs[i]);
				videoTracks[i].payloadType = static_cast<uint8_t>(trackJson.value("pt", defaultVideoPt));
				videoTracks[i].producerId = trackJson.value("producerId",
					i == 0 ? pub.value("videoProdId", "") : "");
				rtcp.registerVideoStream(videoTracks[i].ssrc, videoTracks[i].payloadType, &videoTracks[i].seq);
			}

			for (auto& track : videoTracks) {
				for (int i = 0; i < 5; ++i) {
					uint8_t dummy[12 + 1];
					rtpHeader(dummy, track.payloadType, track.seq++, 0, track.ssrc, false);
					dummy[12] = 0;
					send(udp, dummy, 13, 0);
					std::this_thread::sleep_for(std::chrono::milliseconds(20));
				}
			}
			printf("Sent probe packets for comedia across %zu video tracks\n", videoTracks.size());
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

				auto cachedServerStatsResponse = std::make_shared<CachedServerStatsResponse>();
				for (auto& track : videoTracks) {
					if (copyMode || !track.encoder) continue;
					auto* trackPtr = &track;
					qos::PublisherQosController::Options qosOptions;
					qosOptions.source = qos::Source::Camera;
					qosOptions.trackId = track.trackId;
					qosOptions.producerId = track.producerId;
					qosOptions.initialLevel = 0;
					qosOptions.peerHasAudioTrack = (audIdx >= 0 && aenc != nullptr);
					qosOptions.actionSink =
						[trackPtr, &recreateVideoEncoder, sourceVideoWidth, sourceVideoHeight](const qos::PlannedAction& action) -> bool {
							auto& track = *trackPtr;
							if (action.type == qos::ActionType::SetEncodingParameters && action.encodingParameters.has_value()) {
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
								const int nextWidth = resolveScaledDimension(sourceVideoWidth, nextScale);
								const int nextHeight = resolveScaledDimension(sourceVideoHeight, nextScale);
								const bool requiresRecreate = !track.encoder
									|| nextWidth != track.encoder->width
									|| nextHeight != track.encoder->height
									|| nextFps != track.configuredVideoFps;

								if (requiresRecreate) {
									if (!recreateVideoEncoder(track, nextWidth, nextHeight, nextFps, nextBitrate)) {
										printf("[QoS:%s] encoder reconfigure failed (br=%d fps=%d scale=%.2f size=%dx%d)\n",
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
									track.encoder->bit_rate = track.encBitrate;
									track.encoder->rc_max_rate = track.encBitrate;
									track.encoder->rc_buffer_size = track.encBitrate;
								}
								printf("[QoS:%s] encoder params → br=%d fps=%d scale=%.2f size=%dx%d\n",
									track.trackId.c_str(),
									track.encBitrate,
									track.configuredVideoFps,
									track.scaleResolutionDownBy,
									track.encoder ? track.encoder->width : nextWidth,
									track.encoder ? track.encoder->height : nextHeight);
							} else if (action.type == qos::ActionType::SetMaxSpatialLayer) {
								printf("[QoS:%s] spatial layer request ignored in single-layer plain client\n",
									track.trackId.c_str());
							} else if (action.type == qos::ActionType::EnterAudioOnly || action.type == qos::ActionType::PauseUpstream) {
								if (!track.videoSuppressed) {
									track.videoSuppressed = true;
									printf("[QoS:%s] video suppressed (%s)\n", track.trackId.c_str(), actionStr(action.type));
								}
							} else if (action.type == qos::ActionType::ExitAudioOnly || action.type == qos::ActionType::ResumeUpstream) {
								if (track.videoSuppressed) {
									track.videoSuppressed = false;
									track.forceNextVideoKeyframe = true;
									track.nextVideoEncodePtsSec = -1.0;
									printf("[QoS:%s] video resumed (%s)\n", track.trackId.c_str(), actionStr(action.type));
								}
							}
							return true;
						};
						qosOptions.sendSnapshot =
							[trackPtr, &testClientStatsPayloads, matrixLocalOnly](const nlohmann::json&) {
								if (!testClientStatsPayloads.empty()) return;
								if (matrixLocalOnly) return;
								trackPtr->snapshotRequested = true;
							};
						track.qosCtrl = std::make_unique<qos::PublisherQosController>(qosOptions);
					}

			ws.onNotification = [&](const json& msg) {
				std::string method = msg.value("method", "");
				auto data = msg.value("data", json::object());
				if (method == "qosPolicy") {
					try {
						auto policy = qos::parseQosPolicy(data);
						for (auto& track : videoTracks) if (track.qosCtrl) track.qosCtrl->handlePolicy(policy);
						printf("[QoS] policy updated: sampleMs=%d snapshotMs=%d\n", policy.sampleIntervalMs, policy.snapshotIntervalMs);
					} catch (const std::exception& e) {
						printf("[QoS] policy parse error: %s\n", e.what());
					}
				} else if (method == "qosOverride" && !matrixLocalOnly) {
					try {
						auto override = qos::parseQosOverride(data);
						for (auto& track : videoTracks) if (track.qosCtrl) track.qosCtrl->handleOverride(override);
						printf("[QoS] override: reason=%s ttl=%d\n", override.reason.c_str(), override.ttlMs);
					} catch (const std::exception& e) {
						printf("[QoS] override parse error: %s\n", e.what());
					}
				}
			};
			ws.dispatchNotifications();

			auto requestServerProducerStats = [&]() {
				bool hasVideoQos = false;
				for (const auto& track : videoTracks) {
					if (track.qosCtrl && !track.producerId.empty()) {
						hasVideoQos = true;
						break;
					}
				}
				if (!hasVideoQos) return;

				const int64_t now = steadyNowMs();
				{
					std::lock_guard<std::mutex> lock(cachedServerStatsResponse->mutex);
					if (cachedServerStatsResponse->requestInFlight
						|| now - cachedServerStatsResponse->requestedAtSteadyMs < 500) {
						return;
					}
					cachedServerStatsResponse->requestInFlight = true;
					cachedServerStatsResponse->requestedAtSteadyMs = now;
				}

				bool queued = ws.requestAsync("getStats", json::object(),
					[cachedServerStatsResponse](bool ok, const json& data, const std::string& error) {
						{
							std::lock_guard<std::mutex> lock(cachedServerStatsResponse->mutex);
							cachedServerStatsResponse->requestInFlight = false;
							if (ok && data.is_object()) {
								cachedServerStatsResponse->latestPeerStats = data;
								cachedServerStatsResponse->updatedAtSteadyMs = steadyNowMs();
							}
						}

						if (!ok) printf("[QoS] getStats failed: %s\n", error.c_str());
					});

				if (!queued) {
					std::lock_guard<std::mutex> lock(cachedServerStatsResponse->mutex);
					cachedServerStatsResponse->requestInFlight = false;
				}
			};
			requestServerProducerStats();

		std::thread testClientStatsThread;
		std::unique_ptr<JoinThreadGuard> testClientStatsThreadGuard;
		if (!testClientStatsPayloads.empty()) {
			testClientStatsThread = std::thread([&]() {
				for (const auto& entry : testClientStatsPayloads) {
					if (!g_running.load()) break;
					if (entry.delayMs > 0) {
						std::this_thread::sleep_for(std::chrono::milliseconds(entry.delayMs));
					}
					if (!g_running.load()) break;
					ws.requestAsync("clientStats", entry.payload,
						[](bool ok, const json&, const std::string& error) {
							if (!ok) printf("[QoS] test clientStats failed: %s\n", error.c_str());
						});
				}
			});
			testClientStatsThreadGuard = std::make_unique<JoinThreadGuard>(testClientStatsThread);
		}

		std::thread testWsRequestsThread;
		std::unique_ptr<JoinThreadGuard> testWsRequestsThreadGuard;
		if (!testWsRequests.empty()) {
			testWsRequestsThread = std::thread([&]() {
				for (const auto& entry : testWsRequests) {
					if (!g_running.load()) break;
					if (entry.delayMs > 0) {
						std::this_thread::sleep_for(std::chrono::milliseconds(entry.delayMs));
					}
					if (!g_running.load()) break;
					ws.requestAsync(entry.method, entry.data,
						[method = entry.method](bool ok, const json&, const std::string& error) {
							if (!ok) printf("[QoS] test %s failed: %s\n", method.c_str(), error.c_str());
						});
				}
			});
			testWsRequestsThreadGuard = std::make_unique<JoinThreadGuard>(testWsRequestsThread);
		}

			int64_t lastPeerQosSampleMs = 0;

		// Stream
		AVPacket* pkt = av_packet_alloc();
		auto t0 = std::chrono::steady_clock::now();
		double firstPts = -1;
		int frameCnt = 0;

		while (g_running && av_read_frame(fmtCtx, pkt) >= 0) {
			auto* st = fmtCtx->streams[pkt->stream_index];
			double pts = pkt->pts * av_q2d(st->time_base);
			if (firstPts < 0) firstPts = pts;
			double elapsed = pts - firstPts;

			// Pace
			auto target = t0 + std::chrono::microseconds((int64_t)(elapsed * 1e6));
			std::this_thread::sleep_until(target);
				ws.dispatchNotifications();

				if (pkt->stream_index == vidIdx) {
					uint32_t rtpTs = (uint32_t)(pts * 90000);
					if (!copyMode && vdec && !videoTracks.empty()) {
						// Re-encode path: decode → encode with QoS-controlled bitrate
						if (avcodec_send_packet(vdec, pkt) == 0) {
							while (avcodec_receive_frame(vdec, vframe) == 0) {
								double framePtsSec = pts;
								if (vframe->best_effort_timestamp != AV_NOPTS_VALUE) {
									framePtsSec = vframe->best_effort_timestamp * av_q2d(fmtCtx->streams[vidIdx]->time_base);
								}

								for (auto& track : videoTracks) {
									if (!track.encoder) continue;

									if (track.configuredVideoFps > 0 && !track.forceNextVideoKeyframe) {
										if (track.nextVideoEncodePtsSec < 0) track.nextVideoEncodePtsSec = framePtsSec;
										const double minEncodeIntervalSec = 1.0 / track.configuredVideoFps;
										if (framePtsSec + 1e-6 < track.nextVideoEncodePtsSec) continue;
										track.nextVideoEncodePtsSec = framePtsSec + minEncodeIntervalSec;
									}

									if (track.videoSuppressed) continue;

									AVFrame* frameToEncode = vframe;
									if (track.encoder->width != vframe->width
										|| track.encoder->height != vframe->height
										|| vframe->format != AV_PIX_FMT_YUV420P) {
										if (av_frame_make_writable(track.scaledFrame) < 0) continue;
										track.swsCtx = sws_getCachedContext(
											track.swsCtx,
											vframe->width,
											vframe->height,
											static_cast<AVPixelFormat>(vframe->format),
											track.encoder->width,
											track.encoder->height,
											AV_PIX_FMT_YUV420P,
											SWS_BILINEAR,
											nullptr,
											nullptr,
											nullptr);
										if (!track.swsCtx) continue;
										sws_scale(
											track.swsCtx,
											vframe->data,
											vframe->linesize,
											0,
											vframe->height,
											track.scaledFrame->data,
											track.scaledFrame->linesize);
										track.scaledFrame->pts = vframe->pts;
										frameToEncode = track.scaledFrame;
									}

									if (track.forceNextVideoKeyframe) {
										frameToEncode->pict_type = AV_PICTURE_TYPE_I;
										track.forceNextVideoKeyframe = false;
									} else {
										frameToEncode->pict_type = AV_PICTURE_TYPE_NONE;
									}

									if (avcodec_send_frame(track.encoder, frameToEncode) == 0) {
										AVPacket* encPkt = av_packet_alloc();
										while (avcodec_receive_packet(track.encoder, encPkt) == 0) {
											if (encPkt->flags & AV_PKT_FLAG_KEY) {
												rtcp.cacheKeyframe(track.ssrc, encPkt->data, encPkt->size, rtpTs);
											}
											if (!track.videoSuppressed) {
												sendH264(udp, encPkt->data, encPkt->size,
													track.payloadType, rtpTs, track.ssrc, track.seq);
											}
											av_packet_unref(encPkt);
										}
										av_packet_free(&encPkt);
									}
								}
							}
						}
					} else {
						// Copy path: BSF convert AVCC→Annex-B
						if (bsfCtx) {
						AVPacket* bsfPkt = av_packet_alloc();
						av_packet_ref(bsfPkt, pkt);
						av_bsf_send_packet(bsfCtx, bsfPkt);
							while (av_bsf_receive_packet(bsfCtx, bsfPkt) == 0) {
								for (auto& track : videoTracks) {
									if (bsfPkt->flags & AV_PKT_FLAG_KEY)
										rtcp.cacheKeyframe(track.ssrc, bsfPkt->data, bsfPkt->size, rtpTs);
									if (!track.videoSuppressed) {
										sendH264(udp, bsfPkt->data, bsfPkt->size,
											track.payloadType, rtpTs, track.ssrc, track.seq);
									}
								}
								av_packet_unref(bsfPkt);
							}
							av_packet_free(&bsfPkt);
						} else {
							for (auto& track : videoTracks) {
								if (pkt->flags & AV_PKT_FLAG_KEY)
									rtcp.cacheKeyframe(track.ssrc, pkt->data, pkt->size, rtpTs);
								if (!track.videoSuppressed) {
									sendH264(udp, pkt->data, pkt->size,
										track.payloadType, rtpTs, track.ssrc, track.seq);
								}
							}
						}
					}

					// Process incoming RTCP (PLI + NACK) and send periodic SR
					rtcp.processIncomingRtcp(udp);
					rtcp.maybeSendSR(udp);

					// QoS sampling (peer-level coordination across all video tracks)
					bool hasVideoQos = false;
					int sampleIntervalMs = std::numeric_limits<int>::max();
					for (const auto& track : videoTracks) {
						if (!track.qosCtrl) continue;
						hasVideoQos = true;
						sampleIntervalMs = std::min(sampleIntervalMs, track.qosCtrl->getRuntimeSettings().sampleIntervalMs);
					}

					if (hasVideoQos && sampleIntervalMs < std::numeric_limits<int>::max()) {
						auto now = steadyNowMs();
						if (now - lastPeerQosSampleMs >= sampleIntervalMs) {
							lastPeerQosSampleMs = now;
							requestServerProducerStats();

							std::optional<json> cachedPeerStats;
							{
								std::lock_guard<std::mutex> lock(cachedServerStatsResponse->mutex);
								const int64_t maxStatsAgeMs = std::max<int64_t>(5000, static_cast<int64_t>(sampleIntervalMs) * 4);
								if (cachedServerStatsResponse->latestPeerStats.has_value()
									&& now - cachedServerStatsResponse->updatedAtSteadyMs <= maxStatsAgeMs) {
									cachedPeerStats = *cachedServerStatsResponse->latestPeerStats;
								}
							}

							std::vector<qos::PeerTrackState> peerTrackStates;
							std::map<std::string, qos::DerivedSignals> peerTrackSignals;
							std::map<std::string, qos::PlannedAction> peerLastActions;
							std::map<std::string, qos::RawSenderSnapshot> peerRawSnapshots;
							std::map<std::string, bool> peerActionApplied;
							std::vector<qos::TrackBudgetRequest> trackBudgetRequests;
							uint64_t totalDesiredBudgetBps = 0;
							uint64_t totalObservedBudgetBps = 0;
							bool anyBandwidthLimited = false;
							bool peerSnapshotRequested = false;

							for (auto& track : videoTracks) {
								if (!track.qosCtrl) continue;
								track.snapshotRequested = false;

								qos::RawSenderSnapshot snap;
								snap.timestampMs = now;
								snap.trackId = track.trackId;
								snap.producerId = track.producerId;
								snap.source = qos::Source::Camera;
								snap.kind = qos::TrackKind::Video;
								snap.targetBitrateBps = track.encBitrate;
								snap.configuredBitrateBps = track.encBitrate;

								if (const auto* rtcpStream = rtcp.getVideoStream(track.ssrc)) {
									snap.bytesSent = rtcpStream->octetCount;
									snap.packetsSent = rtcpStream->packetCount;
									snap.packetsLost = rtcpStream->rrCumulativeLost;
									snap.roundTripTimeMs = rtcpStream->rrRttMs;
									snap.jitterMs = rtcpStream->rrJitterMs;
								}

								bool usingServerStats = false;
								bool usingMatrixTestProfile = false;
								double observedBitrateBps = 0.0;
								if (cachedPeerStats.has_value() && !track.producerId.empty()) {
									auto parsed = parseServerProducerStats(*cachedPeerStats, track.producerId, "video");
									if (parsed.has_value()) {
										usingServerStats = true;
										if (!track.lossBaseInitialized) {
											track.lossBase = parsed->packetsLost;
											track.lossBaseInitialized = true;
										}
										if (parsed->packetsLost >= track.lossBase) {
											parsed->packetsLost -= track.lossBase;
										} else {
											track.lossBase = parsed->packetsLost;
											parsed->packetsLost = 0;
										}

										if (parsed->byteCount > 0) snap.bytesSent = parsed->byteCount;
										if (parsed->packetCount > 0) snap.packetsSent = parsed->packetCount;
										snap.packetsLost = parsed->packetsLost;
										if (parsed->roundTripTimeMs >= 0) snap.roundTripTimeMs = parsed->roundTripTimeMs;
										if (parsed->jitterMs >= 0) snap.jitterMs = parsed->jitterMs;
										observedBitrateBps = parsed->bitrateBps;
									}
								}
								if (matrixTestProfile.has_value() && track.index == 0) {
									if (auto syntheticSendBps = applyMatrixTestProfile(
										snap,
										track.encBitrate,
										*matrixTestProfile,
										matrixTestRuntime,
										now)) {
										observedBitrateBps = *syntheticSendBps;
										usingMatrixTestProfile = true;
									}
								}
								if (track.encoder) {
									snap.frameWidth = track.encoder->width;
									snap.frameHeight = track.encoder->height;
									snap.framesPerSecond = track.configuredVideoFps;
								}
								if (observedBitrateBps > 0.0
									&& snap.targetBitrateBps > 0
									&& observedBitrateBps < snap.targetBitrateBps * qos::NETWORK_CONGESTED_UTILIZATION) {
									snap.qualityLimitationReason = qos::QualityLimitationReason::Bandwidth;
									anyBandwidthLimited = true;
								}

								track.qosCtrl->onSample(snap);
								peerTrackStates.push_back(track.qosCtrl->getTrackState());
								if (auto derivedSignals = track.qosCtrl->lastSignals()) {
									peerTrackSignals[track.trackId] = *derivedSignals;
								}
								peerLastActions[track.trackId] = track.qosCtrl->lastAction();
								peerRawSnapshots[track.trackId] = snap;
								peerActionApplied[track.trackId] = track.qosCtrl->lastActionWasApplied();
								peerSnapshotRequested = peerSnapshotRequested || track.snapshotRequested;

								const uint32_t desiredBitrateBps = getTrackDesiredBitrateBps(track);
								totalDesiredBudgetBps += desiredBitrateBps;
								totalObservedBudgetBps += observedBitrateBps > 0.0
									? static_cast<uint64_t>(std::llround(observedBitrateBps))
									: desiredBitrateBps;
								trackBudgetRequests.push_back({
									track.trackId,
									qos::Source::Camera,
									qos::TrackKind::Video,
									getTrackMinBitrateBps(track),
									desiredBitrateBps,
									desiredBitrateBps,
									track.weight,
									false
								});

								printf("[QOS_TRACE] tsMs=%lld track=%s state=%s level=%d mode=%s sample=%s bitrateBps=%d sendBps=%.0f packetsLost=%llu rttMs=%.1f jitterMs=%.1f width=%d height=%d fps=%d suppressed=%d\n",
									static_cast<long long>(wallNowMs()),
									track.trackId.c_str(),
									qos::stateStr(track.qosCtrl->currentState()),
									track.qosCtrl->currentLevel(),
									track.videoSuppressed ? "audio-only" : "audio-video",
									usingMatrixTestProfile ? "matrix" : (usingServerStats ? "server" : "local"),
									track.encBitrate,
									observedBitrateBps > 0.0 ? observedBitrateBps : snap.targetBitrateBps,
									static_cast<unsigned long long>(snap.packetsLost),
									snap.roundTripTimeMs,
									snap.jitterMs,
									track.encoder ? track.encoder->width : 0,
									track.encoder ? track.encoder->height : 0,
									track.configuredVideoFps,
									track.videoSuppressed ? 1 : 0);
							}

							if (trackBudgetRequests.size() > 1) {
								uint32_t totalBudgetBps = static_cast<uint32_t>(
									std::min<uint64_t>(std::numeric_limits<uint32_t>::max(), totalDesiredBudgetBps));
								if (anyBandwidthLimited && totalObservedBudgetBps > 0) {
									totalBudgetBps = std::min<uint32_t>(totalBudgetBps, static_cast<uint32_t>(
										std::min<uint64_t>(std::numeric_limits<uint32_t>::max(), totalObservedBudgetBps)));
								}
								auto budgetDecision = qos::allocatePeerTrackBudgets(totalBudgetBps, trackBudgetRequests);
								auto coordinationOverrides = qos::buildCoordinationOverrides(peerTrackStates, budgetDecision);
								for (auto& track : videoTracks) {
									if (!track.qosCtrl) continue;
									auto overrideIt = coordinationOverrides.find(track.trackId);
									if (overrideIt != coordinationOverrides.end()) track.qosCtrl->setCoordinationOverride(overrideIt->second);
									else track.qosCtrl->setCoordinationOverride(std::nullopt);
								}
							} else {
								for (auto& track : videoTracks) {
									if (track.qosCtrl) track.qosCtrl->setCoordinationOverride(std::nullopt);
								}
							}

							if (peerSnapshotRequested && !peerTrackStates.empty()
								&& testClientStatsPayloads.empty() && !matrixLocalOnly) {
								static constexpr size_t kMaxPendingClientStats = 8;
								static int peerSnapshotSeq = 0;
								if (ws.pendingRequestCount() >= kMaxPendingClientStats) {
									printf("[QoS] clientStats dropped: too many pending requests (%zu)\n",
										ws.pendingRequestCount());
								} else {
									auto peerDecision = qos::buildPeerDecision(peerTrackStates);
									auto peerSnapshot = qos::serializeSnapshot(
										peerSnapshotSeq++,
										wallNowMs(),
										peerDecision.peerQuality,
										false,
										peerTrackStates,
										peerTrackSignals,
										peerLastActions,
										peerRawSnapshots,
										peerActionApplied,
										(audIdx >= 0 && aenc != nullptr));
									ws.requestAsync("clientStats", peerSnapshot,
										[](bool ok, const json&, const std::string& error) {
											if (!ok) printf("[QoS] clientStats failed: %s\n", error.c_str());
										});
								}
							}
						}
					}

					if (++frameCnt % 100 == 0)
						printf("sent %d frames [nack=%d pli=%d tracks=%zu]\n",
							frameCnt, rtcp.nackRetransmitted, rtcp.pliResponded,
							videoTracks.size());
				} else if (pkt->stream_index == audIdx && adec && aenc) {
				if (avcodec_send_packet(adec, pkt) == 0) {
					while (avcodec_receive_frame(adec, aframe) == 0) {
						if (avcodec_send_frame(aenc, aframe) == 0) {
							AVPacket* ep = av_packet_alloc();
							while (avcodec_receive_packet(aenc, ep) == 0) {
								sendOpus(udp, ep->data, ep->size, aPt, (uint32_t)(pts * 48000), aSsrc, aSeq);
								rtcp.onAudioRtpSent(ep->size, (uint32_t)(pts * 48000));
								av_packet_unref(ep);
							}
							av_packet_free(&ep);
						}
					}
				}
			}
			av_packet_unref(pkt);
		}

		g_rtcp = nullptr;
		if (testClientStatsThread.joinable()) {
			testClientStatsThread.join();
		}
		if (testWsRequestsThread.joinable()) {
			testWsRequestsThread.join();
		}
		av_packet_free(&pkt);
		::close(udp);
		printf("Done: %d video frames sent\n", frameCnt);

	} catch (const std::exception& e) {
		fprintf(stderr, "Error: %s\n", e.what());
	}

	ws.close();
	if (bsfCtx) av_bsf_free(&bsfCtx);
	if (vframe) av_frame_free(&vframe);
	for (auto& track : videoTracks) {
		if (track.scaledFrame) av_frame_free(&track.scaledFrame);
		if (track.encoder) avcodec_free_context(&track.encoder);
		if (track.swsCtx) sws_freeContext(track.swsCtx);
	}
	if (vdec) avcodec_free_context(&vdec);
	if (aframe) av_frame_free(&aframe);
	if (aenc) avcodec_free_context(&aenc);
	if (adec) avcodec_free_context(&adec);
	avformat_close_input(&fmtCtx);
	return 0;
}
