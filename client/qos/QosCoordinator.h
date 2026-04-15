#pragma once
#include "QosTypes.h"
#include <vector>
#include <map>
#include <algorithm>

namespace qos {

inline int sourcePriority(Source s) {
	switch (s) { case Source::Audio: return 0; case Source::ScreenShare: return 1; case Source::Camera: return 2; }
	return 2;
}

inline PeerDecision buildPeerDecision(const std::vector<PeerTrackState>& tracks) {
	if (tracks.empty()) return {Quality::Lost, {}, {}, false, false, false};

	auto sorted = tracks;
	std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) {
		int pa = sourcePriority(a.source), pb = sourcePriority(b.source);
		return pa != pb ? pa < pb : a.trackId < b.trackId;
	});

	Quality pq = sorted[0].quality;
	for (size_t i = 1; i < sorted.size(); i++)
		if (sorted[i].quality < pq) pq = sorted[i].quality;

	bool hasAudio = false, hasScreen = false;
	for (auto& t : sorted) {
		if (t.source == Source::Audio) hasAudio = true;
		if (t.source == Source::ScreenShare) hasScreen = true;
	}

	std::vector<std::string> prioritized, sacrificial;
	for (auto& t : sorted) {
		prioritized.push_back(t.trackId);
		if (t.source != Source::Audio && (hasScreen ? t.source == Source::Camera : hasAudio))
			sacrificial.push_back(t.trackId);
	}

	bool allowRecovery = pq != Quality::Poor && pq != Quality::Lost;
	if (allowRecovery) {
		for (auto& t : sorted)
			if (t.source == Source::Audio && t.quality == Quality::Poor) { allowRecovery = false; break; }
	}

	return {pq, prioritized, sacrificial, hasAudio, hasScreen, allowRecovery};
}

} // namespace qos
