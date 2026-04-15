#pragma once
#include "QosTypes.h"
#include <nlohmann/json.hpp>

namespace qos {

using json = nlohmann::json;

inline json serializeSnapshot(int seq, int64_t tsMs, Quality peerQuality, bool stale,
	const std::vector<PeerTrackState>& tracks, const std::map<std::string, DerivedSignals>& trackSignals,
	const std::map<std::string, PlannedAction>& lastActions)
{
	json tracksArr = json::array();
	for (auto& t : tracks) {
		json tj = {
			{"localTrackId", t.trackId}, {"kind", t.kind == TrackKind::Video ? "video" : "audio"},
			{"source", sourceStr(t.source)}, {"state", stateStr(t.state)},
			{"quality", qualityStr(t.quality)}, {"ladderLevel", t.level},
		};
		json sig = {};
		auto sit = trackSignals.find(t.trackId);
		if (sit != trackSignals.end()) {
			sig["sendBitrateBps"] = sit->second.sendBitrateBps;
			sig["targetBitrateBps"] = sit->second.targetBitrateBps;
			sig["lossRate"] = sit->second.lossRate;
			sig["rttMs"] = sit->second.rttMs;
			sig["jitterMs"] = sit->second.jitterMs;
		}
		tj["signals"] = sig;
		auto ait = lastActions.find(t.trackId);
		if (ait != lastActions.end())
			tj["lastAction"] = {{"type", actionStr(ait->second.type)}, {"applied", true}};
		tracksArr.push_back(tj);
	}

	return {
		{"schema", "mediasoup.qos.client.v1"}, {"seq", seq}, {"tsMs", tsMs},
		{"peerState", {
			{"mode", tracks.empty() || tracks[0].kind == TrackKind::Audio ? "audio-only" : "audio-video"},
			{"quality", qualityStr(peerQuality)}, {"stale", stale}
		}},
		{"tracks", tracksArr}
	};
}

} // namespace qos
