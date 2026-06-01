#include "HawkeyeRegisterClient.h"

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <netdb.h>
#include <openssl/sha.h>
#include <poll.h>
#include <random>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace mediasoup {
namespace {

constexpr std::chrono::seconds kReconnectDelay{ 5 };
constexpr std::chrono::seconds kPingInterval{ 15 };
constexpr std::chrono::seconds kPongTimeout{ 10 };

void logLine(const char* level, const std::string& message)
{
	std::fprintf(stderr, "[hawkeye-register][%s] %s\n", level, message.c_str());
}

template <typename... Args>
void logf(const char* level, const char* fmt, Args&&... args)
{
	char buffer[1024];
	std::snprintf(buffer, sizeof(buffer), fmt, std::forward<Args>(args)...);
	logLine(level, buffer);
}

std::string escapeJson(const std::string& value)
{
	std::string out;
	out.reserve(value.size() + 8);
	for (char ch : value) {
		switch (ch) {
		case '\\': out += "\\\\"; break;
		case '"': out += "\\\""; break;
		case '\b': out += "\\b"; break;
		case '\f': out += "\\f"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			out.push_back(ch);
			break;
		}
	}
	return out;
}

} // namespace

HawkeyeRegisterClient::HawkeyeRegisterClient(std::string registerUrl, std::string server, std::string type)
	: registerUrl_(std::move(registerUrl))
	, server_(std::move(server))
	, type_(std::move(type))
{}

HawkeyeRegisterClient::~HawkeyeRegisterClient()
{
	stop();
}

bool HawkeyeRegisterClient::enabled() const
{
	return !registerUrl_.empty() && !server_.empty();
}

void HawkeyeRegisterClient::start()
{
	if (!enabled() || thread_.joinable()) {
		return;
	}
	stop_.store(false, std::memory_order_relaxed);
	thread_ = std::thread([this] { run(); });
}

void HawkeyeRegisterClient::stop()
{
	stop_.store(true, std::memory_order_relaxed);
	const int fd = socketFd_.exchange(-1, std::memory_order_relaxed);
	if (fd >= 0) {
		::shutdown(fd, SHUT_RDWR);
		::close(fd);
	}
	if (thread_.joinable()) {
		thread_.join();
	}
}

void HawkeyeRegisterClient::run()
{
	while (!stop_.load(std::memory_order_relaxed)) {
		if (!connectAndRegister()) {
			if (stop_.load(std::memory_order_relaxed)) {
				break;
			}
			std::this_thread::sleep_for(kReconnectDelay);
			continue;
		}

		if (stop_.load(std::memory_order_relaxed)) {
			break;
		}

		std::this_thread::sleep_for(kReconnectDelay);
	}
}

bool HawkeyeRegisterClient::connectAndRegister()
{
	ParsedUrl url;
	std::string error;
	if (!parseUrl(registerUrl_, url, error)) {
		logf("error", "url parse failed [url:%s error:%s]", registerUrl_.c_str(), error.c_str());
		return false;
	}
	if (url.secure) {
		logf("error", "wss is not supported [url:%s]", registerUrl_.c_str());
		return false;
	}

	int fd = -1;
	if (!openSocket(url, fd, error)) {
		logf("warn", "connect failed [url:%s error:%s]", registerUrl_.c_str(), error.c_str());
		return false;
	}
	socketFd_.store(fd, std::memory_order_relaxed);
	auto cleanupSocket = [&] {
		const int currentFd = socketFd_.exchange(-1, std::memory_order_relaxed);
		if (currentFd >= 0) {
			::shutdown(currentFd, SHUT_RDWR);
			::close(currentFd);
		}
	};

	const std::string key = buildWsKey();
	if (!performHandshake(fd, url, key, error)) {
		logf("warn", "handshake failed [url:%s error:%s]", registerUrl_.c_str(), error.c_str());
		cleanupSocket();
		return false;
	}

	if (!sendRegisterFrame(fd, error)) {
		logf("warn", "register send failed [url:%s error:%s]", registerUrl_.c_str(), error.c_str());
		cleanupSocket();
		return false;
	}

	if (!readLoop(fd, error)) {
		if (!stop_.load(std::memory_order_relaxed)) {
			logf("warn", "disconnected [url:%s error:%s]", registerUrl_.c_str(), error.c_str());
		}
	}

	cleanupSocket();
	return false;
}

