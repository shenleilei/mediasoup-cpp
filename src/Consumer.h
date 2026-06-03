#pragma once
#include "Channel.h"
#include "Constants.h"
#include "RtpTypes.h"
#include "EventEmitter.h"
#include "Logger.h"
#include <string>
#include <memory>

namespace mediasoup {

class Consumer {
public:
	Consumer(const std::string& id, const std::string& producerId,
		const std::string& kind, const RtpParameters& rtpParameters,
		const std::string& type, Channel* channel, const std::string& transportId)
		: id_(id), producerId_(producerId), kind_(kind)
		, rtpParameters_(rtpParameters), type_(type)
		, channel_(channel), transportId_(transportId)
		, logger_(Logger::Get("Consumer")) {}
	Consumer(const Consumer&) = delete;
	Consumer& operator=(const Consumer&) = delete;
	Consumer(Consumer&&) = delete;
	Consumer& operator=(Consumer&&) = delete;

	const std::string& id() const { return id_; }
	const std::string& producerId() const { return producerId_; }
	const std::string& kind() const { return kind_; }
	const RtpParameters& rtpParameters() const { return rtpParameters_; }
	const std::string& type() const { return type_; }
	bool closed() const { return closed_; }
	bool paused() const { return paused_; }
	bool producerPaused() const { return producerPaused_; }
	uint8_t preferredSpatialLayer() const { return preferredSpatialLayer_; }
	uint8_t preferredTemporalLayer() const { return preferredTemporalLayer_; }
	uint8_t priority() const { return priority_; }
	EventEmitter& emitter() { return emitter_; }
	void setChannelListenerId(uint64_t listenerId) { channelListenerId_ = listenerId; }
	void setContext(std::string roomId, std::string peerId) {
		roomId_ = std::move(roomId);
		peerId_ = std::move(peerId);
	}
	const std::string& roomId() const { return roomId_; }
	const std::string& peerId() const { return peerId_; }
	void setProducerPeerId(std::string producerPeerId) {
		producerPeerId_ = std::move(producerPeerId);
	}
	const std::string& producerPeerId() const { return producerPeerId_; }
	std::string logPrefix() const {
		if (roomId_.empty() && peerId_.empty()) return "[" + id_ + "]";
		if (peerId_.empty()) return "[" + roomId_ + " " + id_ + "]";
		return "[" + roomId_ + " " + peerId_ + " " + id_ + "]";
	}

	void pause();
	void resume();
	void setPreferredLayers(uint8_t spatialLayer, uint8_t temporalLayer);
	void setPriority(uint8_t priority);
	void requestKeyFrame();
	void close();
	void transportClosed();

	json toJson() const {
		return {{"id", id_}, {"producerId", producerId_}, {"kind", kind_},
			{"type", type_}, {"paused", paused_}, {"producerPaused", producerPaused_},
			{"rtpParameters", rtpParameters_},
			{"preferredSpatialLayer", preferredSpatialLayer_},
			{"preferredTemporalLayer", preferredTemporalLayer_},
			{"priority", priority_}};
	}

	void handleNotification(FBS::Notification::Event event, const FBS::Notification::Notification* notification);

	struct Score { uint8_t score = 0; uint8_t producerScore = 0; std::vector<uint8_t> producerScores; };
	const Score& currentScore() const { return score_; }
	json getStats(int timeoutMs = kChannelRequestTimeoutMs);

private:
	std::string id_;
	std::string producerId_;
	std::string kind_;
	RtpParameters rtpParameters_;
	std::string type_;
	Channel* channel_;
	std::string transportId_;
	std::string roomId_;
	std::string peerId_;
	std::string producerPeerId_;
	bool closed_ = false;
	bool paused_ = false;
	bool producerPaused_ = false;
	Score score_;
	EventEmitter emitter_;
	std::shared_ptr<spdlog::logger> logger_;
	uint8_t preferredSpatialLayer_ = 0;
	uint8_t preferredTemporalLayer_ = 0;
	uint8_t priority_ = 1;
	uint64_t channelListenerId_{ 0 };
	};

} // namespace mediasoup
