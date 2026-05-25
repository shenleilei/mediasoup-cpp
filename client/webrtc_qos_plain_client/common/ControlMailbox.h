#pragma once

#include <cerrno>
#include <cstring>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <utility>

#include <sys/eventfd.h>
#include <unistd.h>

namespace webrtc_qos_plain {

template <typename T>
class ControlMailbox {
public:
	explicit ControlMailbox(size_t capacity)
		: capacity_(capacity == 0 ? 1 : capacity),
		  wakeFd_(::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK))
	{
	}

	~ControlMailbox()
	{
		if (wakeFd_ >= 0) ::close(wakeFd_);
	}

	ControlMailbox(const ControlMailbox&) = delete;
	ControlMailbox& operator=(const ControlMailbox&) = delete;

	bool valid() const { return wakeFd_ >= 0; }
	int wakeFd() const { return wakeFd_; }

	bool Post(T command)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (closed_) return false;
			if (items_.size() >= capacity_) {
				++dropped_;
				return false;
			}
			items_.push_back(std::move(command));
			++posted_;
		}
		Notify();
		return true;
	}

	bool TryPop(T* out)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (items_.empty()) return false;
		*out = std::move(items_.front());
		items_.pop_front();
		++popped_;
		return true;
	}

	void DrainWakeSignal()
	{
		if (wakeFd_ < 0) return;
		uint64_t value = 0;
		while (::read(wakeFd_, &value, sizeof(value)) == static_cast<ssize_t>(sizeof(value))) {
		}
	}

	void Close()
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (closed_) return;
			closed_ = true;
		}
		Notify();
	}

	bool closed() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return closed_;
	}

	size_t depth() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return items_.size();
	}

	size_t capacity() const { return capacity_; }

	size_t posted() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return posted_;
	}

	size_t popped() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return popped_;
	}

	size_t dropped() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return dropped_;
	}

	std::string lastNotifyError() const
	{
		std::lock_guard<std::mutex> lock(errorMutex_);
		return lastNotifyError_;
	}

private:
	void Notify()
	{
		if (wakeFd_ < 0) return;
		const uint64_t one = 1;
		if (::write(wakeFd_, &one, sizeof(one)) != static_cast<ssize_t>(sizeof(one)) && errno != EAGAIN) {
			const int savedErrno = errno;
			std::lock_guard<std::mutex> lock(errorMutex_);
			lastNotifyError_ = std::string("eventfd write failed: ") + std::strerror(savedErrno);
		}
	}

	const size_t capacity_;
	const int wakeFd_{-1};
	mutable std::mutex mutex_;
	std::deque<T> items_;
	bool closed_{false};
	size_t posted_{0};
	size_t popped_{0};
	size_t dropped_{0};
	mutable std::mutex errorMutex_;
	std::string lastNotifyError_;
};

} // namespace webrtc_qos_plain
