// ThreadTypes.h — Value objects and SPSC queue for multi-thread message boundaries
// Phase 1 of linux-client-multi-source-thread-model migration.
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mt {

// ═══════════════════════════════════════════════════════════
// SPSC (Single-Producer Single-Consumer) lock-free queue
// ═══════════════════════════════════════════════════════════
template <typename T, size_t Capacity>
class SpscQueue {
public:
	bool tryPush(T&& item) {
		size_t head = head_.load(std::memory_order_relaxed);
		size_t next = (head + 1) % (Capacity + 1);
		if (next == tail_.load(std::memory_order_acquire)) return false;
		buf_[head] = std::move(item);
		head_.store(next, std::memory_order_release);
		return true;
	}

	bool tryPop(T& item) {
		size_t tail = tail_.load(std::memory_order_relaxed);
		if (tail == head_.load(std::memory_order_acquire)) return false;
		item = std::move(buf_[tail]);
		tail_.store((tail + 1) % (Capacity + 1), std::memory_order_release);
		return true;
	}

	bool empty() const {
		return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
	}

private:
	T buf_[Capacity + 1];
	alignas(64) std::atomic<size_t> head_{0};
	alignas(64) std::atomic<size_t> tail_{0};
};

// ═══════════════════════════════════════════════════════════
// source worker -> network thread
// ═══════════════════════════════════════════════════════════
struct EncodedAccessUnit {
	uint32_t trackIndex = 0;
	uint32_t ssrc = 0;
	uint8_t payloadType = 0;
	uint32_t rtpTimestamp = 0;
	double ptsSec = 0.0;
	bool isKeyframe = false;
	bool encoderRecreated = false;
	uint64_t configGeneration = 0;
	std::unique_ptr<uint8_t[]> data;
	size_t size = 0;

	EncodedAccessUnit() = default;
	EncodedAccessUnit(EncodedAccessUnit&&) = default;
	EncodedAccessUnit& operator=(EncodedAccessUnit&&) = default;

	void assign(const uint8_t* src, size_t len) {
		data = std::make_unique<uint8_t[]>(len);
		std::memcpy(data.get(), src, len);
		size = len;
	}
};

// ═══════════════════════════════════════════════════════════
// network thread -> control thread
// ═══════════════════════════════════════════════════════════
struct SenderStatsSnapshot {
	int64_t timestampMs = 0;
	uint32_t trackIndex = 0;
	uint32_t ssrc = 0;
	uint64_t generation = 0;  // monotonic counter — detect staleness
	uint64_t packetCount = 0;
	uint64_t octetCount = 0;
	uint32_t rrCumulativeLost = 0;
	double rrLossFraction = 0.0;
	double rrRttMs = -1.0;
	double rrJitterMs = -1.0;
	int64_t lastRrReceivedMs = 0;
};

// ═══════════════════════════════════════════════════════════
// control thread -> source worker
// ═══════════════════════════════════════════════════════════
enum class TrackCommandType {
	SetEncodingParameters,
	ForceKeyframe,
	PauseTrack,
	ResumeTrack,
	StopSource,
};

struct TrackControlCommand {
	TrackCommandType type = TrackCommandType::ForceKeyframe;
	uint32_t trackIndex = 0;
	uint64_t commandId = 0;
	uint64_t configGeneration = 0;
	int64_t issuedAtMs = 0;
	// SetEncodingParameters fields (latest-wins semantics)
	int bitrateBps = 0;
	int fps = 0;
	double scaleResolutionDownBy = 1.0;
};

// ═══════════════════════════════════════════════════════════
// network thread -> source worker (direct, bypasses control)
// ═══════════════════════════════════════════════════════════
struct NetworkToSourceCommand {
	enum Type { ForceKeyframe, PauseTrack, ResumeTrack, SourceCongestedHint };
	Type type = ForceKeyframe;
	uint32_t trackIndex = 0;
};

// ═══════════════════════════════════════════════════════════
// control thread -> network thread
// ═══════════════════════════════════════════════════════════
struct NetworkControlCommand {
	enum Type { TrackTransportConfig, PauseTrack, ResumeTrack, Shutdown };
	Type type = Shutdown;
	uint32_t trackIndex = 0;
	uint64_t commandId = 0;
	// TrackTransportConfig fields
	uint32_t ssrc = 0;
	uint8_t payloadType = 0;
};

// ═══════════════════════════════════════════════════════════
// source worker -> control thread (command acknowledgement)
// ═══════════════════════════════════════════════════════════
struct CommandAck {
	uint32_t trackIndex = 0;
	TrackCommandType type = TrackCommandType::ForceKeyframe;
	uint64_t commandId = 0;
	uint64_t configGeneration = 0;
	bool applied = false;
	std::string reason;
	int64_t appliedAtMs = 0;
	int actualBitrateBps = 0;
	int actualFps = 0;
	int actualWidth = 0;
	int actualHeight = 0;
	double actualScale = 1.0;
};

// ═══════════════════════════════════════════════════════════
// Queue capacity constants
// ═══════════════════════════════════════════════════════════
// ~200ms of 720p@30fps ≈ 6 frames; with RTP fragmentation headroom
static constexpr size_t kEncodedAuQueueCapacity = 16;
static constexpr size_t kControlCommandQueueCapacity = 32;
static constexpr size_t kStatsQueueCapacity = 8;
static constexpr size_t kNetworkSourceQueueCapacity = 16;
static constexpr size_t kCommandAckQueueCapacity = 16;

} // namespace mt
