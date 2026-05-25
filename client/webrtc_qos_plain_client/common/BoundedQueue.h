#pragma once

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>

namespace webrtc_qos_plain {

template <typename T>
class BoundedQueue {
public:
	explicit BoundedQueue(size_t capacity)
		: capacity_(capacity == 0 ? 1 : capacity)
	{
	}

	BoundedQueue(const BoundedQueue&) = delete;
	BoundedQueue& operator=(const BoundedQueue&) = delete;

	bool PushDropOldest(T value)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (closed_) return false;
			if (items_.size() >= capacity_) {
				items_.pop_front();
				++dropped_;
			}
			items_.push_back(std::move(value));
			if (items_.size() > maxDepth_) maxDepth_ = items_.size();
			++pushed_;
		}
		cv_.notify_one();
		return true;
	}

	bool Pop(T* out)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cv_.wait(lock, [&] { return closed_ || !items_.empty(); });
		if (items_.empty()) return false;
		*out = std::move(items_.front());
		items_.pop_front();
		++popped_;
		return true;
	}

	template <typename Rep, typename Period>
	bool PopFor(T* out, const std::chrono::duration<Rep, Period>& timeout)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cv_.wait_for(lock, timeout, [&] { return closed_ || !items_.empty(); });
		if (items_.empty()) return false;
		*out = std::move(items_.front());
		items_.pop_front();
		++popped_;
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

	void Close()
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			closed_ = true;
		}
		cv_.notify_all();
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

	size_t pushed() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return pushed_;
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

	size_t maxDepth() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return maxDepth_;
	}

private:
	const size_t capacity_;
	mutable std::mutex mutex_;
	std::condition_variable cv_;
	std::deque<T> items_;
	bool closed_{false};
	size_t pushed_{0};
	size_t popped_{0};
	size_t dropped_{0};
	size_t maxDepth_{0};
};

} // namespace webrtc_qos_plain
