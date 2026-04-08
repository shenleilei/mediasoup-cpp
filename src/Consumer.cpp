#include "Consumer.h"
#include "request_generated.h"
#include "transport_generated.h"
#include "consumer_generated.h"
#include "rtpStream_generated.h"

namespace mediasoup {

void Consumer::pause() {
	if (closed_) return;
	channel_->requestWait(FBS::Request::Method::CONSUMER_PAUSE,
		FBS::Request::Body::NONE, 0, id_);
	paused_ = true;
	emitter_.emit("pause");
}

void Consumer::resume() {
	if (closed_) return;
	channel_->requestWait(FBS::Request::Method::CONSUMER_RESUME,
		FBS::Request::Body::NONE, 0, id_);
	paused_ = false;
	emitter_.emit("resume");
}

void Consumer::setPreferredLayers(uint8_t spatialLayer, uint8_t temporalLayer) {
	if (closed_) return;
	auto& builder = channel_->bufferBuilder();
	auto layersOff = FBS::Consumer::CreateConsumerLayers(builder, spatialLayer, temporalLayer);
	auto reqOff = FBS::Consumer::CreateSetPreferredLayersRequest(builder, layersOff);
	channel_->requestWait(FBS::Request::Method::CONSUMER_SET_PREFERRED_LAYERS,
		FBS::Request::Body::Consumer_SetPreferredLayersRequest,
		reqOff.Union(), id_);
}

void Consumer::setPriority(uint8_t priority) {
	if (closed_) return;
	auto& builder = channel_->bufferBuilder();
	auto reqOff = FBS::Consumer::CreateSetPriorityRequest(builder, priority);
	channel_->requestWait(FBS::Request::Method::CONSUMER_SET_PRIORITY,
		FBS::Request::Body::Consumer_SetPriorityRequest,
		reqOff.Union(), id_);
}

void Consumer::requestKeyFrame() {
	if (closed_) return;
	channel_->requestWait(FBS::Request::Method::CONSUMER_REQUEST_KEY_FRAME,
		FBS::Request::Body::NONE, 0, id_);
}

void Consumer::close() {
	if (closed_) return;
	closed_ = true;

	channel_->emitter().off(id_);

	auto& builder = channel_->bufferBuilder();
	auto idOff = builder.CreateString(id_);
	auto reqOff = FBS::Transport::CreateCloseConsumerRequest(builder, idOff);

	try {
		channel_->request(FBS::Request::Method::TRANSPORT_CLOSE_CONSUMER,
			FBS::Request::Body::Transport_CloseConsumerRequest,
			reqOff.Union(), transportId_).get();
	} catch (const std::exception& e) {
		spdlog::warn("Consumer::close() request failed [id:{}]: {}", id_, e.what());
	} catch (...) {
		spdlog::warn("Consumer::close() request failed [id:{}]: unknown error", id_);
	}

	emitter_.emit("@close");
}

void Consumer::transportClosed() {
	if (closed_) return;
	closed_ = true;
	channel_->emitter().off(id_);
	emitter_.emit("transportclose");
}

void Consumer::handleNotification(
	FBS::Notification::Event event,
	const FBS::Notification::Notification* notification)
{
	switch (event) {
		case FBS::Notification::Event::CONSUMER_PRODUCER_PAUSE:
			producerPaused_ = true;
			emitter_.emit("producerpause");
			break;
		case FBS::Notification::Event::CONSUMER_PRODUCER_RESUME:
			producerPaused_ = false;
			emitter_.emit("producerresume");
			break;
		case FBS::Notification::Event::CONSUMER_PRODUCER_CLOSE:
			producerPaused_ = true;
			emitter_.emit("producerclose");
			break;
		case FBS::Notification::Event::CONSUMER_LAYERS_CHANGE:
			emitter_.emit("layerschange");
			break;
		case FBS::Notification::Event::CONSUMER_SCORE: {
			if (notification) {
				auto body = notification->body_as_Consumer_ScoreNotification();
				if (body && body->score()) {
					score_.score = body->score()->score();
					score_.producerScore = body->score()->producer_score();
					MS_DEBUG(logger_, "Consumer {} ({}) score={} producerScore={}",
						id_, kind_, score_.score, score_.producerScore);
					score_.producerScores.clear();
					if (body->score()->producer_scores())
						for (size_t i = 0; i < body->score()->producer_scores()->size(); i++)
							score_.producerScores.push_back(body->score()->producer_scores()->Get(i));
				}
			}
			emitter_.emit("score");
			break;
		}
		default:
			break;
	}
}

json Consumer::getStats(int timeoutMs) {
	if (closed_) return json::array();

	auto owned = channel_->requestWait(FBS::Request::Method::CONSUMER_GET_STATS,
		FBS::Request::Body::NONE, 0, id_, timeoutMs);
	auto* response = owned.response();

	json result = json::array();
	if (!response || response->body_type() != FBS::Response::Body::Consumer_GetStatsResponse)
		return result;

	auto statsResp = response->body_as_Consumer_GetStatsResponse();
	if (!statsResp || !statsResp->stats()) return result;

	for (size_t i = 0; i < statsResp->stats()->size(); i++) {
		auto* stat = statsResp->stats()->Get(i);
		if (!stat || !stat->data()) continue;

		json entry;
		auto dataType = stat->data_type();

		if (dataType == FBS::RtpStream::StatsData::SendStats) {
			auto send = stat->data_as_SendStats();
			if (!send || !send->base() || !send->base()->data()) continue;
			auto base = send->base()->data_as_BaseStats();
			if (!base) continue;

			entry["type"] = "outbound-rtp";
			entry["ssrc"] = base->ssrc();
			entry["kind"] = base->kind() == FBS::RtpParameters::MediaKind::AUDIO ? "audio" : "video";
			entry["mimeType"] = base->mime_type() ? base->mime_type()->str() : "";
			entry["packetsLost"] = base->packets_lost();
			entry["fractionLost"] = base->fraction_lost();
			entry["nackCount"] = base->nack_count();
			entry["pliCount"] = base->pli_count();
			entry["firCount"] = base->fir_count();
			entry["score"] = base->score();
			entry["roundTripTime"] = base->round_trip_time();
			entry["packetCount"] = send->packet_count();
			entry["byteCount"] = send->byte_count();
			entry["bitrate"] = send->bitrate();
		}

		if (!entry.empty()) result.push_back(entry);
	}
	return result;
}

} // namespace mediasoup
