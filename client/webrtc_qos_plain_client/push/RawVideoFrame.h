#pragma once

#include <cstdint>
#include <vector>

namespace webrtc_qos_plain {

struct RawVideoFrame {
	int width{0};
	int height{0};
	int64_t captureTimeUs{0};
	int64_t mediaTimeUs{0};
	uint64_t frameIndex{0};
	std::vector<uint8_t> yuv420p;
};

inline size_t RawVideoFrameSize(int width, int height)
{
	if (width <= 0 || height <= 0) return 0;
	const size_t y = static_cast<size_t>(width) * static_cast<size_t>(height);
	return y + y / 2;
}

} // namespace webrtc_qos_plain
