#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <any>
#include <mutex>
#include <cstdint>

namespace mediasoup {

class EventEmitter {
public:
	using Listener = std::function<void(const std::vector<std::any>&)>;

	uint64_t on(const std::string& event, Listener listener) {
		std::lock_guard<std::mutex> lock(mutex_);
		uint64_t id = nextId_++;
		listeners_[event].push_back({id, std::move(listener), false});
		return id;
	}

	uint64_t once(const std::string& event, Listener listener) {
		std::lock_guard<std::mutex> lock(mutex_);
		uint64_t id = nextId_++;
		listeners_[event].push_back({id, std::move(listener), true});
		return id;
	}

	void off(uint64_t id) {
		std::lock_guard<std::mutex> lock(mutex_);
		for (auto& [event, entries] : listeners_) {
			entries.erase(
				std::remove_if(entries.begin(), entries.end(),
					[id](const Entry& e) { return e.id == id; }),
				entries.end());
		}
	}

	void off(const std::string& event) {
		std::lock_guard<std::mutex> lock(mutex_);
		listeners_.erase(event);
	}

	void emit(const std::string& event, const std::vector<std::any>& args = {}) {
		std::vector<Entry> toCall;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			auto it = listeners_.find(event);
			if (it == listeners_.end()) return;
			toCall = it->second;
			// Remove once listeners
			it->second.erase(
				std::remove_if(it->second.begin(), it->second.end(),
					[](const Entry& e) { return e.once; }),
				it->second.end());
		}
		for (auto& entry : toCall) {
			entry.fn(args);
		}
	}

private:
	struct Entry {
		uint64_t id;
		Listener fn;
		bool once;
	};
	std::mutex mutex_;
	std::unordered_map<std::string, std::vector<Entry>> listeners_;
	uint64_t nextId_ = 1;
};

} // namespace mediasoup
