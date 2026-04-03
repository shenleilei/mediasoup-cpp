#include "Consumer.h"
#include "request_generated.h"
#include "transport_generated.h"
#include "consumer_generated.h"

namespace mediasoup {

void Consumer::pause() {
	if (closed_) return;
	channel_->request(FBS::Request::Method::CONSUMER_PAUSE,
		FBS::Request::Body::NONE, 0, id_).get();
	paused_ = true;
	emitter_.emit("pause");
}

void Consumer::resume() {
	if (closed_) return;
	channel_->request(FBS::Request::Method::CONSUMER_RESUME,
		FBS::Request::Body::NONE, 0, id_).get();
	paused_ = false;
	emitter_.emit("resume");
}

void Consumer::setPreferredLayers(uint8_t spatialLayer, uint8_t temporalLayer) {
	if (closed_) return;
	auto& builder = channel_->bufferBuilder();
	auto layersOff = FBS::Consumer::CreateConsumerLayers(builder, spatialLayer, temporalLayer);
	auto reqOff = FBS::Consumer::CreateSetPreferredLayersRequest(builder, layersOff);
	channel_->request(FBS::Request::Method::CONSUMER_SET_PREFERRED_LAYERS,
		FBS::Request::Body::Consumer_SetPreferredLayersRequest,
		reqOff.Union(), id_).get();
}

void Consumer::setPriority(uint8_t priority) {
	if (closed_) return;
	auto& builder = channel_->bufferBuilder();
	auto reqOff = FBS::Consumer::CreateSetPriorityRequest(builder, priority);
	channel_->request(FBS::Request::Method::CONSUMER_SET_PRIORITY,
		FBS::Request::Body::Consumer_SetPriorityRequest,
		reqOff.Union(), id_).get();
}

void Consumer::requestKeyFrame() {
	if (closed_) return;
	channel_->request(FBS::Request::Method::CONSUMER_REQUEST_KEY_FRAME,
		FBS::Request::Body::NONE, 0, id_).get();
}

void Consumer::close() {
	if (closed_) return;
	closed_ = true;

	auto& builder = channel_->bufferBuilder();
	auto idOff = builder.CreateString(id_);
	auto reqOff = FBS::Transport::CreateCloseConsumerRequest(builder, idOff);

	try {
		channel_->request(FBS::Request::Method::TRANSPORT_CLOSE_CONSUMER,
			FBS::Request::Body::Transport_CloseConsumerRequest,
			reqOff.Union(), transportId_).get();
	} catch (...) {}

	emitter_.emit("@close");
}

void Consumer::transportClosed() {
	if (closed_) return;
	closed_ = true;
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
		case FBS::Notification::Event::CONSUMER_SCORE:
			emitter_.emit("score");
			break;
		default:
			break;
	}
}

} // namespace mediasoup
