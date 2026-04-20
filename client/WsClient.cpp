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

#include <cstring>
#include <random>
#include <stdexcept>
#include <vector>

namespace {

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
	fd = socket(res->ai_family, SOCK_STREAM, 0);
	if (fd < 0) {
		freeaddrinfo(res);
		fd = -1;
		return false;
	}
	if (::connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
		freeaddrinfo(res);
		::close(fd);
		fd = -1;
		return false;
	}
	freeaddrinfo(res);
	int flag = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

	unsigned char keyBytes[16];
	std::random_device rd;
	for (auto& b : keyBytes) b = rd() & 0xFF;
	std::string key = base64Encode(keyBytes, 16);

	std::string req = "GET " + path + " HTTP/1.1\r\n"
		"Host: " + host + ":" + std::to_string(port) + "\r\n"
		"Upgrade: websocket\r\nConnection: Upgrade\r\n"
		"Sec-WebSocket-Key: " + key + "\r\n"
		"Sec-WebSocket-Version: 13\r\n\r\n";
	::send(fd, req.data(), req.size(), 0);

	char buf[4096];
	int n = ::recv(fd, buf, sizeof(buf) - 1, 0);
	if (n <= 0) {
		::close(fd);
		fd = -1;
		return false;
	}
	buf[n] = 0;
	if (strstr(buf, "101") == nullptr) {
		::close(fd);
		fd = -1;
		return false;
	}

	running_ = true;
	readerThread_ = std::thread([this]() { readerLoop(); });
	return true;
}

void WsClient::sendText(const std::string& msg)
{
	std::lock_guard<std::mutex> lock(sendMutex_);
	if (fd < 0) return;
	std::vector<uint8_t> frame;
	frame.push_back(0x81);
	uint8_t maskKey[4];
	std::random_device rd;
	for (auto& b : maskKey) b = rd() & 0xFF;

	if (msg.size() < 126) {
		frame.push_back(0x80 | static_cast<uint8_t>(msg.size()));
	} else if (msg.size() < 65536) {
		frame.push_back(0x80 | 126);
		frame.push_back((msg.size() >> 8) & 0xFF);
		frame.push_back(msg.size() & 0xFF);
	} else {
		frame.push_back(0x80 | 127);
		for (int i = 7; i >= 0; i--)
			frame.push_back((msg.size() >> (i * 8)) & 0xFF);
	}
	frame.insert(frame.end(), maskKey, maskKey + 4);
	for (size_t i = 0; i < msg.size(); i++)
		frame.push_back(msg[i] ^ maskKey[i % 4]);
	::send(fd, frame.data(), frame.size(), 0);
}

WsClient::RecvTextStatus WsClient::recvText(std::string* text, int timeoutMs)
{
	if (text) text->clear();
	if (fd < 0) return RecvTextStatus::Closed;

	struct pollfd pfd{fd, POLLIN, 0};
	const int pollRc = poll(&pfd, 1, timeoutMs);
	if (pollRc == 0) return RecvTextStatus::Timeout;
	if (pollRc < 0) return RecvTextStatus::Closed;
	if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
		return RecvTextStatus::Closed;
	if ((pfd.revents & POLLIN) == 0) return RecvTextStatus::Timeout;

	uint8_t hdr[2];
	if (::recv(fd, hdr, 2, MSG_WAITALL) != 2) return RecvTextStatus::Closed;
	const uint8_t opcode = hdr[0] & 0x0F;
	bool masked = hdr[1] & 0x80;
	uint64_t len = hdr[1] & 0x7F;
	if (len == 126) {
		uint8_t ext[2];
		if (::recv(fd, ext, 2, MSG_WAITALL) != 2) return RecvTextStatus::Closed;
		len = (ext[0] << 8) | ext[1];
	} else if (len == 127) {
		uint8_t ext[8];
		if (::recv(fd, ext, 8, MSG_WAITALL) != 8) return RecvTextStatus::Closed;
		len = 0;
		for (int i = 0; i < 8; i++) len = (len << 8) | ext[i];
	}
	uint8_t mask[4] = {};
	if (masked && ::recv(fd, mask, 4, MSG_WAITALL) != 4) return RecvTextStatus::Closed;

	std::string data(len, 0);
	size_t got = 0;
	while (got < len) {
		int n = ::recv(fd, &data[got], len - got, 0);
		if (n <= 0) return RecvTextStatus::Closed;
		got += n;
	}
	if (masked) for (size_t i = 0; i < len; i++) data[i] ^= mask[i % 4];
	if (opcode == 0x8) return RecvTextStatus::Closed;
	if (opcode != 0x1) return RecvTextStatus::Timeout;
	if (text) *text = std::move(data);
	return RecvTextStatus::Text;
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
	sendText(msg.dump());

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
	sendText(msg.dump());
	return true;
}

size_t WsClient::pendingRequestCount() const
{
	std::lock_guard<std::mutex> lock(stateMutex_);
	return pendingRequests_.size();
}

void WsClient::dispatchNotifications()
{
	std::deque<json> notifications;
	{
		std::lock_guard<std::mutex> lock(stateMutex_);
		notifications.swap(pendingNotifications_);
	}
	while (!notifications.empty()) {
		auto msg = std::move(notifications.front());
		notifications.pop_front();
		if (onNotification) onNotification(msg);
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
	const bool wasRunning = running_.exchange(false);
	if (fd >= 0 && wasRunning) {
		::shutdown(fd, SHUT_RDWR);
	}
	if (readerThread_.joinable()) {
		readerThread_.join();
	}
	if (fd >= 0) {
		::close(fd);
		fd = -1;
	}
	failAllPendingRequests("connection closed");
}
