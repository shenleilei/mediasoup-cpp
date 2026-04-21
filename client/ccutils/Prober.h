#pragma once

#include "ProbeTypes.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>

namespace mediasoup::ccutils {

class ProberListener {
public:
	virtual ~ProberListener() = default;
	virtual void OnProbeClusterSwitch(const ProbeClusterInfo& info) = 0;
	virtual void OnSendProbe(int bytesToSend) = 0;
};

enum class ProbeClusterMode {
	Uniform,
	LinearChirp
};

class Prober {
public:
	static constexpr int kBytesPerProbe = 1100;
	static constexpr auto kSleepDuration = std::chrono::milliseconds(20);
	static constexpr auto kSleepDurationMin = std::chrono::milliseconds(10);

	explicit Prober(ProberListener* listener = nullptr)
		: listener_(listener)
	{
	}

	~Prober()
	{
		Stop();
	}

	void SetListener(ProberListener* listener)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		listener_ = listener;
	}

	bool IsRunning() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return activeCluster_ != nullptr || !clusters_.empty();
	}

	void Reset(const ProbeClusterInfo& info = ProbeClusterInfoInvalid)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (activeCluster_ && activeCluster_->Info().id == info.id && info.IsValid()) {
				activeCluster_->MarkCompleted(info.result);
			}
			clusters_.clear();
			activeCluster_.reset();
			stopRequested_ = true;
			cv_.notify_all();
		}
		if (worker_.joinable()) {
			worker_.join();
		}
		stopRequested_ = false;
	}

	ProbeClusterInfo AddCluster(ProbeClusterMode mode, ProbeClusterGoal goal)
	{
		if (goal.desiredBps <= 0) {
			return ProbeClusterInfoInvalid;
		}

		auto cluster = std::make_shared<Cluster>(
			static_cast<ProbeClusterId>(clusterId_.fetch_add(1) + 1),
			mode,
			goal,
			[this](const ProbeClusterInfo& info) {
				ProberListener* listener = nullptr;
				{
					std::lock_guard<std::mutex> lock(mutex_);
					listener = listener_;
				}
				if (listener) {
					listener->OnProbeClusterSwitch(info);
				}
			},
			[this](int bytesToSend) {
				ProberListener* listener = nullptr;
				{
					std::lock_guard<std::mutex> lock(mutex_);
					listener = listener_;
				}
				if (listener) {
					listener->OnSendProbe(bytesToSend);
				}
			});

		{
			std::lock_guard<std::mutex> lock(mutex_);
			clusters_.push_back(cluster);
			if (!worker_.joinable()) {
				worker_ = std::thread([this] { Run(); });
			}
			cv_.notify_all();
		}
		return cluster->Info();
	}

	void ProbesSent(int bytesSent)
	{
		auto cluster = GetFrontCluster();
		if (cluster) {
			cluster->ProbesSent(bytesSent);
		}
	}

	void ClusterDone(const ProbeClusterInfo& info)
	{
		auto cluster = GetFrontCluster();
		if (!cluster) {
			return;
		}
		if (cluster->Id() == info.id) {
			cluster->MarkCompleted(info.result);
			PopFrontCluster(cluster);
		}
	}

	ProbeClusterId GetActiveClusterId() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return activeCluster_ ? activeCluster_->Id() : ProbeClusterIdInvalid;
	}

