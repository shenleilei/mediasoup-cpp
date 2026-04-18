#include "Transport.h"
#include "Producer.h"
#include "Consumer.h"
#include "RtpParametersFbs.h"
#include "ortc.h"
#include "Utils.h"
#include "request_generated.h"
#include "transport_generated.h"
#include "router_generated.h"
#include "webRtcTransport_generated.h"
#include "plainTransport_generated.h"

namespace mediasoup {

static std::string getRequiredString(const json& options, const char* key)
{
	if (!options.contains(key) || !options.at(key).is_string())
		throw std::invalid_argument(std::string("invalid or missing '") + key + "'");
	return options.at(key).get<std::string>();
}

static bool getOptionalBool(const json& options, const char* key, bool defaultValue)
{
	if (!options.contains(key)) return defaultValue;
	if (!options.at(key).is_boolean())
		throw std::invalid_argument(std::string("invalid '") + key + "': expected boolean");
	return options.at(key).get<bool>();
}

static void validateNotificationArgs(const std::vector<std::any>& args,
	const char* owner, const std::string& id,
	FBS::Notification::Event& event,
	const FBS::Notification::Notification*& notif)
{
	notif = nullptr;
	if (args.empty()) {
		spdlog::warn("{} notification args empty [id:{}]", owner, id);
		throw std::invalid_argument("empty notification args");
	}
	event = std::any_cast<FBS::Notification::Event>(args[0]);
	if (args.size() > 1) {
		auto owned = std::any_cast<std::shared_ptr<Channel::OwnedNotification>>(args[1]);
		if (owned) notif = owned->notification();
	}
}

std::shared_ptr<Producer> Transport::produce(const json& options) {
	if (closed_) throw std::runtime_error("Transport closed");

	if (!options.contains("rtpParameters") || !options.at("rtpParameters").is_object()) {
		MS_WARN(logger_, "produce validation failed [transportId:{}]: invalid rtpParameters", id_);
		throw std::invalid_argument("invalid or missing 'rtpParameters'");
	}
	if (!options.contains("routerRtpCapabilities") || !options.at("routerRtpCapabilities").is_object()) {
		MS_WARN(logger_, "produce validation failed [transportId:{}]: invalid routerRtpCapabilities", id_);
		throw std::invalid_argument("invalid or missing 'routerRtpCapabilities'");
	}
	std::string kind = getRequiredString(options, "kind");
	if (kind != "audio" && kind != "video") {
		MS_WARN(logger_, "produce validation failed [transportId:{} kind:{}]", id_, kind);
		throw std::invalid_argument("invalid 'kind': expected 'audio' or 'video'");
	}
	RtpParameters rtpParameters = options.at("rtpParameters").get<RtpParameters>();
	bool paused = getOptionalBool(options, "paused", false);

	// Get router RTP capabilities from options (must be passed in)
	RtpCapabilities routerRtpCapabilities = options.at("routerRtpCapabilities").get<RtpCapabilities>();

	// Compute RTP mapping
	auto rtpMapping = ortc::getProducerRtpParametersMapping(rtpParameters, routerRtpCapabilities);

	MS_DEBUG(logger_, "produce [kind:{}] codecs={}", kind, json(rtpMapping.codecs).dump());

	std::string producerId = utils::generateUUIDv4();
	channel_->requestBuildWait(
		FBS::Request::Method::TRANSPORT_PRODUCE,
		FBS::Request::Body::Transport_ProduceRequest,
		[producerId, &kind, &rtpParameters, &rtpMapping, paused](flatbuffers::FlatBufferBuilder& builder) {
			auto producerIdOff = builder.CreateString(producerId);
			auto fbKind = (kind == "audio") ? FBS::RtpParameters::MediaKind::AUDIO : FBS::RtpParameters::MediaKind::VIDEO;
			auto rtpParamsOff = BuildFbsRtpParameters(builder, rtpParameters);
			auto rtpMappingOff = BuildFbsRtpMapping(builder, rtpMapping);
			auto reqOff = FBS::Transport::CreateProduceRequest(
				builder, producerIdOff, fbKind, rtpParamsOff, rtpMappingOff, 0, paused);
			return reqOff.Union();
		}, id_);

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
		try {
			FBS::Notification::Event event;
			const FBS::Notification::Notification* notif = nullptr;
			validateNotificationArgs(args, "Producer", producer->id(), event, notif);
			producer->handleNotification(event, notif);
		} catch (const std::bad_any_cast& e) {
			spdlog::warn("Producer notification cast failed [id:{}]: {}", producer->id(), e.what());
		} catch (const std::exception& e) {
			spdlog::warn("Producer notification dropped [id:{}]: {}", producer->id(), e.what());
		} catch (...) {
			spdlog::warn("Producer notification dropped [id:{}]: unknown error", producer->id());
		}
	});

	return producer;
}

