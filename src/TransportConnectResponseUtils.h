#pragma once

#include "WebRtcTransport.h"
#include "pipeTransport_generated.h"
#include "plainTransport_generated.h"
#include "response_generated.h"
#include "transport_generated.h"
#include "webRtcTransport_generated.h"

#include <stdexcept>
#include <string>

namespace mediasoup::transportconnect {

inline const FBS::Response::Response& RequireConnectResponse(
	const Channel::OwnedResponse& owned,
	const char* transportName,
	FBS::Response::Body expectedBody)
{
	const auto* response = owned.response();
	if (!response) {
		throw std::runtime_error(std::string(transportName) + " connect response missing worker response body");
	}
	if (response->body_type() != expectedBody) {
		throw std::runtime_error(
			std::string(transportName)
			+ " connect response body mismatch: expected "
			+ FBS::Response::EnumNameBody(expectedBody)
			+ ", got "
			+ FBS::Response::EnumNameBody(response->body_type()));
	}
	return *response;
}

inline std::string ProtocolToString(FBS::Transport::Protocol protocol)
{
	switch (protocol) {
		case FBS::Transport::Protocol::UDP:
			return "udp";
		case FBS::Transport::Protocol::TCP:
			return "tcp";
	}

	throw std::runtime_error("transport connect response contained unknown protocol");
}

inline void ApplyValidatedTuple(
	const FBS::Transport::Tuple* tuple,
	TransportTuple* target,
	const char* transportName)
{
	if (!tuple || !tuple->local_address()) {
		throw std::runtime_error(std::string(transportName) + " connect response missing transport tuple");
	}

	target->localAddress = tuple->local_address()->str();
	target->localPort = tuple->local_port();
	target->remoteIp = tuple->remote_ip() ? tuple->remote_ip()->str() : "";
	target->remotePort = tuple->remote_port();
	target->protocol = ProtocolToString(tuple->protocol());
}

inline std::string ParseValidatedDtlsLocalRole(const Channel::OwnedResponse& owned)
{
	const auto& response = RequireConnectResponse(
		owned,
		"WebRtcTransport",
		FBS::Response::Body::WebRtcTransport_ConnectResponse);
	const auto* connectResponse = response.body_as_WebRtcTransport_ConnectResponse();
	if (!connectResponse) {
		throw std::runtime_error("WebRtcTransport connect response missing parsed body");
	}

	switch (connectResponse->dtls_local_role()) {
		case FBS::WebRtcTransport::DtlsRole::CLIENT:
			return "client";
		case FBS::WebRtcTransport::DtlsRole::SERVER:
			return "server";
		case FBS::WebRtcTransport::DtlsRole::AUTO:
			break;
	}

	throw std::runtime_error("WebRtcTransport connect response missing concrete dtlsLocalRole");
}

inline void ApplyValidatedPlainConnectResponse(
	const Channel::OwnedResponse& owned,
	TransportTuple* tuple)
{
	const auto& response = RequireConnectResponse(
		owned,
		"PlainTransport",
		FBS::Response::Body::PlainTransport_ConnectResponse);
	const auto* connectResponse = response.body_as_PlainTransport_ConnectResponse();
	if (!connectResponse) {
		throw std::runtime_error("PlainTransport connect response missing parsed body");
	}

	ApplyValidatedTuple(connectResponse->tuple(), tuple, "PlainTransport");
}

inline void ApplyValidatedPipeConnectResponse(
	const Channel::OwnedResponse& owned,
	TransportTuple* tuple)
{
	const auto& response = RequireConnectResponse(
		owned,
		"PipeTransport",
		FBS::Response::Body::PipeTransport_ConnectResponse);
	const auto* connectResponse = response.body_as_PipeTransport_ConnectResponse();
	if (!connectResponse) {
		throw std::runtime_error("PipeTransport connect response missing parsed body");
	}

	ApplyValidatedTuple(connectResponse->tuple(), tuple, "PipeTransport");
}

} // namespace mediasoup::transportconnect
