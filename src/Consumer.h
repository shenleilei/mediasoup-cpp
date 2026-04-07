#pragma once
#include "Channel.h"
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

	const std::string& id() const { return id_; }
	const std::string& producerId() const { return producerId_; }
	const std::string& kind() const { return kind_; }
	const RtpParameters& rtpParameters() const { return rtpParameters_; }
	const std::string& type() const { return type_; }
	bool closed() const { return closed_; }
	bool paused() const { return paused_; }
	bool producerPaused() const { return producerPaused_; }
	EventEmitter& emitter() { return emitter_; }

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
			{"rtpParameters", rtpParameters_}};
	}

	void handleNotification(FBS::Notification::Event event, const FBS::Notification::Notification* notification);

	struct Score { uint8_t score = 0; uint8_t producerScore = 0; std::vector<uint8_t> producerScores; };
	const Score& currentScore() const { return score_; }
	json getStats();

private:
	std::string id_;
	std::string producerId_;
	std::string kind_;
	RtpParameters rtpParameters_;
	std::string type_;
	Channel* channel_;
	std::string transportId_;
	bool closed_ = false;
	bool paused_ = false;
	bool producerPaused_ = false;
	Score score_;
	EventEmitter emitter_;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
