#include "RoomService.h"

#include "RoomCleanupHelpers.h"
#include "RoomRecordingHelpers.h"
#include "RoomRegistry.h"
#include "RoomStatsQosHelpers.h"

namespace mediasoup {

RoomService::Result RoomService::join(const std::string& roomId, const std::string& peerId,
	const std::string& displayName, const json& rtpCapabilities, const std::string& clientIp)
{
	if (registry_) {
		try {
			std::string addr = registry_->claimRoom(roomId, clientIp);
			if (!addr.empty()) return {false, {}, addr, ""};
		} catch (const std::exception& e) {
			MS_WARN(logger_, "[{} {}] claimRoom failed ({}), degrading to local", roomId, peerId, e.what());
		}
	}

	auto existingRoom = roomManager_.getRoom(roomId);
	if (!existingRoom) {
		size_t maxRooms = roomManager_.workerManager().maxTotalRouters();
		if (maxRooms > 0 && roomManager_.roomCount() >= maxRooms) {
			MS_WARN(logger_, "[{} {}] local node at capacity ({}/{})", roomId, peerId,
				roomManager_.roomCount(), maxRooms);
			if (registry_) {
				try {
					registry_->unregisterRoom(roomId);
					auto resolved = registry_->resolveRoom(roomId, clientIp);
					if (!resolved.wsUrl.empty() && resolved.wsUrl != registry_->nodeAddress())
						return {false, {}, resolved.wsUrl, ""};
				} catch (...) {}
			}
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
	}
	auto peer = std::make_shared<Peer>();
	peer->id = peerId;
	peer->displayName = displayName;
	if (!rtpCapabilities.empty())
		peer->rtpCapabilities = rtpCapabilities.get<RtpCapabilities>();

	auto oldPeer = room->replacePeer(peer);
	bool isReconnect = false;
	if (oldPeer) {
		isReconnect = true;
		const auto oldPeerProducers = oldPeer->producers;
		for (auto& [pid, _] : oldPeerProducers)
			room->router()->removeProducer(pid);
		cleanupPeerProducerOwnerCache(roomId, oldPeerProducers);
		cleanupPeerProducerDemandCache(roomId, oldPeerProducers);
		oldPeer->close();

		std::string key = roomstatsqos::MakePeerKey(roomId, peerId);
		roomrecording::RemovePeerRecorder(key, recorders_, recorderTransports_, logger_);
		qosRegistry_.ErasePeer(roomId, peerId);
		roomstatsqos::ClearPeerAutomaticOverrideRecords(
			autoQosOverrideRecords_,
			roomId,
			peerId);
		lastConnectionQualitySignatures_.erase(key);
		downlinkQosRegistry_.ErasePeer(roomId, peerId);
		subscriberControllers_.erase(key);
		cleanupPeerTrackQosOverrides(roomId, peerId);
		markDownlinkRoomDirty(roomId);
	}

	if (registry_) {
		auto* reg = registry_;
		std::string rid = roomId;
		postRegistryTask([reg, rid] { reg->refreshRoom(rid); });
	}

	json existingProducers = json::array();
	for (auto& other : room->getOtherPeers(peerId)) {
		for (auto& [pid, prod] : other->producers) {
			existingProducers.push_back({
				{"producerId", prod->id()}, {"producerPeerId", other->id},
				{"kind", prod->kind()}
			});
		}
	}

	if (broadcast_) {
		broadcast_(roomId, peerId, {
			{"notification", true}, {"method", "peerJoined"},
			{"data", {{"peerId", peerId}, {"displayName", displayName},
				{"reconnect", isReconnect}}}
		});
	}

	return {true, {
		{"routerRtpCapabilities", room->router()->rtpCapabilities()},
		{"existingProducers", existingProducers},
		{"participants", room->getParticipants()},
		{"qosPolicy", getDefaultQosPolicy()}
	}};
}

RoomService::Result RoomService::leave(const std::string& roomId, const std::string& peerId) {
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {true, {}};

	std::string key = roomstatsqos::MakePeerKey(roomId, peerId);
	roomrecording::RemovePeerRecorder(key, recorders_, recorderTransports_, logger_);

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
		cleanupPeerProducerOwnerCache(roomId, peer->producers);
		cleanupPeerProducerDemandCache(roomId, peer->producers);
		for (auto& [pid, _] : peer->producers)
			room->router()->removeProducer(pid);
	}

	room->removePeer(peerId);
	if (!room->empty())
		markDownlinkRoomDirty(roomId);

	if (broadcast_) {
		broadcast_(roomId, peerId, {
			{"notification", true}, {"method", "peerLeft"},
			{"data", {{"peerId", peerId}}}
		});
	}

	if (room->empty()) {
		if (registry_) {
			auto* reg = registry_;
			std::string rid = roomId;
			postRegistryTask([reg, rid] { reg->unregisterRoom(rid); });
		}
		roomManager_.removeRoom(roomId);
		if (roomLifecycle_) roomLifecycle_(roomId, false);
	}
	return {true, {}};
}

RoomService::Result RoomService::leaveIfSessionMatches(
	const std::string& roomId,
	const std::string& peerId,
	uint64_t expectedSessionId)
{
	if (expectedSessionId == 0) {
		return {true, {}};
	}

	auto room = roomManager_.getRoom(roomId);
	if (!room) return {true, {}};
	auto peer = room->getPeer(peerId);
	if (!peer) return {true, {}};
	if (peer->sessionId != expectedSessionId) {
		return {true, {}};
	}

	return leave(roomId, peerId);
}

void RoomService::checkRoomHealth() {
	auto deadRooms = roomManager_.getDeadRooms();
	if (deadRooms.empty()) return;

	MS_WARN(logger_, "checkRoomHealth: found {} dead rooms", deadRooms.size());
	for (auto& roomId : deadRooms) {
		auto room = roomManager_.getRoom(roomId);
		if (!room) continue;

		MS_WARN(logger_, "Room {} has dead router, notifying {} peers to reconnect",
			roomId, room->getPeerIds().size());

		if (broadcast_) {
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
		MS_DEBUG(logger_, "GC idle room: {}", id);
		destroyRoom(id);
	}
	cleanOldRecordings();
}

void RoomService::closeAllRooms() {
	auto roomIds = roomManager_.getRoomIds();
	for (auto& roomId : roomIds) {
		destroyRoom(roomId);
	}
}

void RoomService::cleanupRoomResources(const std::string& roomId) {
	roomrecording::CleanupRoomRecorders(roomId, recorders_, recorderTransports_, logger_);
	roomcleanup::CleanupRoomServiceState(
		roomId,
		qosRegistry_,
		downlinkQosRegistry_,
		subscriberControllers_,
		autoQosOverrideRecords_,
		lastConnectionQualitySignatures_,
		lastRoomQosStateSignatures_,
		dirtyDownlinkRooms_,
		pendingDownlinkRooms_,
		downlinkRoomPlanStates_,
		trackQosOverrideRecords_,
		producerDemandCache_,
		producerOwnerPeerIds_);
}

void RoomService::destroyRoom(const std::string& roomId) {
	cleanupRoomResources(roomId);
	if (registry_) {
		auto* reg = registry_;
		std::string rid = roomId;
		postRegistryTask([reg, rid] { reg->unregisterRoom(rid); });
	}
	roomManager_.removeRoom(roomId);
	if (roomLifecycle_) roomLifecycle_(roomId, false);
}

void RoomService::cleanOldRecordings(uint64_t maxBytes) {
	roomrecording::CleanOldRecordingDirs(recordDir_, maxBytes, recorders_, logger_);
}

} // namespace mediasoup
