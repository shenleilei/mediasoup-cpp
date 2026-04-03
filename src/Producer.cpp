#include "Producer.h"
#include "request_generated.h"
#include "transport_generated.h"

namespace mediasoup {

void Producer::pause() {
	if (closed_) return;
	auto future = channel_->request(FBS::Request::Method::PRODUCER_PAUSE,
		FBS::Request::Body::NONE, 0, id_);
	future.get();
	paused_ = true;
	emitter_.emit("pause");
}

void Producer::resume() {
	if (closed_) return;
	auto future = channel_->request(FBS::Request::Method::PRODUCER_RESUME,
		FBS::Request::Body::NONE, 0, id_);
	future.get();
	paused_ = false;
	emitter_.emit("resume");
}

void Producer::close() {
	if (closed_) return;
	closed_ = true;

	auto& builder = channel_->bufferBuilder();
	auto idOff = builder.CreateString(id_);
	auto reqOff = FBS::Transport::CreateCloseProducerRequest(builder, idOff);

	try {
		channel_->request(FBS::Request::Method::TRANSPORT_CLOSE_PRODUCER,
			FBS::Request::Body::Transport_CloseProducerRequest,
			reqOff.Union(), transportId_).get();
	} catch (...) {}

	emitter_.emit("@close");
}

void Producer::transportClosed() {
	if (closed_) return;
	closed_ = true;
	emitter_.emit("transportclose");
}

} // namespace mediasoup
