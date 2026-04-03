#pragma once
#include "Transport.h"
#include "WebRtcTransport.h"
#include <nlohmann/json.hpp>

namespace mediasoup {

class PipeTransport : public Transport {
public:
	PipeTransport(const std::string& id, Channel* channel, const std::string& routerId,
		const TransportTuple& tuple, bool rtx, const json& srtpParameters = {})
		: Transport(id, channel, routerId)
		, tuple_(tuple), rtx_(rtx), srtpParameters_(srtpParameters) {}

	const TransportTuple& tuple() const { return tuple_; }
	bool rtx() const { return rtx_; }

	json connect(const std::string& ip, uint16_t port, const json& srtpParameters = {});

	json toJson() const {
		return {{"id", id_}, {"tuple", tuple_}};
	}

private:
	TransportTuple tuple_;
	bool rtx_;
	json srtpParameters_;
};

} // namespace mediasoup
