#pragma once
#include "Channel.h"
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

	json toJson() const {
		return {{"id", id_}, {"kind", kind_}, {"type", type_}, {"paused", paused_}};
	}

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
	EventEmitter emitter_;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
