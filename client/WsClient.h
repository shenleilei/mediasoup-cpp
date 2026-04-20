#pragma once

#include <nlohmann/json.hpp>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

using json = nlohmann::json;

class WsClient {
public:
	struct PendingRequest {
		std::mutex mutex;
		std::condition_variable cv;
		bool done = false;
		bool ok = false;
		json data = json::object();
		std::string error;
		std::function<void(bool, const json&, const std::string&)> completion;
	};

	~WsClient();

	bool connect(const std::string& host, int port, const std::string& path);
	void sendText(const std::string& msg);
	enum class RecvTextStatus {
		Text,
		Timeout,
		Closed
	};
	RecvTextStatus recvText(std::string* text, int timeoutMs = 10000);
	void readerLoop();

	json request(const std::string& method, const json& reqData, int timeoutMs = 5000);
	bool requestAsync(const std::string& method, const json& reqData,
		std::function<void(bool, const json&, const std::string&)> completion);

	size_t pendingRequestCount() const;
	void dispatchNotifications();
	void handleResponse(const json& response);
	void failAllPendingRequests(const std::string& error);
	void close();

	int fd = -1;
	std::function<void(const json&)> onNotification;
	std::atomic<bool> running_{false};
	std::atomic<uint32_t> nextId_{1};
	std::thread readerThread_;
	mutable std::mutex stateMutex_;
	std::mutex sendMutex_;
	std::unordered_map<uint32_t, std::shared_ptr<PendingRequest>> pendingRequests_;
	std::deque<json> pendingNotifications_;
};
