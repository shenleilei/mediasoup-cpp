#include "WsClient.h"

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <random>
#include <stdexcept>
#include <vector>

namespace {

constexpr uint8_t kWsOpcodeContinuation = 0x0;
constexpr uint8_t kWsOpcodeText = 0x1;
constexpr uint8_t kWsOpcodeClose = 0x8;
constexpr uint8_t kWsOpcodePing = 0x9;
constexpr uint8_t kWsOpcodePong = 0xA;
constexpr uint64_t kMaxIncomingFrameBytes = 4 * 1024 * 1024;

std::string base64Encode(const unsigned char* data, int len)
{
	BIO* b64 = BIO_new(BIO_f_base64());
	BIO* mem = BIO_new(BIO_s_mem());
	b64 = BIO_push(b64, mem);
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	BIO_write(b64, data, len);
	BIO_flush(b64);
	BUF_MEM* bptr;
	BIO_get_mem_ptr(b64, &bptr);
	std::string result(bptr->data, bptr->length);
	BIO_free_all(b64);
	return result;
}

bool sendAll(int fd, const uint8_t* data, size_t len)
{
	size_t sent = 0;
	while (sent < len) {
		const ssize_t rc = ::send(fd, data + sent, len - sent, 0);
		if (rc > 0) {
			sent += static_cast<size_t>(rc);
			continue;
		}
		if (rc < 0 && errno == EINTR) {
			continue;
		}
		return false;
	}
	return true;
}

bool recvAll(int fd, void* data, size_t len)
{
	size_t received = 0;
	while (received < len) {
		const ssize_t rc = ::recv(
			fd,
			static_cast<uint8_t*>(data) + received,
			len - received,
			MSG_WAITALL);
		if (rc > 0) {
			received += static_cast<size_t>(rc);
			continue;
		}
		if (rc < 0 && errno == EINTR) {
			continue;
		}
		return false;
	}
	return true;
}

enum class PollReadStatus {
	Ready,
	Timeout,
	Closed
};

PollReadStatus waitForReadable(int fd, int timeoutMs)
{
	while (true) {
		struct pollfd pfd{fd, POLLIN, 0};
		const int rc = poll(&pfd, 1, timeoutMs);
		if (rc == 0) return PollReadStatus::Timeout;
		if (rc < 0) {
			if (errno == EINTR) {
				continue;
			}
			return PollReadStatus::Closed;
		}
		if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
			return PollReadStatus::Closed;
		if ((pfd.revents & POLLIN) != 0)
			return PollReadStatus::Ready;
		return PollReadStatus::Timeout;
	}
}

struct WsFrame {
	uint8_t opcode{0};
	bool fin{false};
	std::string payload;
};

bool sendClientFrame(int fd, uint8_t opcode, const uint8_t* payload, size_t payloadSize)
{
	std::vector<uint8_t> frame;
	frame.reserve(2 + 8 + 4 + payloadSize);
	frame.push_back(static_cast<uint8_t>(0x80 | (opcode & 0x0F)));
	uint8_t maskKey[4];
	std::random_device rd;
	for (auto& b : maskKey) b = static_cast<uint8_t>(rd() & 0xFF);

	if (payloadSize < 126) {
		frame.push_back(0x80 | static_cast<uint8_t>(payloadSize));
	} else if (payloadSize < 65536) {
		frame.push_back(0x80 | 126);
		frame.push_back(static_cast<uint8_t>((payloadSize >> 8) & 0xFF));
		frame.push_back(static_cast<uint8_t>(payloadSize & 0xFF));
	} else {
		frame.push_back(0x80 | 127);
		for (int i = 7; i >= 0; --i) {
			frame.push_back(static_cast<uint8_t>((payloadSize >> (i * 8)) & 0xFF));
		}
	}

	frame.insert(frame.end(), std::begin(maskKey), std::end(maskKey));
	for (size_t i = 0; i < payloadSize; ++i) {
		frame.push_back(payload[i] ^ maskKey[i % 4]);
	}
	return sendAll(fd, frame.data(), frame.size());
}

bool receiveWsFrame(int fd, WsFrame* frame)
{
	if (!frame) return false;

	uint8_t hdr[2];
	if (!recvAll(fd, hdr, sizeof(hdr))) return false;

	frame->fin = (hdr[0] & 0x80) != 0;
	frame->opcode = hdr[0] & 0x0F;
	const bool masked = (hdr[1] & 0x80) != 0;

	uint64_t payloadSize = hdr[1] & 0x7F;
	if (payloadSize == 126) {
		uint8_t ext[2];
		if (!recvAll(fd, ext, sizeof(ext))) return false;
		payloadSize =
			(static_cast<uint64_t>(ext[0]) << 8) |
			static_cast<uint64_t>(ext[1]);
	} else if (payloadSize == 127) {
		uint8_t ext[8];
		if (!recvAll(fd, ext, sizeof(ext))) return false;
		payloadSize = 0;
		for (uint8_t byte : ext)
			payloadSize = (payloadSize << 8) | static_cast<uint64_t>(byte);
	}

	const bool controlFrame =
		frame->opcode == kWsOpcodeClose ||
		frame->opcode == kWsOpcodePing ||
		frame->opcode == kWsOpcodePong;
	if (controlFrame && (!frame->fin || payloadSize > 125)) {
		return false;
	}
	if (payloadSize > kMaxIncomingFrameBytes) {
		return false;
	}

	uint8_t mask[4] = {};
	if (masked && !recvAll(fd, mask, sizeof(mask))) return false;

	frame->payload.resize(static_cast<size_t>(payloadSize));
	if (payloadSize > 0 &&
		!recvAll(fd, frame->payload.data(), static_cast<size_t>(payloadSize))) {
		return false;
	}
	if (masked) {
		for (size_t i = 0; i < frame->payload.size(); ++i)
			frame->payload[i] ^= mask[i % 4];
	}

	return true;
}

} // namespace

