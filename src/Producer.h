#pragma once
#include "Channel.h"
#include "Constants.h"
#include "RtpTypes.h"
#include "EventEmitter.h"
#include "Logger.h"
#include <string>
#include <memory>

namespace mediasoup {

class Producer {
public:
	Producer(const std::string& id, const std::string& kind,
		const RtpParameters& rtpParameters, const std::string& type,
		const RtpParameters& consumableRtpParameters,
		Channel* channel, const std::string& transportId)
		: id_(id), kind_(kind), rtpParameters_(rtpParameters), type_(type)
		, consumableRtpParameters_(consumableRtpParameters)
		, channel_(channel), transportId_(transportId)
		, logger_(Logger::Get("Producer")) {}

	const std::string& id() const { return id_; }
	const std::string& kind() const { return kind_; }
	const RtpParameters& rtpParameters() const { return rtpParameters_; }
	const std::string& type() const { return type_; }
	const RtpParameters& consumableRtpParameters() const { return consumableRtpParameters_; }
	bool closed() const { return closed_; }
	bool paused() const { return paused_; }
	EventEmitter& emitter() { return emitter_; }

	void pause();
	void resume();
	void close();
	void transportClosed();
	void handleNotification(FBS::Notification::Event event,
		const FBS::Notification::Notification* notification);
	json getStats(int timeoutMs = kChannelRequestTimeoutMs);

	json toJson() const {
		return {{"id", id_}, {"kind", kind_}, {"type", type_}, {"paused", paused_}};
	}

	struct ScoreEntry { uint32_t encodingIdx; uint32_t ssrc; std::string rid; uint8_t score; };
	const std::vector<ScoreEntry>& scores() const { return scores_; }

private:
	std::string id_;
	std::string kind_;
	RtpParameters rtpParameters_;
	std::string type_;
	RtpParameters consumableRtpParameters_;
	Channel* channel_;
	std::string transportId_;
	bool closed_ = false;
	bool paused_ = false;
	std::vector<ScoreEntry> scores_;
	EventEmitter emitter_;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