std::shared_ptr<Consumer> Transport::consume(const json& options) {
	if (closed_) throw std::runtime_error("Transport closed");

	if (!options.contains("rtpCapabilities") || !options.at("rtpCapabilities").is_object()) {
		MS_WARN(logger_, "consume validation failed [transportId:{}]: invalid rtpCapabilities", id_);
		throw std::invalid_argument("invalid or missing 'rtpCapabilities'");
	}
	if (!options.contains("consumableRtpParameters") || !options.at("consumableRtpParameters").is_object()) {
		MS_WARN(logger_, "consume validation failed [transportId:{}]: invalid consumableRtpParameters", id_);
		throw std::invalid_argument("invalid or missing 'consumableRtpParameters'");
	}
	std::string producerId = getRequiredString(options, "producerId");
	RtpCapabilities rtpCapabilities = options.at("rtpCapabilities").get<RtpCapabilities>();
	RtpParameters consumableRtpParameters = options.at("consumableRtpParameters").get<RtpParameters>();
	bool paused = getOptionalBool(options, "paused", false);
	bool pipe = getOptionalBool(options, "pipe", false);
	if (consumableRtpParameters.codecs.empty()) {
		MS_WARN(logger_, "consume validation failed [transportId:{}]: empty codecs", id_);
		throw std::invalid_argument("invalid 'consumableRtpParameters': codecs cannot be empty");
	}
	if (consumableRtpParameters.codecs[0].mimeType.empty()) {
		MS_WARN(logger_, "consume validation failed [transportId:{}]: empty codecs[0].mimeType", id_);
		throw std::invalid_argument("invalid 'consumableRtpParameters': codecs[0].mimeType cannot be empty");
	}

	auto consumerRtpParams = pipe
		? ortc::getPipeConsumerRtpParameters(consumableRtpParameters)
		: ortc::getConsumerRtpParameters(consumableRtpParameters, rtpCapabilities);

	// Assign unique mid for this consumer
	consumerRtpParams.mid = std::to_string(nextMid_++);

	std::string consumerId = utils::generateUUIDv4();
	std::string kind = (consumableRtpParameters.codecs[0].mimeType.find("audio") != std::string::npos)
		? "audio" : "video";
	channel_->requestBuildWait(
		FBS::Request::Method::TRANSPORT_CONSUME,
		FBS::Request::Body::Transport_ConsumeRequest,
		[consumerId, &producerId, &consumableRtpParameters, &consumerRtpParams, paused, pipe](flatbuffers::FlatBufferBuilder& builder) {
			auto consumerIdOff = builder.CreateString(consumerId);
			auto producerIdOff = builder.CreateString(producerId);
			auto fbKind = (consumableRtpParameters.codecs[0].mimeType.find("audio") != std::string::npos)
				? FBS::RtpParameters::MediaKind::AUDIO : FBS::RtpParameters::MediaKind::VIDEO;
			auto rtpParamsOff = BuildFbsRtpParameters(builder, consumerRtpParams);
			auto fbType = pipe ? FBS::RtpParameters::Type::PIPE : FBS::RtpParameters::Type::SIMPLE;

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
			return reqOff.Union();
		}, id_);

	std::string type = pipe ? "pipe" : "simple";

	auto consumer = std::make_shared<Consumer>(
		consumerId, producerId, kind, consumerRtpParams, type, channel_, id_);

	consumers_[consumerId] = consumer;

	consumer->emitter().on("@close", [this, consumerId](auto&) {
		consumers_.erase(consumerId);
	});

	// Register for notifications
	channel_->emitter().on(consumerId, [consumer](const std::vector<std::any>& args) {
		try {
			FBS::Notification::Event event;
			const FBS::Notification::Notification* notif = nullptr;
			validateNotificationArgs(args, "Consumer", consumer->id(), event, notif);
			consumer->handleNotification(event, notif);
		} catch (const std::bad_any_cast& e) {
			spdlog::warn("Consumer notification cast failed [id:{}]: {}", consumer->id(), e.what());
		} catch (const std::exception& e) {
			spdlog::warn("Consumer notification dropped [id:{}]: {}", consumer->id(), e.what());
		} catch (...) {
			spdlog::warn("Consumer notification dropped [id:{}]: unknown error", consumer->id());
		}
	});

	return consumer;
}