WsClient::~WsClient()
{
	close();
}

bool WsClient::connect(const std::string& host, int port, const std::string& path)
{
	struct addrinfo hints{}, *res;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) return false;
	fd_ = socket(res->ai_family, SOCK_STREAM, 0);
	if (fd_ < 0) {
		freeaddrinfo(res);
		fd_ = -1;
		return false;
	}
	if (::connect(fd_, res->ai_addr, res->ai_addrlen) < 0) {
		freeaddrinfo(res);
		::close(fd_);
		fd_ = -1;
		return false;
	}
	freeaddrinfo(res);
	int flag = 1;
	setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

	unsigned char keyBytes[16];
	std::random_device rd;
	for (auto& b : keyBytes) b = rd() & 0xFF;
	std::string key = base64Encode(keyBytes, 16);

	std::string req = "GET " + path + " HTTP/1.1\r\n"
		"Host: " + host + ":" + std::to_string(port) + "\r\n"
		"Upgrade: websocket\r\nConnection: Upgrade\r\n"
		"Sec-WebSocket-Key: " + key + "\r\n"
		"Sec-WebSocket-Version: 13\r\n\r\n";
	if (!sendAll(fd_, reinterpret_cast<const uint8_t*>(req.data()), req.size())) {
		::close(fd_);
		fd_ = -1;
		return false;
	}

	std::string response;
	char buf[4096];
	while (response.find("\r\n\r\n") == std::string::npos) {
		const ssize_t n = ::recv(fd_, buf, sizeof(buf), 0);
		if (n <= 0) {
			::close(fd_);
			fd_ = -1;
			return false;
		}
		response.append(buf, static_cast<size_t>(n));
		if (response.size() > 16384) {
			::close(fd_);
			fd_ = -1;
			return false;
		}
	}
	if (response.find("101") == std::string::npos) {
		::close(fd_);
		fd_ = -1;
		return false;
	}

	running_ = true;
	readerThread_ = std::thread([this]() { readerLoop(); });
	return true;
}