bool HawkeyeRegisterClient::openSocket(const ParsedUrl& url, int& fd, std::string& error) const
{
	addrinfo hints{};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	addrinfo* result = nullptr;
	const int rc = ::getaddrinfo(url.host.c_str(), url.port.c_str(), &hints, &result);
	if (rc != 0) {
		error = ::gai_strerror(rc);
		return false;
	}

	for (addrinfo* ai = result; ai != nullptr; ai = ai->ai_next) {
		fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd < 0) {
			continue;
		}
		if (::connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
			::freeaddrinfo(result);
			return true;
		}
		::close(fd);
		fd = -1;
	}

	if (fd < 0) {
		error = "socket connect failed";
	} else {
		error = std::strerror(errno);
	}
	::freeaddrinfo(result);
	return false;
}

bool HawkeyeRegisterClient::performHandshake(int fd, const ParsedUrl& url, const std::string& key, std::string& error) const
{
	const std::string hostHeader = url.portNumber == 80 ? url.host : (url.host + ":" + url.port);
	const std::string request =
		"GET " + url.path + " HTTP/1.1\r\n"
		"Host: " + hostHeader + "\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Key: " + key + "\r\n"
		"Sec-WebSocket-Version: 13\r\n"
		"\r\n";

	size_t written = 0;
	while (written < request.size()) {
		const ssize_t n = ::send(fd, request.data() + written, request.size() - written, 0);
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			error = std::strerror(errno);
			return false;
		}
		written += static_cast<size_t>(n);
	}

	std::string response;
	response.reserve(1024);
	char buffer[1024];
	while (response.find("\r\n\r\n") == std::string::npos) {
		const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
		if (n <= 0) {
			error = (n == 0) ? "handshake closed" : std::strerror(errno);
			return false;
		}
		response.append(buffer, buffer + n);
		if (response.size() > 8192) {
			error = "handshake response too large";
			return false;
		}
	}

	if (response.find(" 101 ") == std::string::npos) {
		error = "unexpected handshake status";
		return false;
	}

	const std::string accept = headerValue(response, "Sec-WebSocket-Accept");
	const std::string expectedAccept = sha1Base64(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
	if (!accept.empty() && accept != expectedAccept) {
		error = "Sec-WebSocket-Accept mismatch";
		return false;
	}
	return true;
}

bool HawkeyeRegisterClient::sendRegisterFrame(int fd, std::string& error) const
{
	const std::string payload =
		"{\"server\":\"" + escapeJson(server_) + "\","
		"\"type\":\"" + escapeJson(type_) + "\"}";
	return sendFrame(fd, 0x1, payload, error);
}

bool HawkeyeRegisterClient::sendFrame(int fd, uint8_t opcode, std::string_view payload, std::string& error) const
{
	std::array<unsigned char, 4> mask{};
	std::random_device rd;
	for (auto& byte : mask) {
		byte = static_cast<unsigned char>(rd());
	}

	std::string frame;
	frame.reserve(payload.size() + 16);
	frame.push_back(static_cast<char>(0x80 | (opcode & 0x0F)));

	const size_t len = payload.size();
	if (len < 126) {
		frame.push_back(static_cast<char>(0x80 | static_cast<unsigned char>(len)));
	} else if (len <= 0xFFFF) {
		frame.push_back(static_cast<char>(0x80 | 126));
		frame.push_back(static_cast<char>((len >> 8) & 0xFF));
		frame.push_back(static_cast<char>(len & 0xFF));
	} else {
		frame.push_back(static_cast<char>(0x80 | 127));
		for (int shift = 7; shift >= 0; --shift) {
			frame.push_back(static_cast<char>((static_cast<uint64_t>(len) >> (shift * 8)) & 0xFF));
		}
	}

	frame.append(reinterpret_cast<const char*>(mask.data()), mask.size());
	const size_t maskOffset = frame.size();
	frame.resize(maskOffset + payload.size());
	for (size_t i = 0; i < payload.size(); ++i) {
		frame[maskOffset + i] = static_cast<char>(payload[i] ^ mask[i % 4]);
	}

	size_t written = 0;
	while (written < frame.size()) {
		const ssize_t n = ::send(fd, frame.data() + written, frame.size() - written, 0);
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			error = std::strerror(errno);
			return false;
		}
		written += static_cast<size_t>(n);
	}
	return true;
}

