#pragma once
#include "QosTypes.h"
#include <cmath>
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
		if (sorted[i].quality > pq) pq = sorted[i].quality;

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

inline uint32_t normalizeTrackBitrateTarget(const TrackBudgetRequest& request) {
	if (request.paused) return 0;
	uint32_t minBitrateBps = request.minBitrateBps;
	uint32_t maxBitrateBps = request.maxBitrateBps > 0
		? request.maxBitrateBps
		: std::max(request.desiredBitrateBps, minBitrateBps);
	if (maxBitrateBps < minBitrateBps) maxBitrateBps = minBitrateBps;
	uint32_t desiredBitrateBps = request.desiredBitrateBps > 0 ? request.desiredBitrateBps : maxBitrateBps;
	return std::clamp(desiredBitrateBps, minBitrateBps, maxBitrateBps);
}

inline double normalizeTrackWeight(const TrackBudgetRequest& request) {
	return std::isfinite(request.weight) && request.weight > 0.0 ? request.weight : 1.0;
}

inline PeerBudgetDecision allocatePeerTrackBudgets(
	uint32_t totalBudgetBps,
	const std::vector<TrackBudgetRequest>& requests)
{
	PeerBudgetDecision decision;
	decision.totalBudgetBps = totalBudgetBps;
	if (requests.empty() || totalBudgetBps == 0) {
		for (const auto& request : requests)
			decision.allocations.push_back({request.trackId, 0, !request.paused});
		return decision;
	}

	struct NormalizedRequest {
		size_t index = 0;
		int priority = 0;
		uint32_t minBitrateBps = 0;
		uint32_t targetBitrateBps = 0;
		double weight = 1.0;
	};

	std::vector<uint32_t> allocations(requests.size(), 0);
	std::vector<uint32_t> targets(requests.size(), 0);
	std::vector<NormalizedRequest> normalized;
	normalized.reserve(requests.size());

	for (size_t i = 0; i < requests.size(); ++i) {
		const auto& request = requests[i];
		if (request.paused) continue;
		uint32_t targetBitrateBps = normalizeTrackBitrateTarget(request);
		uint32_t minBitrateBps = std::min(request.minBitrateBps, targetBitrateBps);
		targets[i] = targetBitrateBps;
		normalized.push_back({
			i,
			sourcePriority(request.source),
			minBitrateBps,
			targetBitrateBps,
			normalizeTrackWeight(request),
		});
	}

	std::sort(normalized.begin(), normalized.end(), [&](const auto& left, const auto& right) {
		if (left.priority != right.priority) return left.priority < right.priority;
		if (left.weight != right.weight) return left.weight > right.weight;
		return requests[left.index].trackId < requests[right.index].trackId;
	});

	uint32_t remainingBudgetBps = totalBudgetBps;
	for (const auto& request : normalized) {
		if (remainingBudgetBps == 0) break;
		uint32_t granted = std::min(request.minBitrateBps, remainingBudgetBps);
		allocations[request.index] += granted;
		remainingBudgetBps -= granted;
	}

	auto distributeTier = [&](int priority) {
		while (remainingBudgetBps > 0) {
			std::vector<size_t> active;
			active.reserve(normalized.size());
			double totalWeight = 0.0;
			for (const auto& request : normalized) {
				if (request.priority != priority) continue;
				if (allocations[request.index] >= request.targetBitrateBps) continue;
				active.push_back(request.index);
				totalWeight += normalizeTrackWeight(requests[request.index]);
			}
			if (active.empty() || totalWeight <= 0.0) break;

			struct GrantCandidate {
				size_t index = 0;
				uint32_t grant = 0;
				double fractional = 0.0;
			};

			const uint32_t tierRemainingBudgetBps = remainingBudgetBps;
			std::vector<GrantCandidate> grants;
			grants.reserve(active.size());
			uint32_t plannedBudgetBps = 0;

			for (auto index : active) {
				const uint32_t headroomBps = targets[index] - allocations[index];
				double exactGrant = static_cast<double>(tierRemainingBudgetBps)
					* normalizeTrackWeight(requests[index]) / totalWeight;
				uint32_t floorGrant = static_cast<uint32_t>(std::floor(exactGrant));
				uint32_t grant = std::min(headroomBps, floorGrant);
				grants.push_back({index, grant, exactGrant - std::floor(exactGrant)});
				plannedBudgetBps += grant;
			}

			for (auto& grant : grants) {
				allocations[grant.index] += grant.grant;
			}
			remainingBudgetBps -= plannedBudgetBps;

			std::sort(grants.begin(), grants.end(), [&](const auto& left, const auto& right) {
				if (left.fractional != right.fractional) return left.fractional > right.fractional;
				return requests[left.index].trackId < requests[right.index].trackId;
			});

			bool grantedExtra = false;
			for (auto& grant : grants) {
				if (remainingBudgetBps == 0) break;
				if (allocations[grant.index] >= targets[grant.index]) continue;
				allocations[grant.index] += 1;
				remainingBudgetBps -= 1;
				grantedExtra = true;
			}

			if (plannedBudgetBps == 0 && !grantedExtra) break;
		}
	};

	distributeTier(sourcePriority(Source::Audio));
	distributeTier(sourcePriority(Source::ScreenShare));
	distributeTier(sourcePriority(Source::Camera));

	uint32_t allocatedBudgetBps = 0;
	for (size_t i = 0; i < requests.size(); ++i) {
		allocatedBudgetBps += allocations[i];
		decision.allocations.push_back({
			requests[i].trackId,
			allocations[i],
			allocations[i] < targets[i]
		});
	}
	decision.allocatedBudgetBps = allocatedBudgetBps;
	return decision;
}

