#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace webrtc_qos_plain {

struct UdpEndpoint {
	std::string ip;
	uint16_t port = 0;
};

class PlainUdpTransport {
public:
	PlainUdpTransport() = default;
	~PlainUdpTransport();

	PlainUdpTransport(const PlainUdpTransport&) = delete;
	PlainUdpTransport& operator=(const PlainUdpTransport&) = delete;
	PlainUdpTransport(PlainUdpTransport&& other) noexcept;
	PlainUdpTransport& operator=(PlainUdpTransport&& other) noexcept;

	bool Bind(const std::string& ip, uint16_t port, std::string* error);
	bool Connect(const std::string& remoteIp, uint16_t remotePort, std::string* error);
	bool Send(const uint8_t* data, size_t size, std::string* error);
	bool SendTo(const std::string& remoteIp, uint16_t remotePort,
		const uint8_t* data, size_t size, std::string* error);
	ssize_t Recv(uint8_t* data, size_t capacity, UdpEndpoint* from, std::string* error);
	void Close();

	int fd() const { return fd_; }
	UdpEndpoint localEndpoint() const { return local_; }
	UdpEndpoint remoteEndpoint() const { return remote_; }

private:
	bool SetNonBlocking(std::string* error);
	bool RefreshLocalEndpoint(std::string* error);

	int fd_{-1};
	UdpEndpoint local_;
	UdpEndpoint remote_;
};

} // namespace webrtc_qos_plain
