#include "play/AnnexBSink.h"

#include <cerrno>
#include <cstring>

namespace webrtc_qos_plain {

AnnexBSink::~AnnexBSink()
{
	if (file_) {
		fclose(file_);
		file_ = nullptr;
	}
}

bool AnnexBSink::OpenFile(const std::string& path, std::string* error)
{
	file_ = fopen(path.c_str(), "wb");
	if (!file_) {
		if (error) *error = std::strerror(errno);
		return false;
	}
	null_ = false;
	return true;
}

void AnnexBSink::EnableNull()
{
	null_ = true;
}

webrtc_qos::Status AnnexBSink::Write(const webrtc_qos::AnnexBAccessUnitView& accessUnit)
{
	if (!accessUnit.bytes || accessUnit.size == 0) {
		return webrtc_qos::Status::Error(
			webrtc_qos::StatusCode::kInvalidArgument,
			"empty access unit");
	}
	if (null_) {
		++writtenAccessUnits_;
		return webrtc_qos::Status::Ok();
	}
	if (!file_) {
		return webrtc_qos::Status::Error(
			webrtc_qos::StatusCode::kInternalError,
			"sink file is not open");
	}
	const size_t written = fwrite(accessUnit.bytes, 1, accessUnit.size, file_);
	if (written != accessUnit.size) {
		return webrtc_qos::Status::Error(
			webrtc_qos::StatusCode::kInternalError,
			std::string("fwrite failed: ") + std::strerror(errno));
	}
	++writtenAccessUnits_;
	return webrtc_qos::Status::Ok();
}

} // namespace webrtc_qos_plain
