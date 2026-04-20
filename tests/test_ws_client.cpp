#include <gtest/gtest.h>

#include "../client/WsClient.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr uint8_t kWsOpcodeContinuation = 0x0;
constexpr uint8_t kWsOpcodeText = 0x1;
constexpr uint8_t kWsOpcodeClose = 0x8;
constexpr uint8_t kWsOpcodePing = 0x9;
constexpr uint8_t kWsOpcodePong = 0xA;
constexpr uint64_t kOversizedFrameBytes = (4ull * 1024ull * 1024ull) + 1ull;

struct WsFrame {
	uint8_t opcode{0};
	bool fin{false};
	std::string payload;
};

class TestWsServer {
public:
	TestWsServer() = default;

	~TestWsServer()
	{
		stop();
	}

	bool start()
	{
		listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
		if (listenFd_ < 0) return false;

		int opt = 1;
		setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		addr.sin_port = 0;
		if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
			return false;
		if (::listen(listenFd_, 1) != 0)
			return false;

		socklen_t addrLen = sizeof(addr);
		if (::getsockname(listenFd_, reinterpret_cast<sockaddr*>(&addr), &addrLen) != 0)
			return false;
		port_ = ntohs(addr.sin_port);
		return true;
	}

	uint16_t port() const { return port_; }

	template<typename Fn>
	void run(Fn fn)
	{
		thread_ = std::thread([this, fn = std::move(fn)]() mutable {
			sockaddr_in peerAddr{};
			socklen_t peerLen = sizeof(peerAddr);
			clientFd_ = ::accept(listenFd_, reinterpret_cast<sockaddr*>(&peerAddr), &peerLen);
			if (clientFd_ < 0) return;

			if (!readHttpHandshake()) return;
			const std::string response =
				"HTTP/1.1 101 Switching Protocols\r\n"
				"Upgrade: websocket\r\n"
				"Connection: Upgrade\r\n"
				"Sec-WebSocket-Accept: test\r\n\r\n";
			::send(clientFd_, response.data(), response.size(), 0);
			fn(clientFd_);
		});
	}

	void stop()
	{
		if (clientFd_ >= 0) {
			::shutdown(clientFd_, SHUT_RDWR);
			::close(clientFd_);
			clientFd_ = -1;
		}
		if (listenFd_ >= 0) {
			::shutdown(listenFd_, SHUT_RDWR);
			::close(listenFd_);
			listenFd_ = -1;
		}
		if (thread_.joinable()) thread_.join();
	}

private:
	bool readHttpHandshake()
	{
		std::string request;
		char buf[1024];
		while (request.find("\r\n\r\n") == std::string::npos) {
			const ssize_t n = ::recv(clientFd_, buf, sizeof(buf), 0);
			if (n <= 0) return false;
			request.append(buf, static_cast<size_t>(n));
		}
		return true;
	}

	int listenFd_{-1};
	int clientFd_{-1};
	uint16_t port_{0};
	std::thread thread_;
};

bool sendAll(int fd, const uint8_t* data, size_t len)
{
	size_t sent = 0;
	while (sent < len) {
		const ssize_t rc = ::send(fd, data + sent, len - sent, 0);
		if (rc <= 0) return false;
		sent += static_cast<size_t>(rc);
	}
	return true;
}

bool readOneWsFrame(int fd, WsFrame* frame)
{
	if (!frame) return false;
	uint8_t hdr[2];
	if (::recv(fd, hdr, 2, MSG_WAITALL) != 2) return false;

	frame->fin = (hdr[0] & 0x80) != 0;
	frame->opcode = hdr[0] & 0x0F;

	uint64_t len = hdr[1] & 0x7F;
	if (len == 126) {
		uint8_t ext[2];
		if (::recv(fd, ext, 2, MSG_WAITALL) != 2) return false;
		len = (static_cast<uint64_t>(ext[0]) << 8) | ext[1];
	} else if (len == 127) {
		uint8_t ext[8];
		if (::recv(fd, ext, 8, MSG_WAITALL) != 8) return false;
		len = 0;
		for (int i = 0; i < 8; ++i)
			len = (len << 8) | ext[i];
	}

	if ((hdr[1] & 0x80) != 0) {
		uint8_t mask[4];
		if (::recv(fd, mask, 4, MSG_WAITALL) != 4) return false;
		frame->payload.resize(static_cast<size_t>(len));
		size_t got = 0;
		while (got < len) {
			const ssize_t n = ::recv(fd, frame->payload.data() + got, len - got, 0);
			if (n <= 0) return false;
			got += static_cast<size_t>(n);
		}
		for (size_t i = 0; i < frame->payload.size(); ++i)
			frame->payload[i] ^= mask[i % 4];
		return true;
	}

	frame->payload.resize(static_cast<size_t>(len));
	size_t got = 0;
	while (got < len) {
		const ssize_t n = ::recv(fd, frame->payload.data() + got, len - got, 0);
		if (n <= 0) return false;
		got += static_cast<size_t>(n);
	}
	return true;
}

