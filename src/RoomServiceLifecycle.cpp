#include "RoomService.h"

#include "RoomCleanupHelpers.h"
#include "RoomStatsQosHelpers.h"

namespace mediasoup {

RoomService::Result RoomService::join(const std::string& roomId, const std::string& peerId,
	const std::string& displayName, const json& rtpCapabilities, const std::string& clientIp,
	const std::string& audioRole)
{
	(void)clientIp;
	MS_INFO(logger_, "[{} {}] join start displayName={} clientIp={} audioRole={} rtpCapabilities={}",
		roomId, peerId, displayName, clientIp, audioRole, rtpCapabilities.dump());

	if (audioRole != "normal" && audioRole != "audio-restricted") {
		MS_WARN(logger_, "[{} {}] join validation failed: invalid audioRole={}",
			roomId, peerId, audioRole);
		return {false, {}, "", "invalid audioRole"};
	}

	auto existingRoom = roomManager_.getRoom(roomId);
	if (!existingRoom) {
		size_t maxRooms = roomManager_.workerManager().maxTotalRouters();
		if (maxRooms > 0 && roomManager_.roomCount() >= maxRooms) {
			MS_WARN(logger_, "[{} {}] local node at capacity ({}/{})", roomId, peerId,
				roomManager_.roomCount(), maxRooms);
		return {false, {}, "", "no available capacity"};
	}
	}

	if (!rtpCapabilities.is_null() && !rtpCapabilities.empty() && !rtpCapabilities.is_object()) {
		MS_WARN(logger_, "[{} {}] join validation failed: invalid rtpCapabilities type", roomId, peerId);
		return {false, {}, "", "invalid rtpCapabilities"};
	}

	bool roomCreated = (existingRoom == nullptr);
	auto room = roomManager_.createRoom(roomId);
	if (roomCreated && roomLifecycle_) {
		roomLifecycle_(roomId, true);
		MS_INFO(logger_, "[{} system] room created", roomId);
	}
	auto peer = std::make_shared<Peer>();
	peer->id = peerId;
	peer->displayName = displayName;
	peer->audioRole = audioRole == "audio-restricted"
		? Peer::AudioRole::AudioRestricted
		: Peer::AudioRole::Normal;
	if (!rtpCapabilities.empty()) {
		peer->rtpCapabilities = rtpCapabilities.get<RtpCapabilities>();
	} else {
		peer->rtpCapabilities = room->router()->rtpCapabilities();
	}

	auto oldPeer = room->replacePeer(peer);
	bool isReconnect = false;
	if (oldPeer) {
		isReconnect = true;
		for (const auto& targetPeerId : room->removeAudioRestrictedSlotsForOwner(peerId)) {
			closeAudioConsumersForSlot(roomId, room, peerId, targetPeerId);
		}
		std::string oldOwnerPeerId;
		if (room->removeAudioRestrictedSlotForTarget(peerId, oldOwnerPeerId)) {
			closeAudioConsumersForSlot(roomId, room, oldOwnerPeerId, peerId);
		}
		const auto oldPeerProducers = oldPeer->producers;
		for (auto& [pid, _] : oldPeerProducers)
			room->router()->removeProducer(pid);
		cleanupPeerProducerOwnerCache(roomId, oldPeerProducers);
		cleanupPeerProducerDemandCache(roomId, oldPeerProducers);
		oldPeer->close();

		qosRegistry_.ErasePeer(roomId, peerId);
		roomstatsqos::ClearPeerAutomaticOverrideRecords(
			autoQosOverrideRecords_,
			roomId,
			peerId);
		lastConnectionQualitySignatures_.erase(roomstatsqos::MakePeerKey(roomId, peerId));
		downlinkQosRegistry_.ErasePeer(roomId, peerId);
		subscriberControllers_.erase(roomstatsqos::MakePeerKey(roomId, peerId));
		cleanupPeerTrackQosOverrides(roomId, peerId);
		markDownlinkRoomDirty(roomId);
	}

	json existingProducers = json::array();
	std::vector<std::string> targetPeers;
	for (auto& other : room->getOtherPeers(peerId)) {
		targetPeers.push_back(other->id);
		for (auto& [pid, prod] : other->producers) {
			if (!canConsumeProducerForPeer(room, peer, prod)) {
				MS_DEBUG(logger_, "[{} {}] join existingProducers skip unauthorized producer={} owner={} kind={}",
					roomId, peerId, prod->id(), other->id, prod->kind());
				continue;
			}
			json producerInfo = {
				{"producerId", prod->id()}, {"producerPeerId", other->id},
				{"kind", prod->kind()},
				{"appData", prod->appData()}
			};
			existingProducers.push_back(producerInfo);
		}
	}

	if (broadcast_) {
		MS_INFO(logger_, "[{} {}] notify peerJoined joinedPeerId={} displayName={} reconnect={} targetPeerCount={} targetPeers={} participantCount={}",
			roomId,
			peerId,
			peerId,
			displayName,
			isReconnect ? "true" : "false",
			targetPeers.size(),
			json(targetPeers).dump(),
			room->peerCount());
		broadcast_(roomId, peerId, {
			{"notification", true}, {"method", "peerJoined"},
			{"data", {{"peerId", peerId}, {"displayName", displayName},
				{"audioRole", audioRole}, {"reconnect", isReconnect}}}
		});
	}

	auto result = Result{true, {
		{"routerRtpCapabilities", room->router()->rtpCapabilities()},
		{"existingProducers", existingProducers},
		{"participants", room->getParticipants()},
		{"qosPolicy", getDefaultQosPolicy()},
		{"audioRole", audioRole}
	}};
	MS_INFO(logger_, "[{} {}] join done reconnect={} participants={}",
		roomId, peerId, isReconnect ? "true" : "false", room->peerCount());
	return result;
}

RoomService::Result RoomService::leave(const std::string& roomId, const std::string& peerId) {
	MS_INFO(logger_, "[{} {}] leave start", roomId, peerId);
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_INFO(logger_, "[{} {}] leave done room_missing=true", roomId, peerId);
		return {true, {}};
	}

