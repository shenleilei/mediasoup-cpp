#include "Channel.h"
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <algorithm>

namespace mediasoup {

static constexpr uint32_t MESSAGE_MAX_LEN = 4194308;
static constexpr size_t RECV_BUFFER_MAX_LEN = 16 * 1024 * 1024;

Channel::Channel(int producerFd, int consumerFd, int pid)
	: producerFd_(producerFd), consumerFd_(consumerFd), pid_(pid)
	, logger_(Logger::Get("Channel"))
{
	readThread_ = std::thread(&Channel::readLoop, this);
}

Channel::~Channel() {
	close();
}

void Channel::close() {
	if (closed_.exchange(true)) return;

	MS_DEBUG(logger_, "close()");

	// Reject all pending requests
	{
		std::lock_guard<std::mutex> lock(sentsMutex_);
		for (auto& [id, sent] : sents_) {
			sent->promise.set_exception(
				std::make_exception_ptr(std::runtime_error("Channel closed")));
		}
		sents_.clear();
	}

	// Close fds to unblock the read thread (read() will return ≤0)
	if (producerFd_ >= 0) { ::close(producerFd_); producerFd_ = -1; }
	if (consumerFd_ >= 0) { ::close(consumerFd_); consumerFd_ = -1; }

	if (readThread_.joinable()) {
		if (readThread_.get_id() != std::this_thread::get_id())
			readThread_.join();
	}
}

void Channel::sendBytes(const uint8_t* data, size_t len) {
	if (closed_) return;
	size_t written = 0;
	while (written < len) {
		ssize_t n = ::write(producerFd_, data + written, len - written);
		if (n <= 0) {
			MS_ERROR(logger_, "write failed: {}", strerror(errno));
			return;
		}
		written += n;
	}
}

void Channel::notify(
	FBS::Notification::Event event,
	FBS::Notification::Body bodyType,
	flatbuffers::Offset<void> bodyOffset,
	const std::string& handlerId)
{
	if (closed_) throw std::runtime_error("Channel closed");

	std::vector<uint8_t> sendBuf;
	{
		std::lock_guard<std::mutex> lock(builderMutex_);

		auto handlerIdOff = builder_.CreateString(handlerId);
		auto notifOff = FBS::Notification::CreateNotification(
			builder_, handlerIdOff, event, bodyType, bodyOffset);
		auto msgOff = FBS::Message::CreateMessage(
			builder_, FBS::Message::Body::Notification, notifOff.Union());
		builder_.FinishSizePrefixed(msgOff);

		auto buf = builder_.GetBufferPointer();
		auto size = builder_.GetSize();

		if (size > MESSAGE_MAX_LEN) {
			MS_ERROR(logger_, "notification too big");
			builder_.Clear();
			return;
		}

		sendBuf.assign(buf, buf + size);
		builder_.Clear();
	}

	sendBytes(sendBuf.data(), sendBuf.size());
}

std::future<Channel::OwnedResponse> Channel::request(
	FBS::Request::Method method,
	FBS::Request::Body bodyType,
	flatbuffers::Offset<void> bodyOffset,
	const std::string& handlerId)
{
	if (closed_) throw std::runtime_error("Channel closed");

	auto sent = std::make_shared<PendingSent>();
	std::vector<uint8_t> sendBuf;

	{
		std::lock_guard<std::mutex> lock(builderMutex_);

		if (nextId_ < 4294967295u) ++nextId_; else nextId_ = 1;
		uint32_t id = nextId_;

		auto handlerIdOff = builder_.CreateString(handlerId);
		auto reqOff = FBS::Request::CreateRequest(
			builder_, id, method, handlerIdOff, bodyType, bodyOffset);
		auto msgOff = FBS::Message::CreateMessage(
			builder_, FBS::Message::Body::Request, reqOff.Union());
		builder_.FinishSizePrefixed(msgOff);

		auto buf = builder_.GetBufferPointer();
		auto size = builder_.GetSize();

		if (size > MESSAGE_MAX_LEN) {
			builder_.Clear();
			throw std::runtime_error("request too big");
		}

		sendBuf.assign(buf, buf + size);
		builder_.Clear();

		sent->id = id;
		sent->method = FBS::Request::EnumNameMethod(method);

		{
			std::lock_guard<std::mutex> slock(sentsMutex_);
			sents_[id] = sent;
		}
	}

	auto future = sent->promise.get_future();
	sendBytes(sendBuf.data(), sendBuf.size());

	return future;
}

void Channel::readLoop() {
	std::vector<uint8_t> recvBuf;
	uint8_t tmp[65536];

	while (!closed_) {
		ssize_t n = ::read(consumerFd_, tmp, sizeof(tmp));
		if (n <= 0) {
			if (!closed_) MS_DEBUG(logger_, "consumer fd closed");
			break;
		}

		recvBuf.insert(recvBuf.end(), tmp, tmp + n);
		if (recvBuf.size() > RECV_BUFFER_MAX_LEN) {
			MS_WARN(logger_,
				"recv buffer exceeded limit [pid:{} size:{} limit:{}], closing channel",
				pid_, recvBuf.size(), RECV_BUFFER_MAX_LEN);
			close();
			break;
		}

		size_t offset = 0;
		while (offset + 4 <= recvBuf.size()) {
			uint32_t msgLen;
			std::memcpy(&msgLen, recvBuf.data() + offset, 4);
			if (msgLen == 0 || msgLen > MESSAGE_MAX_LEN) {
				MS_WARN(logger_,
					"invalid frame length [pid:{} msgLen:{} max:{}], closing channel",
					pid_, msgLen, MESSAGE_MAX_LEN);
				close();
				offset = recvBuf.size();
				break;
			}
			if (offset + 4 + msgLen > recvBuf.size()) break;

			if (!processMessage(recvBuf.data() + offset + 4, msgLen)) {
				MS_WARN(logger_, "invalid FlatBuffers message [pid:{} len:{}], closing channel", pid_, msgLen);
				close();
				offset = recvBuf.size();
				break;
			}
			offset += 4 + msgLen;
		}

		if (offset > 0) {
			recvBuf.erase(recvBuf.begin(), recvBuf.begin() + offset);
		}
	}
}

bool Channel::processMessage(const uint8_t* data, size_t len) {
	flatbuffers::Verifier verifier(data, len);
	if (!FBS::Message::VerifyMessageBuffer(verifier)) return false;

	auto msg = FBS::Message::GetMessage(data);
	if (!msg) return false;

	switch (msg->data_type()) {
		case FBS::Message::Body::Response: {
			auto response = msg->data_as_Response();
			if (!response) return true;
			uint32_t id = response->id();

			std::shared_ptr<PendingSent> sent;
			{
				std::lock_guard<std::mutex> lock(sentsMutex_);
				auto it = sents_.find(id);
				if (it == sents_.end()) {
					MS_ERROR(logger_, "response does not match any request [id:{}]", id);
					return true;
				}
				sent = it->second;
				sents_.erase(it);
			}

			if (response->accepted()) {
				// Copy the raw message data into OwnedResponse by value
				OwnedResponse ownedResp;
				ownedResp.data.assign(data, data + len);
				sent->promise.set_value(std::move(ownedResp));
			} else {
				std::string reason = response->reason() ? response->reason()->str() : "unknown error";
				sent->promise.set_exception(
					std::make_exception_ptr(std::runtime_error(reason)));
			}
			break;
		}
		case FBS::Message::Body::Notification:
			processNotification(data, len, msg->data_as_Notification());
			break;
		case FBS::Message::Body::Log:
			processLog(msg->data_as_Log());
			break;
		default:
			MS_WARN(logger_, "unexpected message type");
			break;
	}
	return true;
}

void Channel::processNotification(const uint8_t* data, size_t len,
	const FBS::Notification::Notification* notification)
{
	if (!notification) return;
	std::string handlerId = notification->handler_id() ? notification->handler_id()->str() : "";
	auto event = notification->event();

	auto owned = std::make_shared<OwnedNotification>();
	owned->data.assign(data, data + len);
	owned->event = event;

	emitter_.emit(handlerId, {std::any(event), std::any(owned)});
}

void Channel::processLog(const FBS::Log::Log* log) {
	if (!log || !log->data()) return;
	std::string logData = log->data()->str();
	if (logData.empty()) return;

	auto workerLogger = Logger::Get("Worker");
	char flag = logData[0];
	std::string msg = logData.substr(1);

	switch (flag) {
		case 'D': MS_DEBUG(workerLogger, "[pid:{}] {}", pid_, msg); break;
		case 'W': MS_WARN(workerLogger, "[pid:{}] {}", pid_, msg); break;
		case 'E': MS_ERROR(workerLogger, "[pid:{}] {}", pid_, msg); break;
		default: spdlog::info(msg); break;
	}
}

} // namespace mediasoup
