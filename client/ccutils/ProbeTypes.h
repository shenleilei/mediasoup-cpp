#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>

namespace mediasoup::ccutils {

using ProbeClusterId = uint32_t;
static constexpr ProbeClusterId ProbeClusterIdInvalid = 0;

struct ProbeClusterGoal {
	int availableBandwidthBps{ 0 };
	int expectedUsageBps{ 0 };
	int desiredBps{ 0 };
	std::chrono::microseconds duration{ 0 };
	int desiredBytes{ 0 };
};

struct ProbeClusterResult {
	int64_t startTimeUs{ 0 };
	int64_t endTimeUs{ 0 };
	int packetsProbe{ 0 };
	int bytesProbe{ 0 };
	int packetsNonProbePrimary{ 0 };
	int bytesNonProbePrimary{ 0 };
	int packetsNonProbeRtx{ 0 };
	int bytesNonProbeRtx{ 0 };
	bool isCompleted{ false };

	int Bytes() const
	{
		return bytesProbe + bytesNonProbePrimary + bytesNonProbeRtx;
	}

	std::chrono::microseconds Duration() const
	{
		return std::chrono::microseconds(std::max<int64_t>(0, endTimeUs - startTimeUs));
	}

	double Bitrate() const
	{
		const double seconds = Duration().count() / 1000000.0;
		if (seconds <= 0.0) {
			return 0.0;
		}
		return static_cast<double>(Bytes() * 8) / seconds;
	}
};

struct ProbeClusterInfo {
	ProbeClusterId id{ ProbeClusterIdInvalid };
	std::chrono::steady_clock::time_point createdAt{};
	ProbeClusterGoal goal;
	ProbeClusterResult result;

	bool IsValid() const
	{
		return id != ProbeClusterIdInvalid;
	}
};

static const ProbeClusterInfo ProbeClusterInfoInvalid{};

} // namespace mediasoup::ccutils
