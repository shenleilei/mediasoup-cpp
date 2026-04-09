#pragma once
#include "Worker.h"
#include "Constants.h"
#include <vector>
#include <memory>
#include <mutex>

namespace mediasoup {

class WorkerManager {
public:
	~WorkerManager() { close(); }

	std::shared_ptr<Worker> createWorker(const WorkerSettings& settings = {}) {
		auto worker = std::make_shared<Worker>(settings);
		{
			std::lock_guard<std::mutex> lock(mutex_);
			workers_.push_back(worker);
			lastSettings_ = settings;
		}

		installDiedHandler(worker);

		return worker;
	}

	// Add a pre-created Worker (e.g., from WorkerThread with non-threaded mode).
	// Does NOT install the died handler (WorkerThread handles death itself).
	void addExistingWorker(std::shared_ptr<Worker> worker) {
		std::lock_guard<std::mutex> lock(mutex_);
		workers_.push_back(std::move(worker));
	}

	void setMaxRoutersPerWorker(size_t max) { maxRoutersPerWorker_ = max; }
	size_t maxRoutersPerWorker() const { return maxRoutersPerWorker_; }

	std::shared_ptr<Worker> getLeastLoadedWorker() const {
		std::lock_guard<std::mutex> lock(mutex_);
		if (workers_.empty()) return nullptr;
		std::shared_ptr<Worker> best;
		size_t minLoad = SIZE_MAX;
		for (auto& w : workers_) {
			if (w->closed()) continue;
			size_t load = w->routerCount();
			if (maxRoutersPerWorker_ > 0 && load >= maxRoutersPerWorker_) continue;
			if (load < minLoad) {
				minLoad = load;
				best = w;
			}
		}
		return best;
	}

	size_t totalRouterCount() const {
		std::lock_guard<std::mutex> lock(mutex_);
		size_t total = 0;
		for (auto& w : workers_)
			if (!w->closed()) total += w->routerCount();
		return total;
	}

	size_t maxTotalRouters() const {
		std::lock_guard<std::mutex> lock(mutex_);
		if (maxRoutersPerWorker_ == 0) return 0; // unlimited
		size_t count = 0;
		for (auto& w : workers_)
			if (!w->closed()) ++count;
		return count * maxRoutersPerWorker_;
	}

	void close() {
		std::vector<std::shared_ptr<Worker>> workersToClose;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (closing_) return;
			closing_ = true;
			lifetimeToken_.reset();
			workersToClose.swap(workers_);
		}
		for (auto& w : workersToClose) w->close();
	}

	size_t size() const { std::lock_guard<std::mutex> lock(mutex_); return workers_.size(); }

private:
	void installDiedHandler(const std::shared_ptr<Worker>& worker) {
		auto weakToken = std::weak_ptr<void>(lifetimeToken_);
		worker->emitter().on("died", [this, weak = std::weak_ptr<Worker>(worker), weakToken](auto&) {
			if (weakToken.expired()) return;
			WorkerSettings settingsSnapshot;
			bool shouldRespawn = false;
			{
				std::lock_guard<std::mutex> lock(mutex_);
				if (closing_ || weakToken.expired()) return;
				if (auto w = weak.lock())
					workers_.erase(std::remove(workers_.begin(), workers_.end(), w), workers_.end());
				shouldRespawn = prepareRespawnLocked(weakToken, settingsSnapshot);
			}
			// Heavy work outside lock.
			if (shouldRespawn) respawnOne(weakToken, settingsSnapshot);
		});
	}

	bool prepareRespawnLocked(const std::weak_ptr<void>& weakToken, WorkerSettings& outSettings) {
		if (closing_ || weakToken.expired()) return false;

		// Rate-limit: max N respawns within window
		auto now = std::chrono::steady_clock::now();
		respawnTimes_.push_back(now);
		while (!respawnTimes_.empty() &&
			   (now - respawnTimes_.front()) > std::chrono::seconds(kRespawnWindowSec))
			respawnTimes_.erase(respawnTimes_.begin());
		if (respawnTimes_.size() > kMaxRespawnsPerWindow) {
			spdlog::error("Worker respawn rate limit hit ({} in {}s), not respawning",
				respawnTimes_.size(), kRespawnWindowSec);
			return false;
		}
		outSettings = lastSettings_;
		return true;
	}

	void respawnOne(const std::weak_ptr<void>& weakToken, const WorkerSettings& settings) {
		try {
			auto worker = std::make_shared<Worker>(settings);
			{
				std::lock_guard<std::mutex> lock(mutex_);
				if (closing_ || weakToken.expired()) {
					worker->close();
					return;
				}
				workers_.push_back(worker);
			}
			installDiedHandler(worker);
			spdlog::warn("Worker respawned");
		} catch (const std::exception& e) {
			spdlog::error("Failed to respawn worker: {}", e.what());
		}
	}

	mutable std::mutex mutex_;
	std::vector<std::shared_ptr<Worker>> workers_;
	WorkerSettings lastSettings_;
	bool closing_ = false;
	size_t maxRoutersPerWorker_ = 0;
	std::vector<std::chrono::steady_clock::time_point> respawnTimes_;
	std::shared_ptr<void> lifetimeToken_ = std::make_shared<int>(0);
};

} // namespace mediasoup