bool WsClient::sendText(const std::string& msg)
{
	std::lock_guard<std::mutex> lock(sendMutex_);
	if (fd_ < 0) return false;
	const bool ok = sendClientFrame(
		fd_,
		kWsOpcodeText,
		reinterpret_cast<const uint8_t*>(msg.data()),
		msg.size());
	if (!ok) {
		abortConnection();
	}
	return ok;
}

WsClient::RecvTextStatus WsClient::recvText(std::string* text, int timeoutMs)
{
	if (text) text->clear();
	if (fd_ < 0) return RecvTextStatus::Closed;

	const auto deadline =
		std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
	std::string fragmentedText;
	bool awaitingContinuation = false;

	while (true) {
		const auto remainingMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			deadline - std::chrono::steady_clock::now()).count();
		if (remainingMs <= 0) return RecvTextStatus::Timeout;

		const auto pollStatus = waitForReadable(fd_, static_cast<int>(remainingMs));
		if (pollStatus == PollReadStatus::Timeout) return RecvTextStatus::Timeout;
		if (pollStatus == PollReadStatus::Closed) return RecvTextStatus::Closed;

		WsFrame frame;
		if (!receiveWsFrame(fd_, &frame)) {
			return RecvTextStatus::Closed;
		}

		if (frame.opcode == kWsOpcodeClose) {
			return RecvTextStatus::Closed;
		}
		if (frame.opcode == kWsOpcodePing) {
			std::lock_guard<std::mutex> lock(sendMutex_);
			if (!sendClientFrame(
					fd_,
					kWsOpcodePong,
					reinterpret_cast<const uint8_t*>(frame.payload.data()),
					frame.payload.size())) {
				abortConnection();
				return RecvTextStatus::Closed;
			}
			continue;
		}
		if (frame.opcode == kWsOpcodePong) {
			continue;
		}
		if (frame.opcode == kWsOpcodeText) {
			if (awaitingContinuation) {
				return RecvTextStatus::Closed;
			}
			if (frame.fin) {
				if (text) *text = std::move(frame.payload);
				return RecvTextStatus::Text;
			}
			fragmentedText = std::move(frame.payload);
			awaitingContinuation = true;
			continue;
		}
		if (frame.opcode == kWsOpcodeContinuation) {
			if (!awaitingContinuation) {
				return RecvTextStatus::Closed;
			}
			if (fragmentedText.size() + frame.payload.size() > kMaxIncomingFrameBytes) {
				return RecvTextStatus::Closed;
			}
			fragmentedText += frame.payload;
			if (frame.fin) {
				if (text) *text = std::move(fragmentedText);
				return RecvTextStatus::Text;
			}
			continue;
		}

		// Ignore unsupported non-text data frames rather than treating them as a
		// timeout. The plain client only expects JSON text responses.
	}
}

void WsClient::readerLoop()
{
	while (running_) {
		std::string text;
		const auto recvStatus = recvText(&text, 100);
		if (recvStatus == RecvTextStatus::Timeout) continue;
		if (recvStatus == RecvTextStatus::Closed) {
			running_.store(false);
			break;
		}
		auto msg = json::parse(text, nullptr, false);
		if (msg.is_discarded()) continue;
		if (msg.value("response", false)) {
			handleResponse(msg);
		} else if (msg.contains("method")) {
			std::lock_guard<std::mutex> lock(stateMutex_);
			pendingNotifications_.push_back(std::move(msg));
		}
	}

	failAllPendingRequests("connection closed");
}

json WsClient::request(const std::string& method, const json& reqData, int timeoutMs)
{
	uint32_t id = nextId_.fetch_add(1);
	auto pending = std::make_shared<PendingRequest>();
	{
		std::lock_guard<std::mutex> lock(stateMutex_);
		pendingRequests_[id] = pending;
	}

	json msg = {{"request", true}, {"id", id}, {"method", method}, {"data", reqData}};
	if (!sendText(msg.dump())) {
		{
			std::lock_guard<std::mutex> stateLock(stateMutex_);
			pendingRequests_.erase(id);
		}
		throw std::runtime_error(method + ": connection closed");
	}

	std::unique_lock<std::mutex> lock(pending->mutex);
	bool completed = pending->cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&] {
		return pending->done || !running_;
	});
	if (!completed) {
		std::lock_guard<std::mutex> stateLock(stateMutex_);
		pendingRequests_.erase(id);
		throw std::runtime_error("timeout: " + method);
	}
	if (!pending->done) {
		throw std::runtime_error(method + ": connection closed");
	}
	if (!pending->ok) {
		throw std::runtime_error(method + ": " + pending->error);
	}
	return pending->data;
}

