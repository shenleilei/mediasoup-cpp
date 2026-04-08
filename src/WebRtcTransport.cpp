#include "WebRtcTransport.h"
#include "webRtcTransport_generated.h"
#include "request_generated.h"
#include "transport_generated.h"

namespace mediasoup {

json WebRtcTransport::connect(const DtlsParameters& clientDtlsParams) {
	if (closed_) throw std::runtime_error("Transport closed");

	auto& builder = channel_->bufferBuilder();

	// Build DtlsParameters FlatBuffer
	std::vector<flatbuffers::Offset<FBS::WebRtcTransport::Fingerprint>> fbFingerprints;
	for (auto& fp : clientDtlsParams.fingerprints) {
		auto alg = FBS::WebRtcTransport::FingerprintAlgorithm::SHA256;
		if (fp.algorithm == "sha-1") alg = FBS::WebRtcTransport::FingerprintAlgorithm::SHA1;
		else if (fp.algorithm == "sha-224") alg = FBS::WebRtcTransport::FingerprintAlgorithm::SHA224;
		else if (fp.algorithm == "sha-384") alg = FBS::WebRtcTransport::FingerprintAlgorithm::SHA384;
		else if (fp.algorithm == "sha-512") alg = FBS::WebRtcTransport::FingerprintAlgorithm::SHA512;

		fbFingerprints.push_back(
			FBS::WebRtcTransport::CreateFingerprint(builder, alg, builder.CreateString(fp.value)));
	}

	auto dtlsRole = FBS::WebRtcTransport::DtlsRole::AUTO;
	if (clientDtlsParams.role == "client") dtlsRole = FBS::WebRtcTransport::DtlsRole::CLIENT;
	else if (clientDtlsParams.role == "server") dtlsRole = FBS::WebRtcTransport::DtlsRole::SERVER;

	auto dtlsParamsOff = FBS::WebRtcTransport::CreateDtlsParameters(
		builder, builder.CreateVector(fbFingerprints), dtlsRole);

	auto reqOff = FBS::WebRtcTransport::CreateConnectRequest(builder, dtlsParamsOff);

	channel_->requestWait(
		FBS::Request::Method::WEBRTCTRANSPORT_CONNECT,
		FBS::Request::Body::WebRtcTransport_ConnectRequest,
		reqOff.Union(),
		id_);

	return {{"dtlsLocalRole", "server"}};
}

json WebRtcTransport::restartIce() {
	if (closed_) throw std::runtime_error("Transport closed");

	auto owned = channel_->requestWait(
		FBS::Request::Method::TRANSPORT_RESTART_ICE,
		FBS::Request::Body::NONE, 0, id_);

	auto* response = owned.response();
	if (response && response->body_type() == FBS::Response::Body::Transport_RestartIceResponse) {
		auto* iceResponse = response->body_as_Transport_RestartIceResponse();
		if (iceResponse && iceResponse->username_fragment() && iceResponse->password()) {
			iceParameters_.usernameFragment = iceResponse->username_fragment()->str();
			iceParameters_.password = iceResponse->password()->str();
			iceParameters_.iceLite = iceResponse->ice_lite();
		}
	}

	return {{"iceParameters", iceParameters_}};
}

void WebRtcTransport::handleNotification(
	FBS::Notification::Event event,
	const FBS::Notification::Notification* notification)
{
	switch (event) {
		case FBS::Notification::Event::WEBRTCTRANSPORT_ICE_STATE_CHANGE: {
			if (notification) {
				auto body = notification->body_as_WebRtcTransport_IceStateChangeNotification();
				if (body) iceState_ = FBS::WebRtcTransport::EnumNameIceState(body->ice_state());
			}
			emitter_.emit("icestatechange", {std::any(iceState_)});
			break;
		}
		case FBS::Notification::Event::WEBRTCTRANSPORT_DTLS_STATE_CHANGE: {
			if (notification) {
				auto body = notification->body_as_WebRtcTransport_DtlsStateChangeNotification();
				if (body) dtlsState_ = FBS::WebRtcTransport::EnumNameDtlsState(body->dtls_state());
			}
			emitter_.emit("dtlsstatechange", {std::any(dtlsState_)});
			break;
		}
		case FBS::Notification::Event::WEBRTCTRANSPORT_ICE_SELECTED_TUPLE_CHANGE: {
			emitter_.emit("iceselectedtuplechange");
			break;
		}
		default:
			break;
	}
}

} // namespace mediasoup
