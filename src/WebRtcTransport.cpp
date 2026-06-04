#include "WebRtcTransport.h"
#include "TransportConnectResponseUtils.h"
#include "webRtcTransport_generated.h"
#include "request_generated.h"
#include "transport_generated.h"
#include <cstring>

namespace mediasoup {

namespace {

const char* NormalizeFbsStateName(const char* state)
{
	if (!state) return "unknown";

	if (std::strcmp(state, "NEW") == 0) return "new";
	if (std::strcmp(state, "CONNECTED") == 0) return "connected";
	if (std::strcmp(state, "COMPLETED") == 0) return "completed";
	if (std::strcmp(state, "DISCONNECTED") == 0) return "disconnected";
	if (std::strcmp(state, "CONNECTING") == 0) return "connecting";
	if (std::strcmp(state, "FAILED") == 0) return "failed";
	if (std::strcmp(state, "CLOSED") == 0) return "closed";

	return state;
}

} // namespace

json WebRtcTransport::connect(const DtlsParameters& clientDtlsParams) {
	if (closed_) throw std::runtime_error("Transport closed");
	auto owned = channel_->requestBuildWait(
		FBS::Request::Method::WEBRTCTRANSPORT_CONNECT,
		FBS::Request::Body::WebRtcTransport_ConnectRequest,
		[clientDtlsParams](flatbuffers::FlatBufferBuilder& builder) {
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
			return reqOff.Union();
		}, id_);

	const std::string dtlsLocalRole =
		transportconnect::ParseValidatedDtlsLocalRole(owned);
	dtlsParameters_.role = dtlsLocalRole;
	return {{"dtlsLocalRole", dtlsLocalRole}};
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
			const auto previousIceState = iceState_;
			if (notification) {
				auto body = notification->body_as_WebRtcTransport_IceStateChangeNotification();
				if (body) iceState_ = NormalizeFbsStateName(FBS::WebRtcTransport::EnumNameIceState(body->ice_state()));
			}
			if (previousIceState != iceState_) {
				if (iceState_ == "disconnected") {
					MS_WARN(logger_, "{} ICE state changed previous={} current={}",
						logPrefix(), previousIceState, iceState_);
				} else {
					MS_INFO(logger_, "{} ICE state changed previous={} current={}",
						logPrefix(), previousIceState, iceState_);
				}
			}
			emitter_.emit("icestatechange", {std::any(iceState_)});
			break;
		}
		case FBS::Notification::Event::WEBRTCTRANSPORT_DTLS_STATE_CHANGE: {
			const auto previousDtlsState = dtlsState_;
			if (notification) {
				auto body = notification->body_as_WebRtcTransport_DtlsStateChangeNotification();
				if (body) dtlsState_ = NormalizeFbsStateName(FBS::WebRtcTransport::EnumNameDtlsState(body->dtls_state()));
			}
			if (previousDtlsState != dtlsState_) {
				if (dtlsState_ == "failed" || dtlsState_ == "closed") {
					MS_WARN(logger_, "{} DTLS state changed previous={} current={}",
						logPrefix(), previousDtlsState, dtlsState_);
				} else {
					MS_INFO(logger_, "{} DTLS state changed previous={} current={}",
						logPrefix(), previousDtlsState, dtlsState_);
				}
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
