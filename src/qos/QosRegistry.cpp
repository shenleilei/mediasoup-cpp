#include "qos/QosRegistry.h"

namespace mediasoup::qos {

bool QosRegistry::Upsert(const std::string& roomId, const std::string& peerId,
	const ClientQosSnapshot& snapshot, int64_t receivedAtMs, std::string* rejectReason)
{
	auto& peers = rooms_[roomId];
	auto it = peers.find(peerId);
	if (it != peers.end() && snapshot.seq <= it->second.snapshot.seq) {
		if (rejectReason) *rejectReason = "stale-seq";
		return false;
	}

	peers[peerId] = Entry{ snapshot, receivedAtMs };
	return true;
}

const QosRegistry::Entry* QosRegistry::Get(
	const std::string& roomId, const std::string& peerId) const
{
	auto roomIt = rooms_.find(roomId);
	if (roomIt == rooms_.end()) return nullptr;
	auto peerIt = roomIt->second.find(peerId);
	if (peerIt == roomIt->second.end()) return nullptr;

	return &peerIt->second;
}

void QosRegistry::ErasePeer(const std::string& roomId, const std::string& peerId) {
	auto roomIt = rooms_.find(roomId);
	if (roomIt == rooms_.end()) return;
	roomIt->second.erase(peerId);
	if (roomIt->second.empty()) rooms_.erase(roomIt);
}

void QosRegistry::EraseRoom(const std::string& roomId) {
	rooms_.erase(roomId);
}

void QosRegistry::Clear() {
	rooms_.clear();
}

} // namespace mediasoup::qos