bool WsClient::requestAsync(const std::string& method, const json& reqData,
	std::function<void(bool, const json&, const std::string&)> completion)
{
	if (!running_) {
		if (completion) completion(false, json::object(), "connection closed");
		return false;
	}

	uint32_t id = nextId_.fetch_add(1);
	auto pending = std::make_shared<PendingRequest>();
	pending->completion = std::move(completion);
	{
		std::lock_guard<std::mutex> lock(stateMutex_);
		pendingRequests_[id] = pending;
	}

	json msg = {{"request", true}, {"id", id}, {"method", method}, {"data", reqData}};
	if (!sendText(msg.dump())) {
		{
			std::lock_guard<std::mutex> lock(stateMutex_);
			pendingRequests_.erase(id);
		}
		if (pending->completion) {
			pending->completion(false, json::object(), "connection closed");
		}
		return false;
	}
	return true;
}

void WsClient::setNotificationHandler(NotificationHandler handler)
{
	std::lock_guard<std::mutex> lock(stateMutex_);
	notificationHandler_ = std::move(handler);
}

size_t WsClient::pendingRequestCount() const
{
	std::lock_guard<std::mutex> lock(stateMutex_);
	return pendingRequests_.size();
}

void WsClient::dispatchNotifications()
{
	std::deque<json> notifications;
	NotificationHandler handler;
	{
		std::lock_guard<std::mutex> lock(stateMutex_);
		notifications.swap(pendingNotifications_);
		handler = notificationHandler_;
	}
	while (!notifications.empty()) {
		auto msg = std::move(notifications.front());
		notifications.pop_front();
		if (handler) handler(msg);
	}
}

void WsClient::handleResponse(const json& response)
{
	const uint32_t id = response.value("id", 0u);
	std::shared_ptr<PendingRequest> pending;
	{
		std::lock_guard<std::mutex> lock(stateMutex_);
		auto it = pendingRequests_.find(id);
		if (it == pendingRequests_.end()) return;
		pending = it->second;
		pendingRequests_.erase(it);
	}

	std::function<void(bool, const json&, const std::string&)> completion;
	bool ok = response.value("ok", false);
	json data = response.value("data", json::object());
	std::string error = response.value("error", "unknown");
	{
		std::lock_guard<std::mutex> lock(pending->mutex);
		pending->done = true;
		pending->ok = ok;
		pending->data = data;
		pending->error = error;
		completion = pending->completion;
	}
	pending->cv.notify_all();

	if (completion) {
		completion(ok, data, error);
	}
}

void WsClient::failAllPendingRequests(const std::string& error)
{
	std::unordered_map<uint32_t, std::shared_ptr<PendingRequest>> pending;
	{
		std::lock_guard<std::mutex> lock(stateMutex_);
		pending.swap(pendingRequests_);
	}

	for (auto& entry : pending) {
		auto& req = entry.second;
		std::function<void(bool, const json&, const std::string&)> completion;
		{
			std::lock_guard<std::mutex> lock(req->mutex);
			if (req->done) continue;
			req->done = true;
			req->ok = false;
			req->error = error;
			completion = req->completion;
		}
		req->cv.notify_all();
		if (completion) completion(false, json::object(), error);
	}
}

void WsClient::close()
{
	abortConnection();
	if (readerThread_.joinable()) {
		readerThread_.join();
	}
	if (fd_ >= 0) {
		::close(fd_);
		fd_ = -1;
	}
	failAllPendingRequests("connection closed");
}

void WsClient::abortConnection()
{
	const bool wasRunning = running_.exchange(false);
	if (fd_ >= 0 && wasRunning) {
		::shutdown(fd_, SHUT_RDWR);
	}
}
