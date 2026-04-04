#pragma once
#include "Worker.h"
#include <vector>
#include <memory>

namespace mediasoup {

class WorkerManager {
public:
	std::shared_ptr<Worker> createWorker(const WorkerSettings& settings = {}) {
		auto worker = std::make_shared<Worker>(settings);
		workers_.push_back(worker);

		worker->emitter().on("died", [this, weak = std::weak_ptr<Worker>(worker)](auto&) {
			if (auto w = weak.lock()) {
				workers_.erase(std::remove(workers_.begin(), workers_.end(), w), workers_.end());
			}
		});

		return worker;
	}

	// Get worker with least routers for load balancing
	std::shared_ptr<Worker> getLeastLoadedWorker() const {
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
		for (auto& w : workers_) w->close();
		workers_.clear();
	}

	size_t size() const { return workers_.size(); }

private:
	std::vector<std::shared_ptr<Worker>> workers_;
};

} // namespace mediasoup
