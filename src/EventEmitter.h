#pragma once
#include <algorithm>
#include <exception>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <any>
#include <mutex>
#include <cstdint>
#include <spdlog/spdlog.h>
#include "Logger.h"

namespace mediasoup {

class EventEmitter {
public:
	using Listener = std::function<void(const std::vector<std::any>&)>;

	uint64_t on(const std::string& event, Listener listener) {
		std::lock_guard<std::mutex> lock(mutex_);
		uint64_t id = nextId_++;
		listeners_[event].push_back({id, std::move(listener), false});
		listenerEventIndex_[id] = event;
		return id;
	}

	uint64_t once(const std::string& event, Listener listener) {
		std::lock_guard<std::mutex> lock(mutex_);
		uint64_t id = nextId_++;
		listeners_[event].push_back({id, std::move(listener), true});
		listenerEventIndex_[id] = event;
		return id;
	}

	void off(uint64_t id) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto indexIt = listenerEventIndex_.find(id);
		if (indexIt == listenerEventIndex_.end()) {
			return;
		}
		auto listenersIt = listeners_.find(indexIt->second);
		if (listenersIt != listeners_.end()) {
			auto& entries = listenersIt->second;
			entries.erase(
				std::remove_if(entries.begin(), entries.end(),
					[id](const Entry& e) { return e.id == id; }),
				entries.end());
			if (entries.empty()) {
				listeners_.erase(listenersIt);
			}
		}
		listenerEventIndex_.erase(indexIt);
	}

	void off(const std::string& event) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = listeners_.find(event);
		if (it == listeners_.end()) {
			return;
		}
		for (const auto& entry : it->second) {
			listenerEventIndex_.erase(entry.id);
		}
		listeners_.erase(it);
	}

	void emit(const std::string& event, const std::vector<std::any>& args = {}) {
		emitImpl(event, args, false);
	}

	void emitChecked(const std::string& event, const std::vector<std::any>& args = {}) {
		emitImpl(event, args, true);
	}

private:
	void emitImpl(
		const std::string& event,
		const std::vector<std::any>& args,
		bool rethrowListenerFailure)
	{
		std::vector<Entry> toCall;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			auto it = listeners_.find(event);
			if (it == listeners_.end()) return;
			toCall = it->second;
			auto& entries = it->second;
			for (auto entryIt = entries.begin(); entryIt != entries.end(); ) {
				if (entryIt->once) {
					listenerEventIndex_.erase(entryIt->id);
					entryIt = entries.erase(entryIt);
				} else {
					++entryIt;
				}
			}
			if (entries.empty()) {
				listeners_.erase(it);
			}
		}
		std::exception_ptr firstException;
		for (auto& entry : toCall) {
			try {
				entry.fn(args);
			} catch (const std::bad_any_cast& e) {
				MS_SPDLOG_WARN("EventEmitter bad_any_cast in listener [event:{}]: {}", event, e.what());
				if (!firstException) firstException = std::current_exception();
			} catch (const std::exception& e) {
				MS_SPDLOG_WARN("EventEmitter listener exception [event:{}]: {}", event, e.what());
				if (!firstException) firstException = std::current_exception();
			} catch (...) {
				MS_SPDLOG_WARN("EventEmitter listener unknown exception [event:{}]", event);
				if (!firstException) firstException = std::current_exception();
			}
		}
		if (rethrowListenerFailure && firstException) {
			std::rethrow_exception(firstException);
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
	std::unordered_map<uint64_t, std::string> listenerEventIndex_;
	uint64_t nextId_ = 1;
};

} // namespace mediasoup
