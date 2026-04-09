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
	// Original constructor — threaded mode (creates readThread, backward compat)
	Channel(int producerFd, int consumerFd, int pid);
	// New constructor — explicit threading control
	Channel(int producerFd, int consumerFd, int pid, bool threaded);
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

	struct RequestResult {
		std::future<OwnedResponse> future;
		uint32_t requestId = 0;
	};

	RequestResult requestWithId(
		FBS::Request::Method method,
		FBS::Request::Body bodyType = FBS::Request::Body::NONE,
		flatbuffers::Offset<void> bodyOffset = 0,
		const std::string& handlerId = "");

	std::future<OwnedResponse> request(
		FBS::Request::Method method,
		FBS::Request::Body bodyType = FBS::Request::Body::NONE,
		flatbuffers::Offset<void> bodyOffset = 0,
		const std::string& handlerId = "")
	{
		return requestWithId(method, bodyType, bodyOffset, handlerId).future;
	}

	// Convenience: request + timed wait. Throws on timeout.
	// In threaded mode: waits on future (readThread fills it).
	// In non-threaded mode: pumps the consumer fd via poll()+processAvailableData().
	OwnedResponse requestWait(
		FBS::Request::Method method,
		FBS::Request::Body bodyType = FBS::Request::Body::NONE,
		flatbuffers::Offset<void> bodyOffset = 0,
		const std::string& handlerId = "",
		int timeoutMs = 5000);

	void notify(
		FBS::Notification::Event event,
		FBS::Notification::Body bodyType = FBS::Notification::Body::NONE,
		flatbuffers::Offset<void> bodyOffset = 0,
		const std::string& handlerId = "");

	EventEmitter& emitter() { return emitter_; }

	// ── Non-threaded mode API ──

	bool isThreaded() const { return threaded_; }

	// Expose consumer fd for epoll registration (non-threaded mode)
	int consumerFd() const { return consumerFd_; }

	// Read and process available data from the consumer fd (non-blocking).
	// Returns true if the fd is still open; false if EOF (worker died).
	bool processAvailableData();

private:
	void init(int producerFd, int consumerFd, int pid, bool threaded);
	void readLoop();
	bool processMessage(const uint8_t* data, size_t len);
	void processNotification(const uint8_t* data, size_t len,
		const FBS::Notification::Notification* notification);
	void processLog(const FBS::Log::Log* log);
	void sendBytes(const uint8_t* data, size_t len);

	int producerFd_;
	int consumerFd_;
	int pid_;
	bool threaded_ = true;
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

	std::vector<uint8_t> recvBuf_;  // Shared receive buffer (for both modes)
	std::mutex recvBufMutex_;
	std::thread readThread_;
	EventEmitter emitter_;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
