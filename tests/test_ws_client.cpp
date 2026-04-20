#include <gtest/gtest.h>

#include "../client/WsClient.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <thread>

namespace {

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

bool readOneWsFrame(int fd)
{
	uint8_t hdr[2];
	if (::recv(fd, hdr, 2, MSG_WAITALL) != 2) return false;

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
	}

	std::string payload(len, '\0');
	size_t got = 0;
	while (got < len) {
		const ssize_t n = ::recv(fd, payload.data() + got, len - got, 0);
		if (n <= 0) return false;
		got += static_cast<size_t>(n);
	}
	return true;
}

} // namespace

TEST(WsClientTest, RemoteCloseFailsPendingAsyncRequestPromptly)
{
	TestWsServer server;
	ASSERT_TRUE(server.start());
	server.run([](int clientFd) {
		ASSERT_TRUE(readOneWsFrame(clientFd));
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
