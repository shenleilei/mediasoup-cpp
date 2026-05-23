#pragma once

#include <cstdio>
#include <string>

#include "webrtc_qos/session_config.h"
#include "webrtc_qos/status.h"

namespace webrtc_qos_plain {

class AnnexBSink {
public:
	AnnexBSink() = default;
	~AnnexBSink();

	AnnexBSink(const AnnexBSink&) = delete;
	AnnexBSink& operator=(const AnnexBSink&) = delete;

	bool OpenFile(const std::string& path, std::string* error);
	void EnableNull();
	webrtc_qos::Status Write(const webrtc_qos::AnnexBAccessUnitView& accessUnit);
	uint64_t writtenAccessUnits() const { return writtenAccessUnits_; }

private:
	FILE* file_{nullptr};
	bool null_{false};
	uint64_t writtenAccessUnits_{0};
};

} // namespace webrtc_qos_plain
