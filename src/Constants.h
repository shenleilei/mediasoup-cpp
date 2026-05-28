#pragma once
#include <cstdint>

namespace mediasoup {

// ── Timers ──
constexpr int kGcIntervalMs              = 30000;   // idle room cleanup
constexpr int kHealthCheckIntervalMs     = 2000;    // dead router detection
constexpr int kStatsBroadcastIntervalMs  = 5000;    // stats push to clients
constexpr int kShutdownPollIntervalMs    = 500;

// ── Rooms ──
constexpr int kIdleRoomTimeoutSec        = 30;

// ── Workers ──
constexpr int kMaxRespawnsPerWindow      = 3;
constexpr int kRespawnWindowSec          = 10;
constexpr int kRespawnDelayMs            = 500;

// ── Transport ──
constexpr uint32_t kInitialOutgoingBitrate = 600000;
constexpr uint8_t  kIceConsentTimeout      = 30;
constexpr uint16_t kRtcMinPort             = 10000;
constexpr uint16_t kRtcMaxPort             = 59999;

// ── WebSocket ──
constexpr int kWsIdleTimeoutSec          = 120;
constexpr int kWsMaxPayloadBytes         = 16 * 1024;

// ── Channel IPC ──
constexpr int kChannelRequestTimeoutMs   = 5000;    // worker IPC request timeout

// ── Stats ──
constexpr int kStatsTimeoutMs            = 500;     // per-peer getStats IPC timeout

// ── Disk ──
// ── Redis (legacy implementation files remain out of runtime path) ──
constexpr int kRedisConnectTimeoutSec    = 2;
constexpr int kRedisHeartbeatIntervalSec = 10;
constexpr int kRedisRoomTtlSec           = 300;
constexpr int kRedisNodeTtlSec           = 30;

} // namespace mediasoup
