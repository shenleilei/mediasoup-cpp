#pragma once

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <sys/socket.h>

namespace mediasoup::plainclient {

enum class PacketClass {
	Control = 0,
	AudioRtp = 1,
	VideoRetransmission = 2,
	VideoMedia = 3
};

enum class SendStatus {
	Sent,
	WouldBlock,
	HardError
};

struct SendResult {
	SendStatus status{SendStatus::Sent};
	int error{0};
	size_t bytesSent{0};
};

inline bool IsWouldBlockErrno(int error)
{
	return error == EAGAIN || error == EWOULDBLOCK || error == ENOBUFS;
}

inline SendResult SendUdpDatagram(int fd, const uint8_t* data, size_t len)
{
	while (true) {
		ssize_t sent = ::send(fd, data, len, 0);
		if (sent >= 0) {
			if (static_cast<size_t>(sent) == len) {
				return {SendStatus::Sent, 0, static_cast<size_t>(sent)};
			}
			return {SendStatus::HardError, 0, static_cast<size_t>(sent)};
		}

		const int error = errno;
		if (error == EINTR) {
			continue;
		}
		if (IsWouldBlockErrno(error)) {
			return {SendStatus::WouldBlock, error, 0};
		}
		return {SendStatus::HardError, error, 0};
	}
}

using DatagramSendFn = std::function<SendResult(PacketClass, const uint8_t*, size_t)>;

inline size_t PacketClassIndex(PacketClass packetClass)
{
	return static_cast<size_t>(packetClass);
}

} // namespace mediasoup::plainclient