inline std::map<std::string, CoordinationOverride> buildCoordinationOverrides(
	const std::vector<PeerTrackState>& tracks,
	const PeerBudgetDecision& budgetDecision)
{
	std::map<std::string, CoordinationOverride> overrides;
	if (tracks.empty()) return overrides;

	const auto peerDecision = buildPeerDecision(tracks);

	std::map<std::string, uint32_t> allocationsByTrackId;
	for (const auto& allocation : budgetDecision.allocations)
		allocationsByTrackId[allocation.trackId] = allocation.allocatedBitrateBps;

	for (const auto& track : tracks) {
		CoordinationOverride override;
		const bool isSacrificial = std::find(
			peerDecision.sacrificialTrackIds.begin(),
			peerDecision.sacrificialTrackIds.end(),
			track.trackId) != peerDecision.sacrificialTrackIds.end();
		const uint32_t allocatedBitrateBps = allocationsByTrackId.count(track.trackId) > 0
			? allocationsByTrackId[track.trackId]
			: 0;

		if (track.source != Source::Audio && !peerDecision.allowVideoRecovery)
			override.disableRecovery = true;
		if (track.source == Source::Camera && isSacrificial
			&& (peerDecision.peerQuality == Quality::Poor || peerDecision.peerQuality == Quality::Lost))
			override.forceAudioOnly = true;
		if (track.source == Source::Camera && isSacrificial && peerDecision.preferScreenShare)
			override.disableRecovery = true;
		if (allocatedBitrateBps > 0)
			override.maxBitrateCapBps = allocatedBitrateBps;
		if (track.source == Source::Camera && allocatedBitrateBps == 0)
			override.forceAudioOnly = true;
		if (track.source == Source::ScreenShare && allocatedBitrateBps == 0)
			override.pauseUpstream = true;
		if (track.source == Source::ScreenShare && allocatedBitrateBps > 0 && track.level >= 4)
			override.resumeUpstream = true;

		if (override.maxLevelClamp.has_value() || override.disableRecovery.has_value()
			|| override.forceAudioOnly.has_value() || override.pauseUpstream.has_value()
			|| override.resumeUpstream.has_value() || override.maxBitrateCapBps.has_value()) {
			overrides[track.trackId] = override;
		}
	}

	return overrides;
}

class PeerQosCoordinator {
public:
	void upsertTrack(const PeerTrackState& track) {
		tracks_[track.trackId] = track;
	}

	void removeTrack(const std::string& trackId) {
		tracks_.erase(trackId);
	}

	void clear() {
		tracks_.clear();
	}

	std::vector<PeerTrackState> getTracks() const {
		std::vector<PeerTrackState> tracks;
		tracks.reserve(tracks_.size());
		for (const auto& entry : tracks_) tracks.push_back(entry.second);
		return tracks;
	}

	PeerDecision getDecision() const {
		return buildPeerDecision(getTracks());
	}

private:
	std::map<std::string, PeerTrackState> tracks_;
};

} // namespace qos
