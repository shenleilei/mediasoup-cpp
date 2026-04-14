#pragma once
#include "WebRtcTransport.h"
#include "Producer.h"
#include "Consumer.h"
#include "RtpTypes.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>

namespace mediasoup {

using json = nlohmann::json;

struct Peer {
	std::string id;
	std::string displayName;
	uint64_t sessionId = 0;
	RtpCapabilities rtpCapabilities;
	std::shared_ptr<WebRtcTransport> sendTransport;
	std::shared_ptr<WebRtcTransport> recvTransport;
	std::unordered_map<std::string, std::shared_ptr<Producer>> producers;
	std::unordered_map<std::string, std::shared_ptr<Consumer>> consumers;

	void close() {
		// Close transports first so mediasoup-worker tears down attached producers/consumers
		// in one path, which avoids double-close noise during disconnect cleanup.
		if (sendTransport) { sendTransport->close(); sendTransport.reset(); }
		if (recvTransport) { recvTransport->close(); recvTransport.reset(); }
		for (auto& [_, p] : producers)
			if (p && !p->closed()) p->close();
		producers.clear();
		for (auto& [_, c] : consumers)
			if (c && !c->closed()) c->close();
		consumers.clear();
	}

	std::shared_ptr<Transport> getTransport(const std::string& tid) {
		if (sendTransport && sendTransport->id() == tid) return sendTransport;
		if (recvTransport && recvTransport->id() == tid) return recvTransport;
		return nullptr;
	}

	json toJson() const {
		json prods = json::array();
		for (auto& [_, p] : producers)
			prods.push_back({{"producerId", p->id()}, {"kind", p->kind()}});
		return {{"peerId", id}, {"displayName", displayName}, {"producers", prods}};
	}
};

} // namespace mediasoup
