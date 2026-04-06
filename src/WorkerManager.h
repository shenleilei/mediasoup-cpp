#pragma once
#include "Worker.h"
#include "Constants.h"
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
			{
				std::lock_guard<std::mutex> lock(mutex_);
				if (auto w = weak.lock())
					workers_.erase(std::remove(workers_.begin(), workers_.end(), w), workers_.end());
			}
			// Respawn in a detached thread to avoid blocking workerDied/waitThread
			std::thread([this]{ respawnOne(); }).detach();
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
		// Small delay to let workerDied() finish cleanup
		std::this_thread::sleep_for(std::chrono::milliseconds(kRespawnDelayMs));

		std::lock_guard<std::mutex> lock(mutex_);
		if (closing_) return;

		// Rate-limit: max N respawns within window
		auto now = std::chrono::steady_clock::now();
		respawnTimes_.push_back(now);
		while (!respawnTimes_.empty() &&
			   (now - respawnTimes_.front()) > std::chrono::seconds(kRespawnWindowSec))
			respawnTimes_.erase(respawnTimes_.begin());
		if (respawnTimes_.size() > kMaxRespawnsPerWindow) {
			spdlog::error("Worker respawn rate limit hit ({} in {}s), not respawning",
				respawnTimes_.size(), kRespawnWindowSec);
			return;
		}

		try {
			auto worker = std::make_shared<Worker>(lastSettings_);
			workers_.push_back(worker);
			worker->emitter().on("died", [this, weak = std::weak_ptr<Worker>(worker)](auto&) {
				{
					std::lock_guard<std::mutex> lock(mutex_);
					if (auto w = weak.lock())
						workers_.erase(std::remove(workers_.begin(), workers_.end(), w), workers_.end());
				}
				std::thread([this]{ respawnOne(); }).detach();
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
