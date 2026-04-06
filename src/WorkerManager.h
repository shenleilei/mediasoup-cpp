#pragma once
#include "Worker.h"
#include <vector>
#include <memory>
#include <mutex>

namespace mediasoup {

class WorkerManager {
public:
	std::shared_ptr<Worker> createWorker(const WorkerSettings& settings = {}) {
		auto worker = std::make_shared<Worker>(settings);
		{
			std::lock_guard<std::mutex> lock(mutex_);
			workers_.push_back(worker);
			lastSettings_ = settings;
		}

		worker->emitter().on("died", [this, weak = std::weak_ptr<Worker>(worker)](auto&) {
			if (auto w = weak.lock()) {
				std::lock_guard<std::mutex> lock(mutex_);
				workers_.erase(std::remove(workers_.begin(), workers_.end(), w), workers_.end());
			}
			// Auto-respawn after a short delay to avoid tight crash loops
			respawnOne();
		});

		return worker;
	}

	std::shared_ptr<Worker> getLeastLoadedWorker() const {
		std::lock_guard<std::mutex> lock(mutex_);
		if (workers_.empty()) return nullptr;
		std::shared_ptr<Worker> best;
		size_t minLoad = SIZE_MAX;
		for (auto& w : workers_) {
			if (w->closed()) continue;
			if (w->routerCount() < minLoad) {
				minLoad = w->routerCount();
				best = w;
			}
		}
		return best;
	}

	void close() {
		std::lock_guard<std::mutex> lock(mutex_);
		closing_ = true;
		for (auto& w : workers_) w->close();
		workers_.clear();
	}

	size_t size() const { std::lock_guard<std::mutex> lock(mutex_); return workers_.size(); }

private:
	void respawnOne() {
		std::lock_guard<std::mutex> lock(mutex_);
		if (closing_) return;

		// Rate-limit: max 3 respawns within 10 seconds
		auto now = std::chrono::steady_clock::now();
		respawnTimes_.push_back(now);
		while (!respawnTimes_.empty() &&
			   (now - respawnTimes_.front()) > std::chrono::seconds(10))
			respawnTimes_.erase(respawnTimes_.begin());
		if (respawnTimes_.size() > 3) {
			spdlog::error("Worker respawn rate limit hit ({} in 10s), not respawning",
				respawnTimes_.size());
			return;
		}

		try {
			auto worker = std::make_shared<Worker>(lastSettings_);
			workers_.push_back(worker);
			worker->emitter().on("died", [this, weak = std::weak_ptr<Worker>(worker)](auto&) {
				if (auto w = weak.lock()) {
					std::lock_guard<std::mutex> lock(mutex_);
					workers_.erase(std::remove(workers_.begin(), workers_.end(), w), workers_.end());
				}
				respawnOne();
			});
			spdlog::warn("Worker respawned (now {} workers)", workers_.size());
		} catch (const std::exception& e) {
			spdlog::error("Failed to respawn worker: {}", e.what());
		}
	}

	mutable std::mutex mutex_;
	std::vector<std::shared_ptr<Worker>> workers_;
	WorkerSettings lastSettings_;
	bool closing_ = false;
	std::vector<std::chrono::steady_clock::time_point> respawnTimes_;
};

} // namespace mediasoup
