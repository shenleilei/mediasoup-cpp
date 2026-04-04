#pragma once
#include "Channel.h"
#include "RtpTypes.h"
#include "ortc.h"
#include "EventEmitter.h"
#include "Logger.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace mediasoup {

class Transport;
class WebRtcTransport;
class Producer;

struct WebRtcTransportOptions {
	std::vector<json> listenInfos;
	bool enableUdp = true;
	bool enableTcp = false;
	bool preferUdp = true;
	bool preferTcp = false;
	uint32_t initialAvailableOutgoingBitrate = 600000;
	bool enableSctp = false;
	uint8_t iceConsentTimeout = 30;
};

class Router : public std::enable_shared_from_this<Router> {
public:
	Router(const std::string& id, Channel* channel,
		const std::vector<json>& mediaCodecs = {});

	const std::string& id() const { return id_; }
	bool closed() const { return closed_; }
	const RtpCapabilities& rtpCapabilities() const { return rtpCapabilities_; }
	Channel* channel() const { return channel_; }
	EventEmitter& emitter() { return emitter_; }

	std::shared_ptr<WebRtcTransport> createWebRtcTransport(const WebRtcTransportOptions& options);

	std::shared_ptr<Producer> getProducerById(const std::string& id) const;
	void addProducer(std::shared_ptr<Producer> producer);
	void removeProducer(const std::string& id);

	void close();
	void workerClosed();

	json toJson() const;

	const auto& transports() const { return transports_; }

private:
	std::string id_;
	Channel* channel_;
	bool closed_ = false;
	RtpCapabilities rtpCapabilities_;
	std::unordered_map<std::string, std::shared_ptr<Transport>> transports_;
	std::unordered_map<std::string, std::shared_ptr<Producer>> producers_;
	EventEmitter emitter_;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