private:
	struct Bucket {
		std::chrono::microseconds expectedElapsedDuration{ 0 };
		int expectedProbeBytesSent{ 0 };
	};

	class Cluster {
	public:
		using SwitchFn = std::function<void(const ProbeClusterInfo&)>;
		using SendFn = std::function<void(int)>;

		Cluster(
			ProbeClusterId id,
			ProbeClusterMode mode,
			ProbeClusterGoal goal,
			SwitchFn onSwitch,
			SendFn onSend)
			: mode_(mode)
			, onSwitch_(std::move(onSwitch))
			, onSend_(std::move(onSend))
		{
			info_.id = id;
			info_.createdAt = std::chrono::steady_clock::now();
			info_.goal = goal;
			InitProbes();
		}

		void Start()
		{
			if (onSwitch_) {
				onSwitch_(info_);
			}
		}

		ProbeClusterId Id() const
		{
			return info_.id;
		}

		ProbeClusterInfo Info() const
		{
			std::lock_guard<std::mutex> lock(mutex_);
			return info_;
		}

		void ProbesSent(int bytesSent)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			probeBytesSent_ += bytesSent;
		}

		void MarkCompleted(const ProbeClusterResult& result)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			isComplete_ = true;
			info_.result = result;
		}

		std::chrono::microseconds Process()
		{
			int bytesToSend = 0;
			{
				std::lock_guard<std::mutex> lock(mutex_);
				if (isComplete_) {
					return std::chrono::microseconds(0);
				}

				if (startTime_ == std::chrono::steady_clock::time_point{}) {
					startTime_ = std::chrono::steady_clock::now();
					bytesToSend = kBytesPerProbe;
				} else {
					const auto sinceStart =
						std::chrono::duration_cast<std::chrono::microseconds>(
							std::chrono::steady_clock::now() - startTime_);
					if (sinceStart > buckets_[bucketIndex_].expectedElapsedDuration) {
						bucketIndex_++;
						bool overflow = false;
						if (bucketIndex_ >= static_cast<int>(buckets_.size())) {
							bucketIndex_ = static_cast<int>(buckets_.size()) - 1;
							overflow = true;
						}
						if (buckets_[bucketIndex_].expectedProbeBytesSent > probeBytesSent_ || overflow) {
							bytesToSend = std::max(
								kBytesPerProbe,
								buckets_[bucketIndex_].expectedProbeBytesSent - probeBytesSent_);
						}
					}
				}
			}

			if (bytesToSend != 0 && onSend_) {
				onSend_(bytesToSend);
			}
			return std::chrono::duration_cast<std::chrono::microseconds>(kSleepDurationMin);
		}

	private:
		void InitProbes()
		{
			info_.goal.desiredBytes = static_cast<int>(
				std::llround(static_cast<double>(info_.goal.desiredBps) *
					(info_.goal.duration.count() / 1000000.0) / 8.0 + 0.5));

			int numBuckets = static_cast<int>(
				std::llround((info_.goal.duration.count() / 1000000.0) /
					std::chrono::duration<double>(kSleepDuration).count() + 0.5));
			if (numBuckets < 1) {
				numBuckets = 1;
			}
			int numIntervals = numBuckets;
			if (mode_ == ProbeClusterMode::LinearChirp) {
				int sum = 0;
				int i = 1;
				for (;; ++i) {
					sum += i;
					if (sum >= numBuckets) {
						break;
					}
				}
				numBuckets = i;
				numIntervals = sum;
			}

			baseSleepDuration_ = info_.goal.duration / numIntervals;
			if (baseSleepDuration_ < kSleepDurationMin) {
				baseSleepDuration_ = std::chrono::duration_cast<std::chrono::microseconds>(kSleepDurationMin);
			}

			numIntervals = static_cast<int>(
				std::llround((info_.goal.duration.count() / 1000000.0) /
					std::chrono::duration<double>(baseSleepDuration_).count() + 0.5));
			const int desiredProbeBytesPerInterval = static_cast<int>(
				std::llround((((info_.goal.duration.count() / 1000000.0) *
					static_cast<double>(info_.goal.desiredBps - info_.goal.expectedUsageBps) / 8.0) +
					static_cast<double>(numIntervals) - 1.0) /
					static_cast<double>(numIntervals) + 0.5));

			buckets_.resize(numBuckets);
			for (int i = 0; i < numBuckets; ++i) {
				switch (mode_) {
					case ProbeClusterMode::Uniform:
						buckets_[i].expectedElapsedDuration = baseSleepDuration_;
						break;
					case ProbeClusterMode::LinearChirp:
						buckets_[i].expectedElapsedDuration = (numBuckets - i) * baseSleepDuration_;
						break;
				}
				if (i > 0) {
					buckets_[i].expectedElapsedDuration += buckets_[i - 1].expectedElapsedDuration;
				}
				buckets_[i].expectedProbeBytesSent = (i + 1) * desiredProbeBytesPerInterval;
			}
		}

		mutable std::mutex mutex_;
		ProbeClusterInfo info_;
		ProbeClusterMode mode_{ ProbeClusterMode::Uniform };
		SwitchFn onSwitch_;
		SendFn onSend_;
		std::chrono::microseconds baseSleepDuration_{ 0 };
		std::vector<Bucket> buckets_;
		int bucketIndex_{ 0 };
		int probeBytesSent_{ 0 };
		std::chrono::steady_clock::time_point startTime_{};
		bool isComplete_{ false };
	};

	std::shared_ptr<Cluster> GetFrontCluster() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return activeCluster_;
	}

	void PopFrontCluster(const std::shared_ptr<Cluster>& cluster)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!clusters_.empty() && clusters_.front() == cluster) {
			clusters_.pop_front();
		}
		if (activeCluster_ == cluster) {
			activeCluster_.reset();
		}
		cv_.notify_all();
	}

	void Run()
	{
		for (;;) {
			std::shared_ptr<Cluster> cluster;
			{
				std::unique_lock<std::mutex> lock(mutex_);
				cv_.wait(lock, [&] {
					return stopRequested_ || activeCluster_ || !clusters_.empty();
				});
				if (stopRequested_) {
					return;
				}
				if (!activeCluster_) {
					if (clusters_.empty()) {
						continue;
					}
					activeCluster_ = clusters_.front();
					cluster = activeCluster_;
				} else {
					cluster = activeCluster_;
				}
			}

			if (cluster && !startedClusters_.count(cluster->Id())) {
				startedClusters_.insert(cluster->Id());
				cluster->Start();
			}

			if (!cluster) {
				continue;
			}
			const auto sleepDuration = cluster->Process();
			if (sleepDuration.count() == 0) {
				PopFrontCluster(cluster);
				continue;
			}
			std::this_thread::sleep_for(sleepDuration);
		}
	}

	void Stop()
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			stopRequested_ = true;
			cv_.notify_all();
		}
		if (worker_.joinable()) {
			worker_.join();
		}
	}

	mutable std::mutex mutex_;
	std::condition_variable cv_;
	ProberListener* listener_{ nullptr };
	std::atomic<uint32_t> clusterId_{ 0 };
	std::deque<std::shared_ptr<Cluster>> clusters_;
	std::shared_ptr<Cluster> activeCluster_;
	mutable std::unordered_set<ProbeClusterId> startedClusters_;
	std::thread worker_;
	bool stopRequested_{ false };
};

} // namespace mediasoup::ccutils
