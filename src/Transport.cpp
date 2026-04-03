#include "Transport.h"
#include "Producer.h"
#include "Consumer.h"
#include "ortc.h"
#include "Utils.h"
#include "request_generated.h"
#include "transport_generated.h"
#include "rtpParameters_generated.h"
#include "router_generated.h"

namespace mediasoup {

// Helper: build FBS RtpParameters from our types
static flatbuffers::Offset<FBS::RtpParameters::RtpParameters> buildFbsRtpParameters(
	flatbuffers::FlatBufferBuilder& builder, const RtpParameters& params)
{
	// Codecs
	std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpCodecParameters>> fbCodecs;
	for (auto& codec : params.codecs) {
		std::vector<flatbuffers::Offset<FBS::RtpParameters::Parameter>> fbParams;
		for (auto& [k, v] : codec.parameters.items()) {
			flatbuffers::Offset<void> valOff;
			FBS::RtpParameters::Value valType;
			if (v.is_number_integer()) {
				valOff = FBS::RtpParameters::CreateInteger32(builder, v.get<int32_t>()).Union();
				valType = FBS::RtpParameters::Value::Integer32;
			} else if (v.is_number_float()) {
				valOff = FBS::RtpParameters::CreateDouble(builder, v.get<double>()).Union();
				valType = FBS::RtpParameters::Value::Double;
			} else if (v.is_string()) {
				valOff = FBS::RtpParameters::CreateString(builder, builder.CreateString(v.get<std::string>())).Union();
				valType = FBS::RtpParameters::Value::String;
			} else {
				valOff = FBS::RtpParameters::CreateBoolean(builder, v.is_boolean() ? (v.get<bool>() ? 1 : 0) : 0).Union();
				valType = FBS::RtpParameters::Value::Boolean;
			}
			fbParams.push_back(FBS::RtpParameters::CreateParameter(
				builder, builder.CreateString(k), valType, valOff));
		}

		std::vector<flatbuffers::Offset<FBS::RtpParameters::RtcpFeedback>> fbFeedback;
		for (auto& fb : codec.rtcpFeedback) {
			fbFeedback.push_back(FBS::RtpParameters::CreateRtcpFeedback(
				builder, builder.CreateString(fb.type),
				fb.parameter.empty() ? 0 : builder.CreateString(fb.parameter)));
		}

		fbCodecs.push_back(FBS::RtpParameters::CreateRtpCodecParameters(
			builder, builder.CreateString(codec.mimeType), codec.payloadType,
			codec.clockRate, codec.channels > 0 ? codec.channels : flatbuffers::Optional<uint8_t>(),
			builder.CreateVector(fbParams), builder.CreateVector(fbFeedback)));
	}

	// Header extensions
	std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpHeaderExtensionParameters>> fbExts;
	for (auto& ext : params.headerExtensions) {
		auto uri = FBS::RtpParameters::RtpHeaderExtensionUri::Mid; // default
		if (ext.uri.find("abs-send-time") != std::string::npos)
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::AbsSendTime;
		else if (ext.uri.find("transport-wide-cc") != std::string::npos)
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::TransportWideCcDraft01;
		else if (ext.uri.find("ssrc-audio-level") != std::string::npos)
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::AudioLevel;
		else if (ext.uri.find("video-orientation") != std::string::npos)
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::VideoOrientation;
		else if (ext.uri.find("rtp-stream-id") != std::string::npos)
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::RtpStreamId;
		else if (ext.uri.find("repaired-rtp-stream-id") != std::string::npos)
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::RepairRtpStreamId;
		else if (ext.uri.find("sdes:mid") != std::string::npos)
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::Mid;
		else if (ext.uri.find("toffset") != std::string::npos)
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::TimeOffset;
		else if (ext.uri.find("abs-capture-time") != std::string::npos)
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::AbsCaptureTime;
		else if (ext.uri.find("playout-delay") != std::string::npos)
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::PlayoutDelay;
		else if (ext.uri.find("dependency-descriptor") != std::string::npos)
			uri = FBS::RtpParameters::RtpHeaderExtensionUri::Mid; // fallback, not in 3.14

		fbExts.push_back(FBS::RtpParameters::CreateRtpHeaderExtensionParameters(
			builder, uri, ext.id, ext.encrypt, 0));
	}

	// Encodings
	std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters>> fbEncodings;
	for (auto& enc : params.encodings) {
		flatbuffers::Offset<FBS::RtpParameters::Rtx> rtxOff = 0;
		if (enc.rtxSsrc) {
			rtxOff = FBS::RtpParameters::CreateRtx(builder, *enc.rtxSsrc);
		}

		fbEncodings.push_back(FBS::RtpParameters::CreateRtpEncodingParameters(
			builder,
			enc.ssrc ? flatbuffers::Optional<uint32_t>(*enc.ssrc) : flatbuffers::Optional<uint32_t>(),
			enc.rid.empty() ? 0 : builder.CreateString(enc.rid),
			enc.codecPayloadType ? flatbuffers::Optional<uint8_t>(*enc.codecPayloadType) : flatbuffers::Optional<uint8_t>(),
			rtxOff, enc.dtx,
			enc.scalabilityMode.empty() ? 0 : builder.CreateString(enc.scalabilityMode),
			enc.maxBitrate ? flatbuffers::Optional<uint32_t>(*enc.maxBitrate) : flatbuffers::Optional<uint32_t>()));
	}

	// RTCP
	auto rtcpOff = FBS::RtpParameters::CreateRtcpParameters(
		builder, params.rtcp.cname.empty() ? 0 : builder.CreateString(params.rtcp.cname),
		params.rtcp.reducedSize);

	return FBS::RtpParameters::CreateRtpParameters(
		builder,
		params.mid.empty() ? 0 : builder.CreateString(params.mid),
		builder.CreateVector(fbCodecs),
		builder.CreateVector(fbExts),
		builder.CreateVector(fbEncodings),
		rtcpOff);
}

static flatbuffers::Offset<FBS::RtpParameters::RtpMapping> buildFbsRtpMapping(
	flatbuffers::FlatBufferBuilder& builder, const RtpMapping& mapping)
{
	std::vector<flatbuffers::Offset<FBS::RtpParameters::CodecMapping>> fbCodecs;
	for (auto& cm : mapping.codecs) {
		fbCodecs.push_back(FBS::RtpParameters::CreateCodecMapping(
			builder, cm.payloadType, cm.mappedPayloadType));
	}

	std::vector<flatbuffers::Offset<FBS::RtpParameters::EncodingMapping>> fbEncodings;
	for (auto& em : mapping.encodings) {
		fbEncodings.push_back(FBS::RtpParameters::CreateEncodingMapping(
			builder,
			em.rid.empty() ? 0 : builder.CreateString(em.rid),
			em.ssrc ? flatbuffers::Optional<uint32_t>(*em.ssrc) : flatbuffers::Optional<uint32_t>(),
			em.scalabilityMode.empty() ? 0 : builder.CreateString(em.scalabilityMode),
			em.mappedSsrc));
	}

	return FBS::RtpParameters::CreateRtpMapping(
		builder, builder.CreateVector(fbCodecs), builder.CreateVector(fbEncodings));
}

std::shared_ptr<Producer> Transport::produce(const json& options) {
	if (closed_) throw std::runtime_error("Transport closed");

	std::string kind = options.at("kind").get<std::string>();
	RtpParameters rtpParameters = options.at("rtpParameters").get<RtpParameters>();
	bool paused = options.value("paused", false);

	// Get router RTP capabilities from options (must be passed in)
	RtpCapabilities routerRtpCapabilities = options.at("routerRtpCapabilities").get<RtpCapabilities>();

	// Compute RTP mapping
	auto rtpMapping = ortc::getProducerRtpParametersMapping(rtpParameters, routerRtpCapabilities);

	MS_DEBUG(logger_, "produce [kind:{}] codecs={}", kind, json(rtpMapping.codecs).dump());

	std::string producerId = utils::generateUUIDv4();

	auto& builder = channel_->bufferBuilder();

	auto producerIdOff = builder.CreateString(producerId);
	auto fbKind = (kind == "audio") ? FBS::RtpParameters::MediaKind::AUDIO : FBS::RtpParameters::MediaKind::VIDEO;
	auto rtpParamsOff = buildFbsRtpParameters(builder, rtpParameters);
	auto rtpMappingOff = buildFbsRtpMapping(builder, rtpMapping);

	auto reqOff = FBS::Transport::CreateProduceRequest(
		builder, producerIdOff, fbKind, rtpParamsOff, rtpMappingOff, 0, paused);

	auto future = channel_->request(
		FBS::Request::Method::TRANSPORT_PRODUCE,
		FBS::Request::Body::Transport_ProduceRequest,
		reqOff.Union(), id_);

	future.get();

	// Compute consumable RTP parameters
	auto consumableRtpParams = ortc::getConsumableRtpParameters(
		kind, rtpParameters, routerRtpCapabilities, rtpMapping);

	auto producer = std::make_shared<Producer>(
		producerId, kind, rtpParameters, "simple",
		consumableRtpParams, channel_, id_);

	producers_[producerId] = producer;

	producer->emitter().on("@close", [this, producerId](auto&) {
		producers_.erase(producerId);
		emitter_.emit("@producerclose", {std::any(producerId)});
	});

	// Register for notifications
	channel_->emitter().on(producerId, [producer](const std::vector<std::any>& args) {
		if (!args.empty()) {
			auto event = std::any_cast<FBS::Notification::Event>(args[0]);
			producer->emitter().emit("notification", {std::any(event)});
		}
	});

	return producer;
}

std::shared_ptr<Consumer> Transport::consume(const json& options) {
	if (closed_) throw std::runtime_error("Transport closed");

	std::string producerId = options.at("producerId").get<std::string>();
	RtpCapabilities rtpCapabilities = options.at("rtpCapabilities").get<RtpCapabilities>();
	RtpParameters consumableRtpParameters = options.at("consumableRtpParameters").get<RtpParameters>();
	bool paused = options.value("paused", false);
	bool pipe = options.value("pipe", false);

	auto consumerRtpParams = pipe
		? ortc::getPipeConsumerRtpParameters(consumableRtpParameters)
		: ortc::getConsumerRtpParameters(consumableRtpParameters, rtpCapabilities);

	// Assign unique mid for this consumer
	consumerRtpParams.mid = std::to_string(nextMid_++);

	std::string consumerId = utils::generateUUIDv4();

	auto& builder = channel_->bufferBuilder();

	auto consumerIdOff = builder.CreateString(consumerId);
	auto producerIdOff = builder.CreateString(producerId);
	auto fbKind = (consumableRtpParameters.codecs[0].mimeType.find("audio") != std::string::npos)
		? FBS::RtpParameters::MediaKind::AUDIO : FBS::RtpParameters::MediaKind::VIDEO;
	auto rtpParamsOff = buildFbsRtpParameters(builder, consumerRtpParams);

	auto fbType = pipe ? FBS::RtpParameters::Type::PIPE : FBS::RtpParameters::Type::SIMPLE;

	// Build consumable encodings
	std::vector<flatbuffers::Offset<FBS::RtpParameters::RtpEncodingParameters>> fbConsumableEncodings;
	for (auto& enc : consumableRtpParameters.encodings) {
		fbConsumableEncodings.push_back(FBS::RtpParameters::CreateRtpEncodingParameters(
			builder,
			enc.ssrc ? flatbuffers::Optional<uint32_t>(*enc.ssrc) : flatbuffers::Optional<uint32_t>(),
			0, flatbuffers::Optional<uint8_t>(), 0, false, 0, flatbuffers::Optional<uint32_t>()));
	}

	auto reqOff = FBS::Transport::CreateConsumeRequest(
		builder, consumerIdOff, producerIdOff, fbKind, rtpParamsOff,
		fbType, builder.CreateVector(fbConsumableEncodings), paused, 0, false);

	auto future = channel_->request(
		FBS::Request::Method::TRANSPORT_CONSUME,
		FBS::Request::Body::Transport_ConsumeRequest,
		reqOff.Union(), id_);

	future.get();

	std::string kind = (fbKind == FBS::RtpParameters::MediaKind::AUDIO) ? "audio" : "video";
	std::string type = pipe ? "pipe" : "simple";

	auto consumer = std::make_shared<Consumer>(
		consumerId, producerId, kind, consumerRtpParams, type, channel_, id_);

	consumers_[consumerId] = consumer;

	consumer->emitter().on("@close", [this, consumerId](auto&) {
		consumers_.erase(consumerId);
	});

	// Register for notifications
	channel_->emitter().on(consumerId, [consumer](const std::vector<std::any>& args) {
		if (!args.empty()) {
			auto event = std::any_cast<FBS::Notification::Event>(args[0]);
			consumer->handleNotification(event, nullptr);
		}
	});

	return consumer;
}

void Transport::close() {
	if (closed_) return;
	closed_ = true;

	auto& builder = channel_->bufferBuilder();
	auto idOff = builder.CreateString(id_);
	auto reqOff = FBS::Router::CreateCloseTransportRequest(builder, idOff);

	try {
		channel_->request(FBS::Request::Method::ROUTER_CLOSE_TRANSPORT,
			FBS::Request::Body::Router_CloseTransportRequest,
			reqOff.Union(), routerId_).get();
	} catch (...) {}

	// Close all producers and consumers
	for (auto& [id, p] : producers_) p->transportClosed();
	producers_.clear();
	for (auto& [id, c] : consumers_) c->transportClosed();
	consumers_.clear();

	emitter_.emit("@close");
}

void Transport::routerClosed() {
	if (closed_) return;
	closed_ = true;
	for (auto& [id, p] : producers_) p->transportClosed();
	producers_.clear();
	for (auto& [id, c] : consumers_) c->transportClosed();
	consumers_.clear();
	emitter_.emit("routerclose");
}

} // namespace mediasoup
