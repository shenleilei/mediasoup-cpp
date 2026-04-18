#pragma once

#include <cstddef>

namespace mediasoup::qos::contract {

inline constexpr char kClientSchema[] = "mediasoup.qos.client.v1";
inline constexpr char kPolicySchema[] = "mediasoup.qos.policy.v1";
inline constexpr char kOverrideSchema[] = "mediasoup.qos.override.v1";

inline constexpr std::size_t kMaxTracksPerSnapshot = 32u;

} // namespace mediasoup::qos::contract
