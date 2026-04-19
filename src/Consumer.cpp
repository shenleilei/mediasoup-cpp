#include "Consumer.h"
#include "RtpStreamStatsJson.h"
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
	channel_->requestBuildWait(FBS::Request::Method::CONSUMER_SET_PREFERRED_LAYERS,
		FBS::Request::Body::Consumer_SetPreferredLayersRequest,
		[spatialLayer, temporalLayer](flatbuffers::FlatBufferBuilder& builder) {
			auto layersOff = FBS::Consumer::CreateConsumerLayers(builder, spatialLayer, temporalLayer);
			auto reqOff = FBS::Consumer::CreateSetPreferredLayersRequest(builder, layersOff);
			return reqOff.Union();
		}, id_);
	preferredSpatialLayer_ = spatialLayer;
	preferredTemporalLayer_ = temporalLayer;
}

void Consumer::setPriority(uint8_t priority) {
	if (closed_) return;
	channel_->requestBuildWait(FBS::Request::Method::CONSUMER_SET_PRIORITY,
		FBS::Request::Body::Consumer_SetPriorityRequest,
		[priority](flatbuffers::FlatBufferBuilder& builder) {
			auto reqOff = FBS::Consumer::CreateSetPriorityRequest(builder, priority);
			return reqOff.Union();
		}, id_);
	priority_ = priority;
}

void Consumer::requestKeyFrame() {
	if (closed_) return;
	channel_->requestWait(FBS::Request::Method::CONSUMER_REQUEST_KEY_FRAME,
		FBS::Request::Body::NONE, 0, id_);
}

void Consumer::close() {
	if (closed_) return;
	closed_ = true;

	if (channel_) {
		channel_->emitter().off(id_);
	}

	try {
		if (channel_) {
			channel_->requestBuild(FBS::Request::Method::TRANSPORT_CLOSE_CONSUMER,
				FBS::Request::Body::Transport_CloseConsumerRequest,
				[this](flatbuffers::FlatBufferBuilder& builder) {
					auto idOff = builder.CreateString(id_);
					auto reqOff = FBS::Transport::CreateCloseConsumerRequest(builder, idOff);
					return reqOff.Union();
				}, transportId_);
		} else {
			spdlog::warn("Consumer::close() skipped close request due to null channel [id:{}]", id_);
		}
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
	if (channel_) {
		channel_->emitter().off(id_);
	}
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
			if (closed_) break;
			closed_ = true;
			producerPaused_ = true;
			if (channel_) {
				channel_->emitter().off(id_);
			}
			emitter_.emit("@close");
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

		json entry = ConsumerSendStatsToJson(stat);
		if (!entry.empty()) result.push_back(entry);
	}
	return result;
}

} // namespace mediasoup