bool sendWsFrame(int fd, uint8_t opcode, const std::string& payload, bool fin = true)
{
	std::vector<uint8_t> frame;
	frame.reserve(2 + 8 + payload.size());
	frame.push_back(static_cast<uint8_t>((fin ? 0x80 : 0x00) | (opcode & 0x0F)));

	if (payload.size() < 126) {
		frame.push_back(static_cast<uint8_t>(payload.size()));
	} else if (payload.size() < 65536) {
		frame.push_back(126);
		frame.push_back(static_cast<uint8_t>((payload.size() >> 8) & 0xFF));
		frame.push_back(static_cast<uint8_t>(payload.size() & 0xFF));
	} else {
		frame.push_back(127);
		for (int i = 7; i >= 0; --i)
			frame.push_back(static_cast<uint8_t>((payload.size() >> (i * 8)) & 0xFF));
	}

	frame.insert(frame.end(), payload.begin(), payload.end());
	return sendAll(fd, frame.data(), frame.size());
}

bool sendOversizedTextHeader(int fd, uint64_t payloadSize)
{
	std::vector<uint8_t> frame = {
		static_cast<uint8_t>(0x80 | kWsOpcodeText),
		127
	};
	for (int i = 7; i >= 0; --i)
		frame.push_back(static_cast<uint8_t>((payloadSize >> (i * 8)) & 0xFF));
	return sendAll(fd, frame.data(), frame.size());
}

} // namespace

TEST(WsClientTest, RemoteCloseFailsPendingAsyncRequestPromptly)
{
	TestWsServer server;
	ASSERT_TRUE(server.start());
	server.run([](int clientFd) {
		WsFrame request;
		ASSERT_TRUE(readOneWsFrame(clientFd, &request));
		::shutdown(clientFd, SHUT_RDWR);
		::close(clientFd);
	});

	WsClient client;
	ASSERT_TRUE(client.connect("127.0.0.1", server.port(), "/ws"));

	std::promise<std::string> completion;
	auto future = completion.get_future();
	ASSERT_TRUE(client.requestAsync("ping", json::object(),
		[&completion](bool ok, const json&, const std::string& error) {
			completion.set_value(ok ? "ok" : error);
		}));

	ASSERT_EQ(future.wait_for(std::chrono::seconds(2)), std::future_status::ready);
	EXPECT_EQ(future.get(), "connection closed");

	client.close();
	server.stop();
}

TEST(WsClientTest, PingTriggersPongAndRequestStillCompletes)
{
	TestWsServer server;
	ASSERT_TRUE(server.start());
	server.run([](int clientFd) {
		WsFrame request;
		ASSERT_TRUE(readOneWsFrame(clientFd, &request));
		EXPECT_EQ(request.opcode, kWsOpcodeText);

		ASSERT_TRUE(sendWsFrame(clientFd, kWsOpcodePing, "probe"));

		WsFrame pong;
		ASSERT_TRUE(readOneWsFrame(clientFd, &pong));
		EXPECT_EQ(pong.opcode, kWsOpcodePong);
		EXPECT_EQ(pong.payload, "probe");

		ASSERT_TRUE(sendWsFrame(
			clientFd,
			kWsOpcodeText,
			R"({"response":true,"id":1,"ok":true,"data":{"value":7}})"));
	});

	WsClient client;
	ASSERT_TRUE(client.connect("127.0.0.1", server.port(), "/ws"));

	const auto response = client.request("ping", json::object(), 2000);
	EXPECT_EQ(response.value("value", 0), 7);

	client.close();
	server.stop();
}

TEST(WsClientTest, FragmentedTextResponseIsReassembled)
{
	TestWsServer server;
	ASSERT_TRUE(server.start());
	server.run([](int clientFd) {
		WsFrame request;
		ASSERT_TRUE(readOneWsFrame(clientFd, &request));
		EXPECT_EQ(request.opcode, kWsOpcodeText);

		ASSERT_TRUE(sendWsFrame(
			clientFd,
			kWsOpcodeText,
			R"({"response":true,"id":1,)",
			false));
		ASSERT_TRUE(sendWsFrame(
			clientFd,
			kWsOpcodeContinuation,
			R"("ok":true,"data":{"fragmented":true}})"));
	});

	WsClient client;
	ASSERT_TRUE(client.connect("127.0.0.1", server.port(), "/ws"));

	const auto response = client.request("fragmented", json::object(), 2000);
	EXPECT_TRUE(response.value("fragmented", false));

	client.close();
	server.stop();
}

TEST(WsClientTest, OversizedIncomingFrameFailsPendingRequestPromptly)
{
	TestWsServer server;
	ASSERT_TRUE(server.start());
	server.run([](int clientFd) {
		WsFrame request;
		ASSERT_TRUE(readOneWsFrame(clientFd, &request));
		EXPECT_EQ(request.opcode, kWsOpcodeText);
		ASSERT_TRUE(sendOversizedTextHeader(clientFd, kOversizedFrameBytes));

		struct pollfd pfd{clientFd, POLLIN, 0};
		(void)poll(&pfd, 1, 500);
		::shutdown(clientFd, SHUT_RDWR);
		::close(clientFd);
	});

	WsClient client;
	ASSERT_TRUE(client.connect("127.0.0.1", server.port(), "/ws"));

	std::promise<std::string> completion;
	auto future = completion.get_future();
	ASSERT_TRUE(client.requestAsync("oversized", json::object(),
		[&completion](bool ok, const json&, const std::string& error) {
			completion.set_value(ok ? "ok" : error);
		}));

	ASSERT_EQ(future.wait_for(std::chrono::seconds(2)), std::future_status::ready);
	EXPECT_EQ(future.get(), "connection closed");

	client.close();
	server.stop();
}
