#include "qos/DownlinkQosRegistry.h"

namespace mediasoup::qos {

bool DownlinkQosRegistry::Upsert(const std::string& roomId, const std::string& peerId,
	const DownlinkSnapshot& snapshot, int64_t receivedAtMs, std::string* rejectReason)
{
	auto& peers = rooms_[roomId];
	auto it = peers.find(peerId);
	if (it != peers.end()) {
		const auto prevSeq = it->second.snapshot.seq;
		if (snapshot.seq <= prevSeq) {
			constexpr uint64_t kSeqResetThreshold = 1000u;
			if (prevSeq - snapshot.seq <= kSeqResetThreshold) {
				if (rejectReason) *rejectReason = "stale-seq";
				return false;
			}
		}
	}
	peers[peerId] = Entry{ snapshot, receivedAtMs };
	return true;
}

const DownlinkQosRegistry::Entry* DownlinkQosRegistry::Get(
	const std::string& roomId, const std::string& peerId) const
{
	auto roomIt = rooms_.find(roomId);
	if (roomIt == rooms_.end()) return nullptr;
	auto peerIt = roomIt->second.find(peerId);
	if (peerIt == roomIt->second.end()) return nullptr;
	return &peerIt->second;
}

void DownlinkQosRegistry::ErasePeer(const std::string& roomId, const std::string& peerId) {
	auto roomIt = rooms_.find(roomId);
	if (roomIt == rooms_.end()) return;
	roomIt->second.erase(peerId);
	if (roomIt->second.empty()) rooms_.erase(roomIt);
}

void DownlinkQosRegistry::EraseRoom(const std::string& roomId) {
	rooms_.erase(roomId);
}

} // namespace mediasoup::qos
