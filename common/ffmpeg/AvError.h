#pragma once

#include <string>

namespace mediasoup::ffmpeg {

std::string ErrorToString(int errnum);
void CheckError(int errnum, const std::string& action);

} // namespace mediasoup::ffmpeg