bool HawkeyeRegisterClient::readFrame(std::string& buffered, Frame& frame) const
{
	if (buffered.size() < 2) {
		return false;
	}

	const unsigned char b0 = static_cast<unsigned char>(buffered[0]);
	const unsigned char b1 = static_cast<unsigned char>(buffered[1]);
	const bool masked = (b1 & 0x80) != 0;
	uint64_t payloadLen = static_cast<uint64_t>(b1 & 0x7F);
	size_t offset = 2;
	if (payloadLen == 126) {
		if (buffered.size() < offset + 2) {
			return false;
		}
		payloadLen =
			(static_cast<uint64_t>(static_cast<unsigned char>(buffered[offset])) << 8) |
			static_cast<uint64_t>(static_cast<unsigned char>(buffered[offset + 1]));
		offset += 2;
	} else if (payloadLen == 127) {
		if (buffered.size() < offset + 8) {
			return false;
		}
		payloadLen = 0;
		for (int i = 0; i < 8; ++i) {
			payloadLen = (payloadLen << 8) |
				static_cast<uint64_t>(static_cast<unsigned char>(buffered[offset + i]));
		}
		offset += 8;
	}

	std::array<unsigned char, 4> mask{};
	if (masked) {
		if (buffered.size() < offset + mask.size()) {
			return false;
		}
		for (size_t i = 0; i < mask.size(); ++i) {
			mask[i] = static_cast<unsigned char>(buffered[offset + i]);
		}
		offset += mask.size();
	}

	if (buffered.size() < offset + payloadLen) {
		return false;
	}

	frame.opcode = b0 & 0x0F;
	frame.payload.assign(buffered.begin() + offset, buffered.begin() + offset + static_cast<size_t>(payloadLen));
	if (masked) {
		for (size_t i = 0; i < frame.payload.size(); ++i) {
			frame.payload[i] = static_cast<char>(static_cast<unsigned char>(frame.payload[i]) ^ mask[i % 4]);
		}
	}

	buffered.erase(0, offset + static_cast<size_t>(payloadLen));
	return true;
}

bool HawkeyeRegisterClient::readLoop(int fd, std::string& error)
{
	std::string buffered;
	buffered.reserve(4096);
	auto lastPing = std::chrono::steady_clock::now();
	bool waitingForPong = false;

	while (!stop_.load(std::memory_order_relaxed)) {
		pollfd pfd{};
		pfd.fd = fd;
		pfd.events = POLLIN;

		const int rc = ::poll(&pfd, 1, 1000);
		const auto now = std::chrono::steady_clock::now();
		if (rc < 0) {
			if (errno == EINTR) {
				continue;
			}
			error = std::strerror(errno);
			return false;
		}

		if (rc == 0) {
			if (waitingForPong && now - lastPing > kPongTimeout) {
				error = "pong timeout";
				return false;
			}
			if (!waitingForPong && now - lastPing > kPingInterval) {
				if (!sendFrame(fd, 0x9, "", error)) {
					return false;
				}
				waitingForPong = true;
				lastPing = now;
			}
			continue;
		}

		if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
			error = "socket closed";
			return false;
		}

		char buffer[4096];
		const ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
		if (n <= 0) {
			error = (n == 0) ? "socket closed" : std::strerror(errno);
			return false;
		}
		buffered.append(buffer, buffer + n);

		Frame frame;
		while (readFrame(buffered, frame)) {
			switch (frame.opcode) {
			case 0x8:
				error = "close frame";
				return false;
			case 0x9:
				if (!sendFrame(fd, 0xA, frame.payload, error)) {
					return false;
				}
				break;
			case 0xA:
				waitingForPong = false;
				break;
			default:
				break;
			}
		}
	}

	return false;
}

std::string HawkeyeRegisterClient::buildWsKey()
{
	std::array<unsigned char, 16> bytes{};
	std::random_device rd;
	for (auto& byte : bytes) {
		byte = static_cast<unsigned char>(rd());
	}
	return base64Encode(bytes.data(), bytes.size());
}

