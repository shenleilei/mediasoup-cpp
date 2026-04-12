#include "qos/QosRoomAggregator.h"

namespace mediasoup::qos {
namespace {

int QualityRank(const std::string& quality) {
	if (quality == "lost") return 0;
	if (quality == "poor") return 1;
	if (quality == "good") return 2;
	if (quality == "excellent") return 3;
	return -1;
}

std::string MinQuality(const std::string& lhs, const std::string& rhs) {
	return QualityRank(lhs) <= QualityRank(rhs) ? lhs : rhs;
}

} // namespace

RoomQosAggregate QosRoomAggregator::Aggregate(const std::vector<PeerQosAggregate>& peers) {
	RoomQosAggregate aggregate;
	aggregate.peerCount = peers.size();
	if (peers.empty()) return aggregate;

	bool hasQos = false;
	std::string quality = "excellent";
	json peerSummaries = json::array();
	for (const auto& peer : peers) {
		if (!peer.hasSnapshot) continue;
		hasQos = true;
		if (peer.stale) aggregate.stalePeers++;
		if (peer.quality == "poor") aggregate.poorPeers++;
		if (peer.quality == "lost" || peer.lost) aggregate.lostPeers++;
		quality = MinQuality(quality, peer.quality);
		peerSummaries.push_back(peer.data);
	}

	aggregate.hasQos = hasQos;
	if (!hasQos) {
		aggregate.quality = "unknown";
		return aggregate;
	}

	aggregate.quality = (aggregate.lostPeers > 0u) ? "lost" : quality;
	aggregate.data = {
		{"peerCount", aggregate.peerCount},
		{"stalePeers", aggregate.stalePeers},
		{"poorPeers", aggregate.poorPeers},
		{"lostPeers", aggregate.lostPeers},
		{"quality", aggregate.quality},
		{"peers", peerSummaries}
	};

	return aggregate;
}

} // namespace mediasoup::qos
