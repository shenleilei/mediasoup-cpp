#pragma once
// Minimal WebSocket client for integration tests (RFC 6455, no TLS)
#include <string>
#include <functional>
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <random>
#include <sstream>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <chrono>
#include <cerrno>

using json = nlohmann::json;

class TestWsClient {
public:
	~TestWsClient() { close(); }

	bool connect(const std::string& host, int port, const std::string& path = "/ws") {
		fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
		if (fd_ < 0) return false;

		struct timeval tv{5, 0};
		setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		setsockopt(fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

		if (::connect(fd_, (sockaddr*)&addr, sizeof(addr)) < 0) { ::close(fd_); fd_ = -1; return false; }

		// WebSocket handshake - key must be 16 bytes base64-encoded (24 chars)
		std::string key = "dGVzdGtleTEyMzQ1Njc4OQ=="; // base64("testkey123456789")
		std::string req = "GET " + path + " HTTP/1.1\r\n"
			"Host: " + host + ":" + std::to_string(port) + "\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Key: " + key + "\r\n"
			"Sec-WebSocket-Version: 13\r\n"
			"Origin: http://" + host + "\r\n\r\n";
		if (::send(fd_, req.data(), req.size(), 0) <= 0) { ::close(fd_); fd_ = -1; return false; }

		// Read handshake response
		char buf[4096]{};
		int n = ::recv(fd_, buf, sizeof(buf) - 1, 0);
		if (n <= 0 || std::string(buf, n).find("101") == std::string::npos) {
			::close(fd_); fd_ = -1; return false;
		}

		connected_ = true;
		// Start reader thread
		reader_ = std::thread([this]{ readLoop(); });
		return true;
	}

	void close() {
		connected_ = false;
		if (fd_ >= 0) { ::shutdown(fd_, SHUT_RDWR); ::close(fd_); fd_ = -1; }
		if (reader_.joinable()) reader_.join();
	}

	// Send a JSON request and wait for matching response
	json request(const std::string& method, const json& data = {}, int timeoutMs = 5000) {
		uint64_t id = nextId_++;
		json msg = {{"request", true}, {"id", id}, {"method", method}, {"data", data}};
		sendText(msg.dump());

		auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
		std::unique_lock<std::mutex> lock(mu_);
		while (true) {
			// Check if response already arrived
			for (auto it = responses_.begin(); it != responses_.end(); ++it) {
				if (it->value("id", 0) == id) {
					json resp = std::move(*it);
					responses_.erase(it);
					return resp;
				}
			}
			if (cv_.wait_until(lock, deadline) == std::cv_status::timeout)
				return {{"ok", false}, {"error", "timeout"}};
		}
	}

	// Drain notifications received so far
	std::vector<json> drainNotifications() {
		std::lock_guard<std::mutex> lock(mu_);
		std::vector<json> result;
		result.swap(notifications_);
		return result;
	}

	// Wait for a notification with given method
	json waitNotification(const std::string& method, int timeoutMs = 5000) {
		auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
		std::unique_lock<std::mutex> lock(mu_);
		while (true) {
			for (auto it = notifications_.begin(); it != notifications_.end(); ++it) {
				if (it->value("method", "") == method) {
					json n = std::move(*it);
					notifications_.erase(it);
					return n;
				}
			}
			if (cv_.wait_until(lock, deadline) == std::cv_status::timeout)
				return {};
		}
	}

private:
	void sendText(const std::string& text) {
		// WebSocket frame: FIN=1, opcode=1 (text), MASK=1
		std::vector<uint8_t> frame;
		frame.push_back(0x81); // FIN + TEXT
		uint8_t mask[4] = {0x12, 0x34, 0x56, 0x78};
		size_t len = text.size();
		if (len < 126) {
			frame.push_back(0x80 | (uint8_t)len);
		} else if (len < 65536) {
			frame.push_back(0x80 | 126);
			frame.push_back((len >> 8) & 0xFF);
			frame.push_back(len & 0xFF);
		} else {
			frame.push_back(0x80 | 127);
			for (int i = 7; i >= 0; --i) frame.push_back((len >> (8 * i)) & 0xFF);
		}
		frame.insert(frame.end(), mask, mask + 4);
		for (size_t i = 0; i < len; ++i) frame.push_back(text[i] ^ mask[i % 4]);
		auto sent = ::send(fd_, frame.data(), frame.size(), 0);
		(void)sent;
	}

	void readLoop() {
		std::vector<uint8_t> buf(65536);
		std::vector<uint8_t> pending; // buffer for incomplete frames
		while (connected_) {
			int n = ::recv(fd_, buf.data(), buf.size(), 0);
			if (n < 0) {
				// Timeout (EAGAIN/EWOULDBLOCK) — just retry
				if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
				break; // real error
			}
			if (n == 0) break; // connection closed
			pending.insert(pending.end(), buf.begin(), buf.begin() + n);

			// Parse all complete frames in pending buffer
			while (pending.size() >= 2) {
				int opcode = pending[0] & 0x0F;
				if (opcode == 0x8) { connected_ = false; return; } // close
				bool masked = pending[1] & 0x80;
				uint64_t payloadLen = pending[1] & 0x7F;
				size_t headerLen = 2;
				if (payloadLen == 126) {
					if (pending.size() < 4) break;
					payloadLen = ((uint64_t)pending[2] << 8) | pending[3];
					headerLen = 4;
				} else if (payloadLen == 127) {
					if (pending.size() < 10) break;
					payloadLen = 0;
					for (int i = 0; i < 8; ++i) payloadLen = (payloadLen << 8) | pending[2 + i];
					headerLen = 10;
				}
				if (masked) headerLen += 4;
				size_t frameLen = headerLen + payloadLen;
				if (pending.size() < frameLen) break; // incomplete frame

				std::string text((char*)&pending[headerLen], payloadLen);
				pending.erase(pending.begin(), pending.begin() + frameLen);

				try {
					json msg = json::parse(text);
					std::lock_guard<std::mutex> lock(mu_);
					if (msg.contains("response") && msg["response"].get<bool>())
						responses_.push_back(std::move(msg));
					else if (msg.contains("notification") && msg["notification"].get<bool>())
						notifications_.push_back(std::move(msg));
					cv_.notify_all();
				} catch (...) {}
			}
		}
	}

	int fd_ = -1;
	std::atomic<bool> connected_{false};
	std::atomic<uint64_t> nextId_{1};
	std::thread reader_;
	std::mutex mu_;
	std::condition_variable cv_;
	std::vector<json> responses_;
	std::vector<json> notifications_;
};