std::string HawkeyeRegisterClient::base64Encode(const unsigned char* data, size_t len)
{
	static constexpr char kAlphabet[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	out.reserve(((len + 2) / 3) * 4);
	for (size_t i = 0; i < len; i += 3) {
		const unsigned int n =
			static_cast<unsigned int>(data[i]) << 16 |
			(static_cast<unsigned int>(i + 1 < len ? data[i + 1] : 0) << 8) |
			static_cast<unsigned int>(i + 2 < len ? data[i + 2] : 0);
		out.push_back(kAlphabet[(n >> 18) & 0x3F]);
		out.push_back(kAlphabet[(n >> 12) & 0x3F]);
		out.push_back(i + 1 < len ? kAlphabet[(n >> 6) & 0x3F] : '=');
		out.push_back(i + 2 < len ? kAlphabet[n & 0x3F] : '=');
	}
	return out;
}

std::string HawkeyeRegisterClient::sha1Base64(const std::string& value)
{
	unsigned char digest[SHA_DIGEST_LENGTH]{};
	::SHA1(reinterpret_cast<const unsigned char*>(value.data()), value.size(), digest);
	return base64Encode(digest, sizeof(digest));
}

bool HawkeyeRegisterClient::parseUrl(const std::string& value, ParsedUrl& out, std::string& error)
{
	const auto schemeEnd = value.find("://");
	if (schemeEnd == std::string::npos) {
		error = "missing scheme";
		logf("error", "parseUrl failed [url:%s error:%s]", value.c_str(), error.c_str());
		return false;
	}
	const std::string scheme = toLowerCopy(value.substr(0, schemeEnd));
	if (scheme != "ws" && scheme != "wss") {
		error = "unsupported scheme";
		logf("error", "parseUrl failed [url:%s error:%s]", value.c_str(), error.c_str());
		return false;
	}
	out.secure = scheme == "wss";

	const std::string rest = value.substr(schemeEnd + 3);
	const auto pathPos = rest.find('/');
	const std::string authority = pathPos == std::string::npos ? rest : rest.substr(0, pathPos);
	out.path = pathPos == std::string::npos ? "/register_ws" : rest.substr(pathPos);
	if (out.path.empty() || out.path == "/") {
		out.path = "/register_ws";
	}

	const auto colonPos = authority.rfind(':');
	if (authority.empty()) {
		error = "missing host";
		logf("error", "parseUrl failed [url:%s error:%s]", value.c_str(), error.c_str());
		return false;
	}
	if (colonPos == std::string::npos) {
		out.host = authority;
		out.port = out.secure ? "443" : "80";
		out.portNumber = out.secure ? 443 : 80;
		return true;
	}

	out.host = authority.substr(0, colonPos);
	out.port = authority.substr(colonPos + 1);
	if (out.host.empty() || out.port.empty()) {
		error = "invalid authority";
		logf("error", "parseUrl failed [url:%s error:%s]", value.c_str(), error.c_str());
		return false;
	}

	try {
		out.portNumber = std::stoi(out.port);
	} catch (...) {
		error = "invalid port";
		logf("error", "parseUrl failed [url:%s error:%s]", value.c_str(), error.c_str());
		return false;
	}
	if (out.portNumber <= 0 || out.portNumber > 65535) {
		error = "port out of range";
		logf("error", "parseUrl failed [url:%s error:%s]", value.c_str(), error.c_str());
		return false;
	}
	return true;
}

std::string HawkeyeRegisterClient::toLowerCopy(std::string value)
{
	for (char& ch : value) {
		ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
	}
	return value;
}

std::string HawkeyeRegisterClient::trimCopy(const std::string& value)
{
	size_t start = 0;
	while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
		++start;
	}
	size_t end = value.size();
	while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
		--end;
	}
	return value.substr(start, end - start);
}

std::string HawkeyeRegisterClient::headerValue(const std::string& headers, const std::string& name)
{
	std::istringstream stream(headers);
	std::string line;
	const std::string needle = toLowerCopy(name);
	while (std::getline(stream, line)) {
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		const auto colonPos = line.find(':');
		if (colonPos == std::string::npos) {
			continue;
		}
		std::string key = toLowerCopy(trimCopy(line.substr(0, colonPos)));
		if (key != needle) {
			continue;
		}
		return trimCopy(line.substr(colonPos + 1));
	}
	return {};
}

} // namespace mediasoup