json Transport::getStats(int timeoutMs) {
	if (closed_) return json::object();

	auto owned = channel_->requestWait(FBS::Request::Method::TRANSPORT_GET_STATS,
		FBS::Request::Body::NONE, 0, id_, timeoutMs);
	auto* response = owned.response();

	json result;

	// Try WebRtcTransport stats first (most common)
	if (response && response->body_type() == FBS::Response::Body::WebRtcTransport_GetStatsResponse) {
		auto statsResp = response->body_as_WebRtcTransport_GetStatsResponse();
		if (statsResp && statsResp->base()) {
			auto* s = statsResp->base();
			result["transportId"] = s->transport_id() ? s->transport_id()->str() : "";
			result["timestamp"] = s->timestamp();
			result["bytesReceived"] = s->bytes_received();
			result["recvBitrate"] = s->recv_bitrate();
			result["bytesSent"] = s->bytes_sent();
			result["sendBitrate"] = s->send_bitrate();
			result["rtpBytesReceived"] = s->rtp_bytes_received();
			result["rtpRecvBitrate"] = s->rtp_recv_bitrate();
			result["rtpBytesSent"] = s->rtp_bytes_sent();
			result["rtpSendBitrate"] = s->rtp_send_bitrate();
			result["rtxBytesReceived"] = s->rtx_bytes_received();
			result["rtxRecvBitrate"] = s->rtx_recv_bitrate();
			result["rtxBytesSent"] = s->rtx_bytes_sent();
			result["rtxSendBitrate"] = s->rtx_send_bitrate();
			result["probationBytesSent"] = s->probation_bytes_sent();
			result["probationSendBitrate"] = s->probation_send_bitrate();
			result["availableOutgoingBitrate"] = s->available_outgoing_bitrate();
			result["availableIncomingBitrate"] = s->available_incoming_bitrate();
			result["maxIncomingBitrate"] = s->max_incoming_bitrate();
			result["rtpPacketLossReceived"] = s->rtp_packet_loss_received();
			result["rtpPacketLossSent"] = s->rtp_packet_loss_sent();

			// WebRTC-specific fields
			result["iceState"] = FBS::WebRtcTransport::EnumNameIceState(statsResp->ice_state());
			result["dtlsState"] = FBS::WebRtcTransport::EnumNameDtlsState(statsResp->dtls_state());
			if (statsResp->ice_selected_tuple()) {
				auto* t = statsResp->ice_selected_tuple();
				result["iceSelectedTuple"] = {
					{"localAddress", t->local_address() ? t->local_address()->str() : ""},
					{"localPort", t->local_port()},
					{"remoteIp", t->remote_ip() ? t->remote_ip()->str() : ""},
					{"remotePort", t->remote_port()},
					{"protocol", t->protocol() == FBS::Transport::Protocol::UDP ? "udp" : "tcp"}
				};
			}
		}
	}
	// PlainTransport stats
	else if (response && response->body_type() == FBS::Response::Body::PlainTransport_GetStatsResponse) {
		auto statsResp = response->body_as_PlainTransport_GetStatsResponse();
		if (statsResp && statsResp->base()) {
			auto* s = statsResp->base();
			result["transportId"] = s->transport_id() ? s->transport_id()->str() : "";
			result["recvBitrate"] = s->recv_bitrate();
			result["sendBitrate"] = s->send_bitrate();
			result["rtpRecvBitrate"] = s->rtp_recv_bitrate();
			result["rtpSendBitrate"] = s->rtp_send_bitrate();
			result["rtpPacketLossReceived"] = s->rtp_packet_loss_received();
		}
	}

	return result;
}

void Transport::close() {
	if (closed_) return;
	closed_ = true;

	// Unregister channel listener to break shared_ptr cycle
	channel_->emitter().off(id_);

	try {
		channel_->requestBuild(FBS::Request::Method::ROUTER_CLOSE_TRANSPORT,
			FBS::Request::Body::Router_CloseTransportRequest,
			[this](flatbuffers::FlatBufferBuilder& builder) {
				auto idOff = builder.CreateString(id_);
				auto reqOff = FBS::Router::CreateCloseTransportRequest(builder, idOff);
				return reqOff.Union();
			}, routerId_);
	} catch (const std::exception& e) {
		spdlog::warn("Transport::close() request failed [id:{}]: {}", id_, e.what());
	} catch (...) {
		spdlog::warn("Transport::close() request failed [id:{}]: unknown error", id_);
	}

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

	channel_->emitter().off(id_);

	for (auto& [id, p] : producers_) p->transportClosed();
	producers_.clear();
	for (auto& [id, c] : consumers_) c->transportClosed();
	consumers_.clear();
	emitter_.emit("routerclose");
}

} // namespace mediasoup
