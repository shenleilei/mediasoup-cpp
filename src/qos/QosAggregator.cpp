#include "qos/QosAggregator.h"
#include <algorithm>

namespace mediasoup::qos {

PeerQosAggregate QosAggregator::Aggregate(const QosRegistry::Entry* entry,
	int64_t nowMs, int64_t staleAfterMs, int64_t lostAfterMs)
{
	PeerQosAggregate aggregate;
	if (!entry) return aggregate;

	aggregate.hasSnapshot = true;
	aggregate.receivedAtMs = entry->receivedAtMs;
	aggregate.lastUpdatedMs = entry->snapshot.tsMs;
	aggregate.ageMs = std::max<int64_t>(0, nowMs - entry->receivedAtMs);
	aggregate.stale = (aggregate.ageMs > staleAfterMs);
	aggregate.lost = (aggregate.ageMs > lostAfterMs);
	aggregate.quality = aggregate.lost ? "lost" : entry->snapshot.peerQuality;

	aggregate.data = {
		{"mode", entry->snapshot.peerMode},
		{"quality", aggregate.quality},
		{"stale", aggregate.stale},
		{"lost", aggregate.lost},
		{"seq", entry->snapshot.seq},
		{"tsMs", entry->snapshot.tsMs},
		{"receivedAtMs", entry->receivedAtMs},
		{"ageMs", aggregate.ageMs},
		{"tracks", entry->snapshot.raw.value("tracks", json::array())}
	};

	return aggregate;
}

} // namespace mediasoup::qos
