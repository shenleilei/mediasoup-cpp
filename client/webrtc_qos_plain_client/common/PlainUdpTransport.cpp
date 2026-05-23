#include "common/PlainUdpTransport.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

namespace webrtc_qos_plain {
namespace {

bool BuildSockaddr(const std::string& ip, uint16_t port, sockaddr_in* out, std::string* error)
{
	if (!out) return false;
	*out = {};
	out->sin_family = AF_INET;
	out->sin_port = htons(port);
	if (inet_pton(AF_INET, ip.c_str(), &out->sin_addr) != 1) {
		if (error) *error = "invalid IPv4 address: " + ip;
		return false;
	}
	return true;
}

std::string SockaddrIp(const sockaddr_in& addr)
{
	char text[INET_ADDRSTRLEN] = {};
	const char* rc = inet_ntop(AF_INET, &addr.sin_addr, text, sizeof(text));
	return rc ? std::string(rc) : std::string("0.0.0.0");
}

std::string ErrnoString(const char* operation)
{
	return std::string(operation) + ": " + std::strerror(errno);
}

} // namespace

PlainUdpTransport::~PlainUdpTransport()
{
	Close();
}

PlainUdpTransport::PlainUdpTransport(PlainUdpTransport&& other) noexcept
	: fd_(other.fd_),
	  local_(std::move(other.local_)),
	  remote_(std::move(other.remote_))
{
	other.fd_ = -1;
	other.local_ = {};
	other.remote_ = {};
}

PlainUdpTransport& PlainUdpTransport::operator=(PlainUdpTransport&& other) noexcept
{
	if (this == &other) return *this;
	Close();
	fd_ = other.fd_;
	local_ = std::move(other.local_);
	remote_ = std::move(other.remote_);
	other.fd_ = -1;
	other.local_ = {};
	other.remote_ = {};
	return *this;
}

bool PlainUdpTransport::Bind(const std::string& ip, uint16_t port, std::string* error)
{
	Close();
	fd_ = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd_ < 0) {
		if (error) *error = ErrnoString("socket");
		return false;
	}

	int reuse = 1;
	(void)setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	sockaddr_in addr;
	if (!BuildSockaddr(ip, port, &addr, error)) {
		Close();
		return false;
	}
	if (bind(fd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0) {
		if (error) *error = ErrnoString("bind");
		Close();
		return false;
	}
	if (!SetNonBlocking(error) || !RefreshLocalEndpoint(error)) {
		Close();
		return false;
	}
	return true;
}

bool PlainUdpTransport::Connect(const std::string& remoteIp, uint16_t remotePort, std::string* error)
{
	if (fd_ < 0) {
		fd_ = socket(AF_INET, SOCK_DGRAM, 0);
		if (fd_ < 0) {
			if (error) *error = ErrnoString("socket");
			return false;
		}
		if (!SetNonBlocking(error)) {
			Close();
			return false;
		}
	}

	sockaddr_in addr;
	if (!BuildSockaddr(remoteIp, remotePort, &addr, error)) return false;
	if (connect(fd_, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0) {
		if (error) *error = ErrnoString("connect");
		return false;
	}
	remote_ = {remoteIp, remotePort};
	return RefreshLocalEndpoint(error);
}

bool PlainUdpTransport::Send(const uint8_t* data, size_t size, std::string* error)
{
	if (fd_ < 0) {
		if (error) *error = "UDP socket is not open";
		return false;
	}
	const ssize_t sent = send(fd_, data, size, 0);
	if (sent == static_cast<ssize_t>(size)) return true;
	if (error) *error = sent < 0 ? ErrnoString("send") : "partial UDP send";
	return false;
}

bool PlainUdpTransport::SendTo(const std::string& remoteIp, uint16_t remotePort,
	const uint8_t* data, size_t size, std::string* error)
{
	if (fd_ < 0) {
		if (error) *error = "UDP socket is not open";
		return false;
	}
	sockaddr_in addr;
	if (!BuildSockaddr(remoteIp, remotePort, &addr, error)) return false;
	const ssize_t sent = sendto(
		fd_,
		data,
		size,
		0,
		reinterpret_cast<const sockaddr*>(&addr),
		sizeof(addr));
	if (sent == static_cast<ssize_t>(size)) return true;
	if (error) *error = sent < 0 ? ErrnoString("sendto") : "partial UDP sendto";
	return false;
}

ssize_t PlainUdpTransport::Recv(uint8_t* data, size_t capacity, UdpEndpoint* from, std::string* error)
{
	if (fd_ < 0) {
		if (error) *error = "UDP socket is not open";
		return -1;
	}
	sockaddr_in addr{};
	socklen_t len = sizeof(addr);
	const ssize_t received = recvfrom(
		fd_,
		data,
		capacity,
		0,
		reinterpret_cast<sockaddr*>(&addr),
		&len);
	if (received >= 0) {
		if (from) *from = {SockaddrIp(addr), ntohs(addr.sin_port)};
		return received;
	}
	if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
	if (error) *error = ErrnoString("recvfrom");
	return -1;
}

void PlainUdpTransport::Close()
{
	if (fd_ >= 0) {
		close(fd_);
		fd_ = -1;
	}
	local_ = {};
	remote_ = {};
}

bool PlainUdpTransport::SetNonBlocking(std::string* error)
{
	const int flags = fcntl(fd_, F_GETFL, 0);
	if (flags < 0 || fcntl(fd_, F_SETFL, flags | O_NONBLOCK) != 0) {
		if (error) *error = ErrnoString("fcntl O_NONBLOCK");
		return false;
	}
	return true;
}

bool PlainUdpTransport::RefreshLocalEndpoint(std::string* error)
{
	sockaddr_in addr{};
	socklen_t len = sizeof(addr);
	if (getsockname(fd_, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
		if (error) *error = ErrnoString("getsockname");
		return false;
	}
	local_ = {SockaddrIp(addr), ntohs(addr.sin_port)};
	return true;
}

} // namespace webrtc_qos_plain
