#pragma once

#include <cstdint>
#include <vector>

#include "push/H264AnnexBSource.h"
#include "webrtc_qos/session_config.h"
#include "webrtc_qos/status.h"

namespace webrtc_qos_plain {

struct EncodedAccessUnit {
	std::vector<uint8_t> bytes;
	int64_t captureTimeUs{0};
	bool keyframe{false};
	webrtc_qos::TransportIds ids;
};

inline webrtc_qos::Status CopyEncodedAccessUnit(
	const AnnexBAccessUnit& source,
	int64_t captureTimeUs,
	const webrtc_qos::TransportIds& ids,
	EncodedAccessUnit* out)
{
	if (!out) {
		return webrtc_qos::Status::Error(
			webrtc_qos::StatusCode::kInvalidArgument,
			"encoded access unit output is null");
	}
	if (source.bytes.empty()) {
		return webrtc_qos::Status::Error(
			webrtc_qos::StatusCode::kInvalidArgument,
			"empty encoded access unit");
	}
	out->bytes = source.bytes;
	out->captureTimeUs = captureTimeUs;
	out->keyframe = source.keyframe;
	out->ids = ids;
	return webrtc_qos::Status::Ok();
}

inline webrtc_qos::AnnexBAccessUnitView ToAnnexBAccessUnitView(const EncodedAccessUnit& item)
{
	webrtc_qos::AnnexBAccessUnitView view;
	view.bytes = item.bytes.data();
	view.size = item.bytes.size();
	view.capture_time_us = item.captureTimeUs;
	view.keyframe = item.keyframe;
	view.ids = item.ids;
	return view;
}

} // namespace webrtc_qos_plain
