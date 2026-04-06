#pragma once
#include "Transport.h"
#include "WebRtcTransport.h"
#include "plainTransport_generated.h"
#include "request_generated.h"
#include <nlohmann/json.hpp>

namespace mediasoup {

class PlainTransport : public Transport {
public:
	PlainTransport(const std::string& id, Channel* channel, const std::string& routerId,
		const TransportTuple& tuple, bool rtcpMux)
		: Transport(id, channel, routerId), tuple_(tuple), rtcpMux_(rtcpMux) {}

	const TransportTuple& tuple() const { return tuple_; }

	json connect(const std::string& ip, uint16_t port) {
		if (closed_) throw std::runtime_error("Transport closed");

		auto& builder = channel_->bufferBuilder();
		auto reqOff = FBS::PlainTransport::CreateConnectRequest(
			builder, builder.CreateString(ip),
			flatbuffers::Optional<uint16_t>(port),
			flatbuffers::Optional<uint16_t>(), 0);

		auto future = channel_->request(
			FBS::Request::Method::PLAINTRANSPORT_CONNECT,
			FBS::Request::Body::PlainTransport_ConnectRequest,
			reqOff.Union(), id_);

		auto owned = future.get();
		auto* response = owned.response();

		if (response && response->body_type() == FBS::Response::Body::PlainTransport_ConnectResponse) {
			auto cr = response->body_as_PlainTransport_ConnectResponse();
			if (cr && cr->tuple()) {
				tuple_.localAddress = cr->tuple()->local_address()->str();
				tuple_.localPort = cr->tuple()->local_port();
				if (cr->tuple()->remote_ip())
					tuple_.remoteIp = cr->tuple()->remote_ip()->str();
				tuple_.remotePort = cr->tuple()->remote_port();
			}
		}

		return {{"connected", true}};
	}

	json toJson() const {
		return {{"id", id_}, {"tuple", tuple_}};
	}

private:
	TransportTuple tuple_;
	bool rtcpMux_;
};

} // namespace mediasoup