	qosRegistry_.ErasePeer(roomId, peerId);
	roomstatsqos::ClearPeerAutomaticOverrideRecords(
		autoQosOverrideRecords_,
		roomId,
		peerId);
	lastConnectionQualitySignatures_.erase(roomstatsqos::MakePeerKey(roomId, peerId));
	downlinkQosRegistry_.ErasePeer(roomId, peerId);
	subscriberControllers_.erase(roomstatsqos::MakePeerKey(roomId, peerId));
	cleanupPeerTrackQosOverrides(roomId, peerId);

	auto peer = room->getPeer(peerId);
	if (peer) {
		for (const auto& targetPeerId : room->removeAudioRestrictedSlotsForOwner(peerId)) {
			closeAudioConsumersForSlot(roomId, room, peerId, targetPeerId);
		}
		std::string ownerPeerId;
		if (room->removeAudioRestrictedSlotForTarget(peerId, ownerPeerId)) {
			closeAudioConsumersForSlot(roomId, room, ownerPeerId, peerId);
		}
		cleanupPeerProducerOwnerCache(roomId, peer->producers);
		cleanupPeerProducerDemandCache(roomId, peer->producers);
		for (auto& [pid, _] : peer->producers)
			room->router()->removeProducer(pid);
	}

	room->removePeer(peerId);
	if (!room->empty())
		markDownlinkRoomDirty(roomId);

	if (broadcast_) {
		const auto remainingPeers = room->getPeerIds();
		MS_INFO(logger_, "[{} {}] notify peerLeft leftPeerId={} targetPeerCount={} targetPeers={} participantCount={}",
			roomId,
			peerId,
			peerId,
			remainingPeers.size(),
			json(remainingPeers).dump(),
			room->peerCount());
		broadcast_(roomId, peerId, {
			{"notification", true}, {"method", "peerLeft"},
			{"data", {{"peerId", peerId}}}
		});
	}

