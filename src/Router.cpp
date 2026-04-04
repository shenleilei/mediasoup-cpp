#include "Router.h"
#include "Transport.h"
#include "WebRtcTransport.h"
#include "PipeTransport.h"
#include "PlainTransport.h"
#include "Producer.h"
#include "Consumer.h"
#include "Utils.h"
#include "router_generated.h"
#include "worker_generated.h"
#include "request_generated.h"
#include "transport_generated.h"
#include "webRtcTransport_generated.h"
#include "plainTransport_generated.h"
#include "sctpParameters_generated.h"

namespace mediasoup {

Router::Router(const std::string& id, Channel* channel,
	const std::vector<json>& mediaCodecs)
	: id_(id), channel_(channel), logger_(Logger::Get("Router"))
{
	if (!mediaCodecs.empty()) {
		std::vector<RtpCodecCapability> codecs;
		for (auto& mc : mediaCodecs)
			codecs.push_back(mc.get<RtpCodecCapability>());
		rtpCapabilities_ = ortc::generateRouterRtpCapabilities(codecs);
	}
}

std::shared_ptr<WebRtcTransport> Router::createWebRtcTransport(
	const WebRtcTransportOptions& options)
{
	if (closed_) throw std::runtime_error("Router closed");

	std::string transportId = utils::generateUUIDv4();
	auto& builder = channel_->bufferBuilder();

	std::vector<flatbuffers::Offset<FBS::Transport::ListenInfo>> fbListenInfos;
	for (auto& li : options.listenInfos) {
		std::string ip = li.value("ip", "0.0.0.0");
		std::string announcedAddress = li.value("announcedAddress", "");
		std::string protocol = li.value("protocol", "udp");
		uint16_t port = li.value("port", 0);

		auto portRange = FBS::Transport::CreatePortRange(builder, uint16_t(0), uint16_t(0));
		auto flags = FBS::Transport::CreateSocketFlags(builder, false, false);

		fbListenInfos.push_back(FBS::Transport::CreateListenInfo(
			builder,
			protocol == "tcp" ? FBS::Transport::Protocol::TCP : FBS::Transport::Protocol::UDP,
			builder.CreateString(ip),
			announcedAddress.empty() ? 0 : builder.CreateString(announcedAddress),
			port, portRange, flags, 0, 0));
	}

	auto listenIndividualOff = FBS::WebRtcTransport::CreateListenIndividual(
		builder, builder.CreateVector(fbListenInfos));

	auto numSctpStreams = FBS::SctpParameters::CreateNumSctpStreams(builder, 1024, 1024);
	auto baseOptions = FBS::Transport::CreateOptions(
		builder, false, flatbuffers::Optional<uint32_t>(),
		flatbuffers::Optional<uint32_t>(options.initialAvailableOutgoingBitrate),
		options.enableSctp, numSctpStreams, 262144, 262144, true);

	auto webRtcOptions = FBS::WebRtcTransport::CreateWebRtcTransportOptions(
		builder, baseOptions,
		FBS::WebRtcTransport::Listen::ListenIndividual, listenIndividualOff.Union(),
		options.enableUdp, options.enableTcp,
		options.preferUdp, options.preferTcp,
		options.iceConsentTimeout);

	auto transportIdOff = builder.CreateString(transportId);
	auto reqOff = FBS::Router::CreateCreateWebRtcTransportRequest(
		builder, transportIdOff, webRtcOptions);

	auto future = channel_->request(
		FBS::Request::Method::ROUTER_CREATE_WEBRTCTRANSPORT,
		FBS::Request::Body::Router_CreateWebRtcTransportRequest,
		reqOff.Union(), id_);

	auto owned = future.get();
	auto* response = owned.response();

	IceParameters iceParams;
	std::vector<IceCandidate> iceCandidates;
	DtlsParameters dtlsParams;

	if (response && response->body_type() == FBS::Response::Body::WebRtcTransport_DumpResponse) {
		auto dump = response->body_as_WebRtcTransport_DumpResponse();
		if (dump) {
			if (dump->ice_parameters()) {
				iceParams.usernameFragment = dump->ice_parameters()->username_fragment()->str();
				iceParams.password = dump->ice_parameters()->password()->str();
				iceParams.iceLite = dump->ice_parameters()->ice_lite();
			}
			if (dump->ice_candidates()) {
				for (size_t i = 0; i < dump->ice_candidates()->size(); i++) {
					auto* ic = dump->ice_candidates()->Get(i);
					IceCandidate cand;
					cand.foundation = ic->foundation()->str();
					cand.priority = ic->priority();
					cand.address = ic->address()->str();
					cand.protocol = (ic->protocol() == FBS::Transport::Protocol::UDP) ? "udp" : "tcp";
					cand.port = ic->port();
					cand.type = "host";
					iceCandidates.push_back(cand);
				}
			}
			if (dump->dtls_parameters()) {
				for (size_t i = 0; i < dump->dtls_parameters()->fingerprints()->size(); i++) {
					auto* fp = dump->dtls_parameters()->fingerprints()->Get(i);
					DtlsFingerprint dfp;
					switch (fp->algorithm()) {
						case FBS::WebRtcTransport::FingerprintAlgorithm::SHA1:   dfp.algorithm = "sha-1"; break;
						case FBS::WebRtcTransport::FingerprintAlgorithm::SHA224: dfp.algorithm = "sha-224"; break;
						case FBS::WebRtcTransport::FingerprintAlgorithm::SHA256: dfp.algorithm = "sha-256"; break;
						case FBS::WebRtcTransport::FingerprintAlgorithm::SHA384: dfp.algorithm = "sha-384"; break;
						case FBS::WebRtcTransport::FingerprintAlgorithm::SHA512: dfp.algorithm = "sha-512"; break;
					}
					dfp.value = fp->value()->str();
					dtlsParams.fingerprints.push_back(dfp);
				}
				dtlsParams.role = "auto";
			}
		}
	}

	auto transport = std::make_shared<WebRtcTransport>(
		transportId, channel_, id_, iceParams, iceCandidates, dtlsParams);

	transports_[transportId] = transport;

	transport->emitter().on("@close", [this, transportId](auto&) {
		transports_.erase(transportId);
	});

	channel_->emitter().on(transportId, [transport](const std::vector<std::any>& args) {
		if (!args.empty()) {
			auto event = std::any_cast<FBS::Notification::Event>(args[0]);
			transport->handleNotification(event, nullptr);
		}
	});

	MS_DEBUG(logger_, "WebRtcTransport created [id:{}]", transportId);
	return transport;
}

std::shared_ptr<PlainTransport> Router::createPlainTransport(
	const PlainTransportOptions& options)
{
	if (closed_) throw std::runtime_error("Router closed");

	std::string transportId = utils::generateUUIDv4();
	auto& builder = channel_->bufferBuilder();

	// Use first listenInfo
	auto& li = options.listenInfos[0];
	std::string ip = li.value("ip", "127.0.0.1");
	std::string announcedAddress = li.value("announcedAddress", "");
	uint16_t port = li.value("port", 0);

	auto portRange = FBS::Transport::CreatePortRange(builder, uint16_t(0), uint16_t(0));
	auto flags = FBS::Transport::CreateSocketFlags(builder, false, false);
	auto listenInfo = FBS::Transport::CreateListenInfo(
		builder, FBS::Transport::Protocol::UDP,
		builder.CreateString(ip),
		announcedAddress.empty() ? 0 : builder.CreateString(announcedAddress),
		port, portRange, flags, 0, 0);

	auto numSctpStreams = FBS::SctpParameters::CreateNumSctpStreams(builder, 1024, 1024);
	auto baseOptions = FBS::Transport::CreateOptions(
		builder, false, flatbuffers::Optional<uint32_t>(),
		flatbuffers::Optional<uint32_t>(), false, numSctpStreams, 262144, 262144, true);

	auto plainOptions = FBS::PlainTransport::CreatePlainTransportOptions(
		builder, baseOptions, listenInfo, 0,
		options.rtcpMux, options.comedia, false);

	auto transportIdOff = builder.CreateString(transportId);
	auto reqOff = FBS::Router::CreateCreatePlainTransportRequest(
		builder, transportIdOff, plainOptions);

	auto future = channel_->request(
		FBS::Request::Method::ROUTER_CREATE_PLAINTRANSPORT,
		FBS::Request::Body::Router_CreatePlainTransportRequest,
		reqOff.Union(), id_);

	auto owned = future.get();
	auto* response = owned.response();

	TransportTuple tuple;
	if (response && response->body_type() == FBS::Response::Body::PlainTransport_DumpResponse) {
		auto dump = response->body_as_PlainTransport_DumpResponse();
		if (dump && dump->tuple()) {
			tuple.localAddress = dump->tuple()->local_address()->str();
			tuple.localPort = dump->tuple()->local_port();
		}
	}

	auto transport = std::make_shared<PlainTransport>(
		transportId, channel_, id_, tuple, false);

	transports_[transportId] = transport;

	transport->emitter().on("@close", [this, transportId](auto&) {
		transports_.erase(transportId);
	});

	MS_DEBUG(logger_, "PlainTransport created [id:{} {}:{}]",
		transportId, tuple.localAddress, tuple.localPort);
	return transport;
}

std::shared_ptr<Producer> Router::getProducerById(const std::string& id) const {
	auto it = producers_.find(id);
	if (it != producers_.end()) return it->second;
	for (auto& [tid, transport] : transports_) {
		auto& prods = transport->producers();
		auto pit = prods.find(id);
		if (pit != prods.end()) return pit->second;
	}
	return nullptr;
}

void Router::addProducer(std::shared_ptr<Producer> producer) {
	producers_[producer->id()] = producer;
}

void Router::removeProducer(const std::string& id) {
	producers_.erase(id);
}

void Router::close() {
	if (closed_) return;
	closed_ = true;

	MS_DEBUG(logger_, "close() [id:{}]", id_);

	for (auto& [id, transport] : transports_)
		transport->routerClosed();
	transports_.clear();
	producers_.clear();

	auto& builder = channel_->bufferBuilder();
	auto idOff = builder.CreateString(id_);
	auto reqOff = FBS::Worker::CreateCloseRouterRequest(builder, idOff);

	try {
		channel_->request(FBS::Request::Method::WORKER_CLOSE_ROUTER,
			FBS::Request::Body::Worker_CloseRouterRequest,
			reqOff.Union()).get();
	} catch (...) {}

	emitter_.emit("@close");
}

void Router::workerClosed() {
	if (closed_) return;
	closed_ = true;

	for (auto& [id, transport] : transports_)
		transport->routerClosed();
	transports_.clear();
	producers_.clear();

	emitter_.emit("workerclose");
}

json Router::toJson() const {
	return {{"id", id_}, {"closed", closed_}};
}

} // namespace mediasoup
