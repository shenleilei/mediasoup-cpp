#include "Producer.h"
#include "RtpStreamStatsJson.h"
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

	try {
		channel_->requestBuild(FBS::Request::Method::TRANSPORT_CLOSE_PRODUCER,
			FBS::Request::Body::Transport_CloseProducerRequest,
			[this](flatbuffers::FlatBufferBuilder& builder) {
				auto idOff = builder.CreateString(id_);
				auto reqOff = FBS::Transport::CreateCloseProducerRequest(builder, idOff);
				return reqOff.Union();
			}, transportId_);
			// Fire-and-forget: don't .get() — avoids blocking control thread if worker is slow
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

json Producer::getStats(int timeoutMs) {
	if (closed_) return json::array();

	auto owned = channel_->requestWait(FBS::Request::Method::PRODUCER_GET_STATS,
		FBS::Request::Body::NONE, 0, id_, timeoutMs);
	auto* response = owned.response();

	json result = json::array();
	if (!response || response->body_type() != FBS::Response::Body::Producer_GetStatsResponse)
		return result;

	auto statsResp = response->body_as_Producer_GetStatsResponse();
	if (!statsResp || !statsResp->stats()) return result;

	for (size_t i = 0; i < statsResp->stats()->size(); i++) {
		auto* stat = statsResp->stats()->Get(i);
		if (!stat || !stat->data()) continue;

		json entry = ProducerRecvStatsToJson(stat);
		if (!entry.empty()) result.push_back(entry);
	}
	return result;
}

} // namespace mediasoup