	if (room->empty()) {
		roomManager_.removeRoom(roomId);
		if (roomLifecycle_) roomLifecycle_(roomId, false);
	}
	MS_INFO(logger_, "[{} {}] leave done room_empty={}", roomId, peerId, room->empty() ? "true" : "false");
	return {true, {}};
}

bool RoomService::leaveIfSessionMatches(
	const std::string& roomId,
	const std::string& peerId,
	uint64_t expectedSessionId)
{
	if (expectedSessionId == 0) {
		MS_DEBUG(logger_, "[{} {}] leave skipped: expectedSessionId=0", roomId, peerId);
		return false;
	}

	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_DEBUG(logger_, "[{} {}] leave skipped: room not found expectedSession={}",
			roomId, peerId, expectedSessionId);
		return false;
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_DEBUG(logger_, "[{} {}] leave skipped: peer not found expectedSession={}",
			roomId, peerId, expectedSessionId);
		return false;
	}
	if (peer->sessionId != expectedSessionId) {
		MS_DEBUG(logger_, "[{} {}] leave skipped: session mismatch expected={} actual={}",
			roomId, peerId, expectedSessionId, peer->sessionId);
		return false;
	}

	auto result = leave(roomId, peerId);
	return result.ok;
}

void RoomService::checkRoomHealth() {
	auto deadRooms = roomManager_.getDeadRooms();
	if (deadRooms.empty()) return;

	MS_WARN(logger_, "[system] checkRoomHealth: found {} dead rooms", deadRooms.size());
	for (auto& roomId : deadRooms) {
		auto room = roomManager_.getRoom(roomId);
		if (!room) continue;

		MS_WARN(logger_, "[{} system] dead router, notifying {} peers to reconnect",
			roomId, room->getPeerIds().size());

		if (broadcast_) {
			const auto targetPeers = room->getPeerIds();
			MS_INFO(logger_, "[{} system] notify serverRestart reason=worker crashed targetPeerCount={} targetPeers={}",
				roomId, targetPeers.size(), json(targetPeers).dump());
			broadcast_(roomId, "", {
				{"notification", true}, {"method", "serverRestart"},
				{"data", {{"roomId", roomId}, {"reason", "worker crashed"}}}
			});
		}

		destroyRoom(roomId);
	}
}

void RoomService::cleanIdleRooms(int idleSeconds) {
	for (auto& id : roomManager_.getIdleRooms(idleSeconds)) {
		MS_DEBUG(logger_, "[{} system] GC idle room", id);
		destroyRoom(id);
	}
}

void RoomService::closeAllRooms() {
	auto roomIds = roomManager_.getRoomIds();
	for (auto& roomId : roomIds) {
		destroyRoom(roomId);
	}
}

void RoomService::cleanupRoomResources(const std::string& roomId) {
	roomcleanup::CleanupRoomServiceState(
		roomId,
		qosRegistry_,
		downlinkQosRegistry_,
		subscriberControllers_,
		autoQosOverrideRecords_,
		lastConnectionQualitySignatures_,
		lastRoomQosStateSignatures_,
		lastStatsReportProducerScores_,
		dirtyDownlinkRooms_,
		pendingDownlinkRooms_,
		downlinkRoomPlanStates_,
		trackQosOverrideRecords_,
		producerDemandCache_,
		producerOwnerPeerIds_);
}

void RoomService::destroyRoom(const std::string& roomId) {
	cleanupRoomResources(roomId);
	bool removed = roomManager_.removeRoom(roomId);
	MS_INFO(logger_, "[{} system] room destroyed removed={}", roomId, removed ? "true" : "false");
	if (roomLifecycle_) roomLifecycle_(roomId, false);
}

} // namespace mediasoup
