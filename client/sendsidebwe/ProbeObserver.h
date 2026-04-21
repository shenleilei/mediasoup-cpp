#pragma once

#include "../ccutils/ProbeTypes.h"

#include <mutex>

namespace mediasoup::plainclient::sendsidebwe {

class ProbeObserverListener {
public:
	virtual ~ProbeObserverListener() = default;
	virtual void OnProbeObserverClusterComplete(mediasoup::ccutils::ProbeClusterId clusterId) = 0;
};

class ProbeObserver {
public:
	void SetListener(ProbeObserverListener* listener)
	{
		listener_ = listener;
	}

	void StartProbeCluster(const mediasoup::ccutils::ProbeClusterInfo& probeClusterInfo)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (isInProbe_) {
			return;
		}
		probeClusterInfo_ = probeClusterInfo;
		probeClusterInfo_.result = mediasoup::ccutils::ProbeClusterResult{};
		probeClusterInfo_.result.startTimeUs = NowUs();
		isInProbe_ = true;
	}

	mediasoup::ccutils::ProbeClusterInfo EndProbeCluster(mediasoup::ccutils::ProbeClusterId probeClusterId)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!isInProbe_ || probeClusterInfo_.id != probeClusterId) {
			return mediasoup::ccutils::ProbeClusterInfoInvalid;
		}
		if (probeClusterInfo_.result.endTimeUs == 0) {
			probeClusterInfo_.result.endTimeUs = NowUs();
		}
		isInProbe_ = false;
		return probeClusterInfo_;
	}

	void RecordPacket(size_t size, bool isRtx, mediasoup::ccutils::ProbeClusterId probeClusterId, bool isProbe)
	{
		ProbeObserverListener* listener = nullptr;
		mediasoup::ccutils::ProbeClusterId clusterId = mediasoup::ccutils::ProbeClusterIdInvalid;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (!isInProbe_ || probeClusterInfo_.id != probeClusterId || probeClusterInfo_.result.endTimeUs != 0) {
				return;
			}

			if (isProbe) {
				probeClusterInfo_.result.packetsProbe++;
				probeClusterInfo_.result.bytesProbe += static_cast<int>(size);
			} else if (isRtx) {
				probeClusterInfo_.result.packetsNonProbeRtx++;
				probeClusterInfo_.result.bytesNonProbeRtx += static_cast<int>(size);
			} else {
				probeClusterInfo_.result.packetsNonProbePrimary++;
				probeClusterInfo_.result.bytesNonProbePrimary += static_cast<int>(size);
			}

			if (probeClusterInfo_.result.endTimeUs == 0 &&
				probeClusterInfo_.result.Bytes() >= probeClusterInfo_.goal.desiredBytes &&
				std::chrono::microseconds(NowUs() - probeClusterInfo_.result.startTimeUs) >= probeClusterInfo_.goal.duration) {
				probeClusterInfo_.result.endTimeUs = NowUs();
				probeClusterInfo_.result.isCompleted = true;
				listener = listener_;
				clusterId = probeClusterInfo_.id;
			}
		}
		if (listener && clusterId != mediasoup::ccutils::ProbeClusterIdInvalid) {
			listener->OnProbeObserverClusterComplete(clusterId);
		}
	}

	bool InProbe() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return isInProbe_;
	}

	mediasoup::ccutils::ProbeClusterId ActiveClusterId() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return isInProbe_ ? probeClusterInfo_.id : mediasoup::ccutils::ProbeClusterIdInvalid;
	}

	mediasoup::ccutils::ProbeClusterInfo CurrentProbeClusterInfo() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return isInProbe_ ? probeClusterInfo_ : mediasoup::ccutils::ProbeClusterInfoInvalid;
	}

private:
	static int64_t NowUs()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	mutable std::mutex mutex_;
	ProbeObserverListener* listener_{ nullptr };
	bool isInProbe_{ false };
	mediasoup::ccutils::ProbeClusterInfo probeClusterInfo_{ mediasoup::ccutils::ProbeClusterInfoInvalid };
};

} // namespace mediasoup::plainclient::sendsidebwe
