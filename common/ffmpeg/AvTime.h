#pragma once

extern "C" {
#include <libavutil/mathematics.h>
}

#include <cstdint>

namespace mediasoup::ffmpeg {

inline int64_t RescaleQ(int64_t value, AVRational from, AVRational to)
{
	return av_rescale_q(value, from, to);
}

inline int64_t ClampNonNegativePts(int64_t value)
{
	return value < 0 ? 0 : value;
}

} // namespace mediasoup::ffmpeg
