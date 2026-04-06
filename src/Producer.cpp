#include "Producer.h"
#include "request_generated.h"
#include "transport_generated.h"
#include "producer_generated.h"
#include "rtpStream_generated.h"

namespace mediasoup {

void Producer::pause() {
	if (closed_) return;
	channel_->requestWait(FBS::Request::Method::PRODUCER_PAUSE,
		FBS::Request::Body::NONE, 0, id_);
	paused_ = true;
	emitter_.emit("pause");
}

void Producer::resume() {
	if (closed_) return;
	channel_->requestWait(FBS::Request::Method::PRODUCER_RESUME,
		FBS::Request::Body::NONE, 0, id_);
	paused_ = false;
	emitter_.emit("resume");
}

void Producer::close() {
	if (closed_) return;
	closed_ = true;

	channel_->emitter().off(id_);

	auto& builder = channel_->bufferBuilder();
	auto idOff = builder.CreateString(id_);
	auto reqOff = FBS::Transport::CreateCloseProducerRequest(builder, idOff);

	try {
		channel_->request(FBS::Request::Method::TRANSPORT_CLOSE_PRODUCER,
			FBS::Request::Body::Transport_CloseProducerRequest,
			reqOff.Union(), transportId_).get();
	} catch (const std::exception& e) {
		spdlog::warn("Producer::close() request failed [id:{}]: {}", id_, e.what());
	} catch (...) {
		spdlog::warn("Producer::close() request failed [id:{}]: unknown error", id_);
	}

	emitter_.emit("@close");
}

void Producer::transportClosed() {
	if (closed_) return;
	closed_ = true;
	channel_->emitter().off(id_);
	emitter_.emit("transportclose");
}

void Producer::handleNotification(
	FBS::Notification::Event event,
	const FBS::Notification::Notification* notification)
{
	switch (event) {
		case FBS::Notification::Event::PRODUCER_SCORE: {
			if (notification) {
				auto body = notification->body_as_Producer_ScoreNotification();
				if (body && body->scores()) {
					scores_.clear();
					for (size_t i = 0; i < body->scores()->size(); i++) {
						auto* s = body->scores()->Get(i);
						scores_.push_back({s->encoding_idx(), s->ssrc(),
							s->rid() ? s->rid()->str() : "", s->score()});
					}
				}
			}
			emitter_.emit("score");
			break;
		}
		default:
			break;
	}
}

json Producer::getStats() {
	if (closed_) return json::array();

	auto owned = channel_->requestWait(FBS::Request::Method::PRODUCER_GET_STATS,
		FBS::Request::Body::NONE, 0, id_);
	auto* response = owned.response();

	json result = json::array();
	if (!response || response->body_type() != FBS::Response::Body::Producer_GetStatsResponse)
		return result;

	auto statsResp = response->body_as_Producer_GetStatsResponse();
	if (!statsResp || !statsResp->stats()) return result;

	for (size_t i = 0; i < statsResp->stats()->size(); i++) {
		auto* stat = statsResp->stats()->Get(i);
		if (!stat || !stat->data()) continue;

		json entry;
		auto dataType = stat->data_type();

		if (dataType == FBS::RtpStream::StatsData::RecvStats) {
			auto recv = stat->data_as_RecvStats();
			if (!recv || !recv->base() || !recv->base()->data()) continue;
			auto base = recv->base()->data_as_BaseStats();
			if (!base) continue;

			entry["type"] = "inbound-rtp";
			entry["ssrc"] = base->ssrc();
			entry["kind"] = base->kind() == FBS::RtpParameters::MediaKind::AUDIO ? "audio" : "video";
			entry["mimeType"] = base->mime_type() ? base->mime_type()->str() : "";
			entry["packetsLost"] = base->packets_lost();
			entry["fractionLost"] = base->fraction_lost();
			entry["packetsDiscarded"] = base->packets_discarded();
			entry["packetsRetransmitted"] = base->packets_retransmitted();
			entry["packetsRepaired"] = base->packets_repaired();
			entry["nackCount"] = base->nack_count();
			entry["nackPacketCount"] = base->nack_packet_count();
			entry["pliCount"] = base->pli_count();
			entry["firCount"] = base->fir_count();
			entry["score"] = base->score();
			entry["roundTripTime"] = base->round_trip_time();
			if (base->rid()) entry["rid"] = base->rid()->str();
			entry["jitter"] = recv->jitter();
			entry["packetCount"] = recv->packet_count();
			entry["byteCount"] = recv->byte_count();
			entry["bitrate"] = recv->bitrate();
		}

		if (!entry.empty()) result.push_back(entry);
	}
	return result;
}

} // namespace mediasoup
