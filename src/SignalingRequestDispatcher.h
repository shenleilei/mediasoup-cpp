#pragma once

#include "RoomService.h"
#include <stdexcept>
#include <string>
#include <vector>

namespace mediasoup::signaldispatch {

struct JoinRequestContext {
	std::string roomId;
	std::string peerId;
	std::string displayName;
	std::string clientIp;
	json rtpCapabilities = json::object();
};

inline bool IsValidJoinId(const std::string& value)
{
	if (value.empty() || value.size() > 128) {
		return false;
	}

	for (char c : value) {
		if (!((c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') ||
			c == '_' || c == '-')) {
			return false;
		}
	}

	return true;
}

inline bool BuildJoinRequestContext(
	const json& data,
	const std::string& remoteAddress,
	JoinRequestContext& joinRequest,
	std::string& error)
{
	joinRequest.roomId = data.value("roomId", "default");
	joinRequest.peerId = data.value("peerId", "");
	if (!IsValidJoinId(joinRequest.roomId) || !IsValidJoinId(joinRequest.peerId)) {
		error = "invalid roomId or peerId (allowed: [A-Za-z0-9_-]{1,128})";
		return false;
	}

	joinRequest.displayName = data.value("displayName", joinRequest.peerId);
	joinRequest.rtpCapabilities = data.value("rtpCapabilities", json::object());
	joinRequest.clientIp = remoteAddress;

	return true;
}

inline std::vector<uint32_t> ParseVideoSsrcs(const json& data)
{
	std::vector<uint32_t> videoSsrcs;
	if (data.contains("videoSsrcs") && data.at("videoSsrcs").is_array()) {
		for (const auto& value : data.at("videoSsrcs")) {
			videoSsrcs.push_back(value.get<uint32_t>());
		}
	}
	if (videoSsrcs.empty()) {
		videoSsrcs.push_back(data.value("videoSsrc", 1111u));
	}

	return videoSsrcs;
}

inline RoomService::Result DispatchRoomServiceRequest(
	RoomService& roomService,
	const std::string& method,
	const std::string& roomId,
	const std::string& peerId,
	const json& data,
	const JoinRequestContext* joinRequest,
	uint64_t newSessionId)
{
	if (method == "join") {
		if (!joinRequest) {
			throw std::invalid_argument("missing join request context");
		}

		auto result = roomService.join(
			joinRequest->roomId,
			joinRequest->peerId,
			joinRequest->displayName,
			joinRequest->rtpCapabilities,
			joinRequest->clientIp);
		if (result.ok && result.redirect.empty()) {
			auto room = roomService.getRoom(joinRequest->roomId);
			if (room) {
				auto peer = room->getPeer(joinRequest->peerId);
				if (peer) {
					peer->sessionId = newSessionId;
				}
			}
		}
		return result;
	}
	if (method == "createWebRtcTransport") {
		return roomService.createTransport(
			roomId, peerId,
			data.value("producing", false),
			data.value("consuming", false));
	}
	if (method == "connectWebRtcTransport") {
		return roomService.connectTransport(
			roomId, peerId,
			data.at("transportId").get<std::string>(),
			data.at("dtlsParameters").get<DtlsParameters>());
	}
	if (method == "produce") {
		return roomService.produce(
			roomId, peerId,
			data.at("transportId").get<std::string>(),
			data.at("kind").get<std::string>(),
			data.at("rtpParameters"));
	}
	if (method == "consume") {
		return roomService.consume(
			roomId, peerId,
			data.at("transportId").get<std::string>(),
			data.at("producerId").get<std::string>(),
			data.at("rtpCapabilities"));
	}
	if (method == "pauseProducer") {
		return roomService.pauseProducer(
			roomId,
			data.at("producerId").get<std::string>());
	}
	if (method == "resumeProducer") {
		return roomService.resumeProducer(
			roomId,
			data.at("producerId").get<std::string>());
	}
	if (method == "restartIce") {
		return roomService.restartIce(
			roomId, peerId,
			data.at("transportId").get<std::string>());
	}
	if (method == "getStats") {
		json stats = roomService.collectPeerStats(
			roomId,
			data.value("peerId", peerId));
		return { true, stats };
	}
	if (method == "setQosOverride") {
		return roomService.setQosOverride(
			roomId,
			peerId,
			data.value("peerId", peerId),
			data.at("override"));
	}
	if (method == "setQosPolicy") {
		return roomService.setQosPolicy(
			roomId,
			peerId,
			data.value("peerId", peerId),
			data.at("policy"));
	}
	if (method == "pauseConsumer") {
		return roomService.pauseConsumer(
			roomId, peerId,
			data.at("consumerId").get<std::string>());
	}
	if (method == "resumeConsumer") {
		return roomService.resumeConsumer(
			roomId, peerId,
			data.at("consumerId").get<std::string>());
	}
	if (method == "getConsumerState") {
		return roomService.getConsumerState(
			roomId, peerId,
			data.at("consumerId").get<std::string>());
	}
	if (method == "setConsumerPreferredLayers") {
		return roomService.setConsumerPreferredLayers(
			roomId, peerId,
			data.at("consumerId").get<std::string>(),
			data.at("spatialLayer").get<uint8_t>(),
			data.at("temporalLayer").get<uint8_t>());
	}
	if (method == "setConsumerPriority") {
		return roomService.setConsumerPriority(
			roomId, peerId,
			data.at("consumerId").get<std::string>(),
			data.at("priority").get<uint8_t>());
	}
	if (method == "requestConsumerKeyFrame") {
		return roomService.requestConsumerKeyFrame(
			roomId, peerId,
			data.at("consumerId").get<std::string>());
	}
	if (method == "plainPublish") {
		return roomService.plainPublish(
			roomId,
			peerId,
			ParseVideoSsrcs(data),
			data.value("audioSsrc", 2222u));
	}
	if (method == "plainSubscribe") {
		return roomService.plainSubscribe(
			roomId, peerId,
			data.at("recvIp").get<std::string>(),
			data.at("recvPort").get<uint16_t>());
	}

	return { false, {}, "", "unknown method: " + method };
}

} // namespace mediasoup::signaldispatch
