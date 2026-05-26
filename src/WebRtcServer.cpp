#include "WebRtcServer.h"

#include "request_generated.h"
#include "webRtcServer_generated.h"
#include "worker_generated.h"

namespace mediasoup {

WebRtcServer::WebRtcServer(
	const std::string& id,
	Channel* channel,
	const std::vector<nlohmann::json>& listenInfos)
	: id_(id)
	, channel_(channel)
	, listenInfos_(listenInfos)
	, logger_(Logger::Get("WebRtcServer"))
{
}

uint16_t WebRtcServer::firstListenPort() const
{
	for (const auto& listenInfo : listenInfos_) {
		const int port = listenInfo.value("port", 0);
		if (port > 0 && port <= 65535) {
			return static_cast<uint16_t>(port);
		}
	}
	return 0;
}

nlohmann::json WebRtcServer::dump()
{
	if (closed_) return nlohmann::json::object();

	auto owned = channel_->requestWait(
		FBS::Request::Method::WEBRTCSERVER_DUMP,
		FBS::Request::Body::NONE,
		0,
		id_);
	auto* response = owned.response();

	nlohmann::json result = nlohmann::json::object();
	if (!response || response->body_type() != FBS::Response::Body::WebRtcServer_DumpResponse) {
		return result;
	}

	auto* dump = response->body_as_WebRtcServer_DumpResponse();
	if (!dump) return result;

	result["id"] = dump->id() ? dump->id()->str() : id_;
	result["udpSockets"] = nlohmann::json::array();
	result["tcpServers"] = nlohmann::json::array();
	result["webRtcTransportIds"] = nlohmann::json::array();

	if (dump->udp_sockets()) {
		for (auto* socket : *dump->udp_sockets()) {
			if (!socket) continue;
			result["udpSockets"].push_back({
				{"ip", socket->ip() ? socket->ip()->str() : ""},
				{"port", socket->port()}
			});
		}
	}
	if (dump->tcp_servers()) {
		for (auto* server : *dump->tcp_servers()) {
			if (!server) continue;
			result["tcpServers"].push_back({
				{"ip", server->ip() ? server->ip()->str() : ""},
				{"port", server->port()}
			});
		}
	}
	if (dump->web_rtc_transport_ids()) {
		for (auto* transportId : *dump->web_rtc_transport_ids()) {
			if (transportId) result["webRtcTransportIds"].push_back(transportId->str());
		}
	}

	return result;
}

void WebRtcServer::close()
{
	if (closed_) return;
	closed_ = true;

	try {
		channel_->requestBuild(
			FBS::Request::Method::WORKER_WEBRTCSERVER_CLOSE,
			FBS::Request::Body::Worker_CloseWebRtcServerRequest,
			[this](flatbuffers::FlatBufferBuilder& builder) {
				auto idOff = builder.CreateString(id_);
				auto reqOff = FBS::Worker::CreateCloseWebRtcServerRequest(builder, idOff);
				return reqOff.Union();
			});
	} catch (const std::exception& e) {
		MS_WARN(logger_, "WebRtcServer::close() request failed [id:{}]: {}", id_, e.what());
	} catch (...) {
		MS_WARN(logger_, "WebRtcServer::close() request failed [id:{}]: unknown error", id_);
	}

	emitter_.emitChecked("@close", {std::any(std::string("close"))});
}

void WebRtcServer::workerClosed()
{
	if (closed_) return;
	closed_ = true;
	emitter_.emitChecked("@close", {std::any(std::string("workerclose"))});
}

} // namespace mediasoup
