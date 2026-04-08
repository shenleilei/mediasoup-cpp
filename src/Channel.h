#pragma once
#include "EventEmitter.h"
#include "Logger.h"
#include <flatbuffers/flatbuffers.h>
#include "message_generated.h"
#include "request_generated.h"
#include "response_generated.h"
#include "notification_generated.h"
#include "log_generated.h"
#include <functional>
#include <future>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <cstdint>

namespace mediasoup {

class Channel {
public:
	Channel(int producerFd, int consumerFd, int pid);
	~Channel();

	void close();
	bool closed() const { return closed_; }

	flatbuffers::FlatBufferBuilder& bufferBuilder() { return builder_; }

	struct OwnedResponse {
		std::vector<uint8_t> data;
		const FBS::Response::Response* response() const {
			if (data.empty()) return nullptr;
			auto* msg = FBS::Message::GetMessage(data.data());
			return msg ? msg->data_as_Response() : nullptr;
		}
	};

	struct OwnedNotification {
		std::vector<uint8_t> data;
		FBS::Notification::Event event = FBS::Notification::Event::WORKER_RUNNING;
		const FBS::Notification::Notification* notification() const {
			if (data.empty()) return nullptr;
			auto* msg = FBS::Message::GetMessage(data.data());
			return msg ? msg->data_as_Notification() : nullptr;
		}
	};

	std::future<OwnedResponse> request(
		FBS::Request::Method method,
		FBS::Request::Body bodyType = FBS::Request::Body::NONE,
		flatbuffers::Offset<void> bodyOffset = 0,
		const std::string& handlerId = "");

	// Convenience: request + timed wait. Throws on timeout.
	OwnedResponse requestWait(
		FBS::Request::Method method,
		FBS::Request::Body bodyType = FBS::Request::Body::NONE,
		flatbuffers::Offset<void> bodyOffset = 0,
		const std::string& handlerId = "",
		int timeoutMs = 5000)
	{
		auto fut = request(method, bodyType, bodyOffset, handlerId);
		if (fut.wait_for(std::chrono::milliseconds(timeoutMs)) != std::future_status::ready) {
			// Clean up the pending entry to prevent accumulation
			cleanupTimedOutRequest(method, handlerId);
			throw std::runtime_error("Channel request timeout (" + std::to_string(timeoutMs) + "ms)");
		}
		return fut.get();
	}

	void cleanupTimedOutRequest(FBS::Request::Method method, const std::string& handlerId) {
		std::lock_guard<std::mutex> lock(sentsMutex_);
		for (auto it = sents_.begin(); it != sents_.end(); ++it) {
			if (it->second->method == FBS::Request::EnumNameMethod(method)) {
				spdlog::warn("Channel request timeout, removing pending [method:{} handler:{}]",
					it->second->method, handlerId);
				it->second->promise.set_exception(
					std::make_exception_ptr(std::runtime_error("timeout")));
				sents_.erase(it);
				return;
			}
		}
	}

	void notify(
		FBS::Notification::Event event,
		FBS::Notification::Body bodyType = FBS::Notification::Body::NONE,
		flatbuffers::Offset<void> bodyOffset = 0,
		const std::string& handlerId = "");

	EventEmitter& emitter() { return emitter_; }

private:
	void readLoop();
	bool processMessage(const uint8_t* data, size_t len);
	void processNotification(const uint8_t* data, size_t len,
		const FBS::Notification::Notification* notification);
	void processLog(const FBS::Log::Log* log);
	void sendBytes(const uint8_t* data, size_t len);

	int producerFd_;
	int consumerFd_;
	int pid_;
	std::atomic<bool> closed_{false};

	flatbuffers::FlatBufferBuilder builder_{1024};
	std::mutex builderMutex_;

	uint32_t nextId_ = 0;

	struct PendingSent {
		uint32_t id;
		std::string method;
		std::promise<OwnedResponse> promise;
	};
	std::unordered_map<uint32_t, std::shared_ptr<PendingSent>> sents_;
	std::mutex sentsMutex_;

	std::thread readThread_;
	EventEmitter emitter_;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
