#pragma once

#include <cstdint>
#include <mutex>
#include <utility>

namespace webrtc_qos_plain {

template <typename T>
class LatestValue {
public:
	LatestValue() = default;
	explicit LatestValue(T initial)
		: value_(std::move(initial)),
		  hasValue_(true),
		  version_(1)
	{
	}

	void Store(T value)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		value_ = std::move(value);
		hasValue_ = true;
		++version_;
	}

	bool Load(T* out, uint64_t* version = nullptr) const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!hasValue_) return false;
		*out = value_;
		if (version) *version = version_;
		return true;
	}

	bool LoadIfNewer(uint64_t lastSeenVersion, T* out, uint64_t* version = nullptr) const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!hasValue_ || version_ <= lastSeenVersion) return false;
		*out = value_;
		if (version) *version = version_;
		return true;
	}

	uint64_t version() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return version_;
	}

	bool hasValue() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return hasValue_;
	}

private:
	mutable std::mutex mutex_;
	T value_{};
	bool hasValue_{false};
	uint64_t version_{0};
};

} // namespace webrtc_qos_plain
