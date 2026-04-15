#pragma once
namespace qos {
constexpr int DEFAULT_SAMPLE_INTERVAL_MS = 1000;
constexpr int DEFAULT_SNAPSHOT_INTERVAL_MS = 2000;
constexpr int DEFAULT_RECOVERY_COOLDOWN_MS = 8000;
constexpr int DEFAULT_TRACE_BUFFER_SIZE = 256;
constexpr double EWMA_ALPHA = 0.35;
constexpr double NETWORK_WARN_LOSS_RATE = 0.04;
constexpr double NETWORK_WARN_RTT_MS = 220;
constexpr double NETWORK_CONGESTED_UTILIZATION = 0.65;
constexpr int CPU_REASON_MIN_SAMPLES = 2;
} // namespace qos
