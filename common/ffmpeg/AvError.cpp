#include "ffmpeg/AvError.h"

extern "C" {
#include <libavutil/error.h>
}

#include <array>
#include <stdexcept>

namespace mediasoup::ffmpeg {

std::string ErrorToString(int errnum)
{
	std::array<char, AV_ERROR_MAX_STRING_SIZE> buffer{};
	if (av_strerror(errnum, buffer.data(), buffer.size()) == 0)
		return std::string(buffer.data());
	return "unknown ffmpeg error " + std::to_string(errnum);
}

void CheckError(int errnum, const std::string& action)
{
	if (errnum >= 0) return;
	throw std::runtime_error(action + ": " + ErrorToString(errnum));
}

} // namespace mediasoup::ffmpeg
