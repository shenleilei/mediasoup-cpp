#include "qos/SubscriberBudgetAllocator.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace mediasoup::qos {
namespace {

// Rough bitrate estimate per spatial layer (relative to base).
// S0 = base, S1 ~ 3x, S2 ~ 9x.  Temporal layers scale linearly within.
constexpr double kSpatialMultiplier[] = { 1.0, 3.0, 9.0 };
constexpr double kBaseBitrateBps = 100'000.0; // 100 kbps assumed base
constexpr double kScreenShareBaseBps = 300'000.0;
constexpr int kMaxSpatial = 2;
constexpr int kMaxTemporal = 2;
constexpr int64_t kMinDwellMs = 2'000;
constexpr int64_t kUpgradeCooldownMs = 3'000;
constexpr double kUpgradeHeadroomRatio = 0.10; // require 10% budget headroom for upgrades
constexpr double kSwitchPenalty = 0.25; // discourage needless oscillation

struct UpgradeStep {
	size_t subIdx;
	uint8_t spatialLayer;
	uint8_t temporalLayer;
	double deltaCost;
	double deltaUtility;
	double density; // deltaUtility / deltaCost
};

struct PrevLayerState {
	bool exists{ false };
	bool paused{ false };
	uint8_t spatial{ 0 };
	uint8_t temporal{ 0 };
	int64_t lastLayerChangeAtMs{ 0 };
	int64_t lastUpgradeAtMs{ 0 };
};

} // namespace

double SubscriberBudgetAllocator::computeBudgetBps(const DownlinkSnapshot& snapshot) const {
	double budget = kDefaultSafetyCap;
	if (snapshot.availableIncomingBitrate > 0.0)
		budget = std::min(budget, snapshot.availableIncomingBitrate * kAlpha);
	return budget;
}

double SubscriberBudgetAllocator::computeUtility(const DownlinkSubscription& sub) const {
	double base = 1.0;
	if (sub.isScreenShare)   base = 10.0;
	else if (sub.pinned)     base = 8.0;
	else if (sub.activeSpeaker) base = 6.0;
	else if (sub.visible)    base = 3.0;
	else                     base = 0.0; // hidden

	// Area modifier: larger display area -> higher utility
	double area = static_cast<double>(sub.targetWidth) * sub.targetHeight;
	if (area > 0.0) base *= std::min(std::sqrt(area / (320.0 * 240.0)), 3.0);

	// Freeze penalty
	if (sub.freezeRate > 0.1) base *= 0.7;

	return base;
}

double SubscriberBudgetAllocator::estimateLayerBitrateBps(
	const DownlinkSubscription& sub, uint8_t spatialLayer, uint8_t temporalLayer) const
{
	double baseBps = sub.isScreenShare ? kScreenShareBaseBps : kBaseBitrateBps;
	int s = std::min(static_cast<int>(spatialLayer), kMaxSpatial);
	int t = std::min(static_cast<int>(temporalLayer), kMaxTemporal);
	double spatialScale = kSpatialMultiplier[s];
	double temporalScale = (t + 1.0) / (kMaxTemporal + 1.0);
	return baseBps * spatialScale * temporalScale;
}

SubscriberBudgetPlan SubscriberBudgetAllocator::Allocate(
	const DownlinkSnapshot& snapshot,
	int degradeLevel,
	std::unordered_map<std::string, ConsumerLastState>* lastState,
	int64_t nowMs) const
{
	SubscriberBudgetPlan plan;
	plan.budgetBps = computeBudgetBps(snapshot);
	if (snapshot.subscriptions.empty()) return plan;

	const auto& subs = snapshot.subscriptions;
	size_t n = subs.size();

	// Current allocation: each visible sub starts at layer (0,0), hidden gets nothing.
	struct Alloc {
		uint8_t spatial{ 0 };
		uint8_t temporal{ 0 };
		bool active{ false };
		double utility{ 0.0 };
	};
	std::vector<Alloc> allocs(n);
	std::vector<PrevLayerState> prev(n);
	double usedBps = 0.0;

	// Compute utility for all subs first
	for (size_t i = 0; i < n; ++i)
		allocs[i].utility = computeUtility(subs[i]);
	for (size_t i = 0; i < n; ++i) {
		if (!lastState) continue;
		auto it = lastState->find(subs[i].consumerId);
		if (it == lastState->end()) continue;
		prev[i].exists = true;
		prev[i].paused = it->second.paused;
		prev[i].spatial = it->second.spatialLayer;
		prev[i].temporal = it->second.temporalLayer;
		prev[i].lastLayerChangeAtMs = it->second.lastLayerChangeAtMs;
		prev[i].lastUpgradeAtMs = it->second.lastUpgradeAtMs;
	}

	// Step 1: assign base layer in priority order (highest utility first)
	std::vector<size_t> order(n);
	std::iota(order.begin(), order.end(), 0);
	std::sort(order.begin(), order.end(),
		[&](size_t a, size_t b) { return allocs[a].utility > allocs[b].utility; });

	for (size_t idx : order) {
		if (allocs[idx].utility <= 0.0) continue;
		double cost = estimateLayerBitrateBps(subs[idx], 0, 0);
		if (usedBps + cost <= plan.budgetBps) {
			allocs[idx].active = true;
			usedBps += cost;
		}
	}

	// Step 2: greedy upgrade — pick best marginal utility/cost
	int maxSpatialCap = kMaxSpatial;
	int maxTemporalCap = kMaxTemporal;
	if (degradeLevel >= 3) { maxSpatialCap = 0; maxTemporalCap = 0; }
	else if (degradeLevel == 2) { maxSpatialCap = 1; maxTemporalCap = 1; }
	else if (degradeLevel == 1) { maxSpatialCap = 2; maxTemporalCap = 1; }

	// Step 1.5: try preserving prior layers first (state memory).
	// This avoids unnecessary down/up oscillation when budget hovers around thresholds.
	for (size_t idx : order) {
		if (!allocs[idx].active || !prev[idx].exists || prev[idx].paused) continue;
		uint8_t targetS = static_cast<uint8_t>(std::min(static_cast<int>(prev[idx].spatial), maxSpatialCap));
		uint8_t targetT = static_cast<uint8_t>(std::min(static_cast<int>(prev[idx].temporal), maxTemporalCap));
		if (targetS == allocs[idx].spatial && targetT == allocs[idx].temporal) continue;
		double newCost = estimateLayerBitrateBps(subs[idx], targetS, targetT);
		double oldCost = estimateLayerBitrateBps(subs[idx], allocs[idx].spatial, allocs[idx].temporal);
		double delta = newCost - oldCost;
		if (delta <= 0.0 || usedBps + delta > plan.budgetBps) continue;
		allocs[idx].spatial = targetS;
		allocs[idx].temporal = targetT;
		usedBps += delta;
	}

	bool progress = true;
	std::vector<bool> cooldownCounted(n, false);
	while (progress) {
		progress = false;
		double bestDensity = 0.0;
		size_t bestIdx = 0;
		uint8_t bestS = 0, bestT = 0;
		double bestCost = 0.0;

		for (size_t i = 0; i < n; ++i) {
			if (!allocs[i].active) continue;
			// Try spatial upgrade
			if (allocs[i].spatial < maxSpatialCap) {
				uint8_t ns = allocs[i].spatial + 1;
				uint8_t nt = allocs[i].temporal;
				double newCost = estimateLayerBitrateBps(subs[i], ns, nt);
				double oldCost = estimateLayerBitrateBps(subs[i], allocs[i].spatial, allocs[i].temporal);
				double delta = newCost - oldCost;
				if (delta > 0.0 && usedBps + delta <= plan.budgetBps) {
					double remainingHeadroom = plan.budgetBps - (usedBps + delta);
					if (remainingHeadroom < plan.budgetBps * kUpgradeHeadroomRatio)
						continue;
					bool inCooldown = prev[i].exists && nowMs > 0 &&
						prev[i].lastUpgradeAtMs > 0 &&
						(nowMs - prev[i].lastUpgradeAtMs) < kUpgradeCooldownMs;
					if (inCooldown) {
						if (!cooldownCounted[i] && lastState) {
							auto it = lastState->find(subs[i].consumerId);
							if (it != lastState->end())
								it->second.upgradeBlockedByCooldownCount++;
							cooldownCounted[i] = true;
						}
						continue;
					}
					double deltaUtility = allocs[i].utility;
					if (prev[i].exists && (ns != prev[i].spatial || nt != prev[i].temporal))
						deltaUtility *= (1.0 - kSwitchPenalty);
					double density = deltaUtility / delta;
					if (density > bestDensity) {
						bestDensity = density; bestIdx = i;
						bestS = ns; bestT = nt; bestCost = delta;
					}
				}
			}
			// Try temporal upgrade
			if (allocs[i].temporal < maxTemporalCap) {
				uint8_t ns = allocs[i].spatial;
				uint8_t nt = allocs[i].temporal + 1;
				double newCost = estimateLayerBitrateBps(subs[i], ns, nt);
				double oldCost = estimateLayerBitrateBps(subs[i], allocs[i].spatial, allocs[i].temporal);
				double delta = newCost - oldCost;
				if (delta > 0.0 && usedBps + delta <= plan.budgetBps) {
					double remainingHeadroom = plan.budgetBps - (usedBps + delta);
					if (remainingHeadroom < plan.budgetBps * kUpgradeHeadroomRatio)
						continue;
					bool inCooldown = prev[i].exists && nowMs > 0 &&
						prev[i].lastUpgradeAtMs > 0 &&
						(nowMs - prev[i].lastUpgradeAtMs) < kUpgradeCooldownMs;
					if (inCooldown) {
						if (!cooldownCounted[i] && lastState) {
							auto it = lastState->find(subs[i].consumerId);
							if (it != lastState->end())
								it->second.upgradeBlockedByCooldownCount++;
							cooldownCounted[i] = true;
						}
						continue;
					}
					double deltaUtility = allocs[i].utility;
					if (prev[i].exists && (ns != prev[i].spatial || nt != prev[i].temporal))
						deltaUtility *= (1.0 - kSwitchPenalty);
					double density = deltaUtility / delta;
					if (density > bestDensity) {
						bestDensity = density; bestIdx = i;
						bestS = ns; bestT = nt; bestCost = delta;
					}
				}
			}
		}
		if (bestDensity > 0.0) {
			allocs[bestIdx].spatial = bestS;
			allocs[bestIdx].temporal = bestT;
			usedBps += bestCost;
			progress = true;
		}
	}

	// Step 3: emit actions
	for (size_t i = 0; i < n; ++i) {
		const auto& sub = subs[i];
		if (allocs[i].active && prev[i].exists && !prev[i].paused && nowMs > 0 &&
			prev[i].lastLayerChangeAtMs > 0 &&
			(nowMs - prev[i].lastLayerChangeAtMs) < kMinDwellMs)
		{
			// Dwell-time guard: suppress immediate reversals right after a switch.
			if (allocs[i].spatial != prev[i].spatial || allocs[i].temporal != prev[i].temporal) {
				uint8_t dwellS = static_cast<uint8_t>(std::min(static_cast<int>(prev[i].spatial), maxSpatialCap));
				uint8_t dwellT = static_cast<uint8_t>(std::min(static_cast<int>(prev[i].temporal), maxTemporalCap));
				double currentCost = estimateLayerBitrateBps(subs[i], allocs[i].spatial, allocs[i].temporal);
				double dwellCost = estimateLayerBitrateBps(subs[i], dwellS, dwellT);
				double delta = dwellCost - currentCost;
				if (delta <= 0.0 || usedBps + delta <= plan.budgetBps) {
					allocs[i].spatial = dwellS;
					allocs[i].temporal = dwellT;
					usedBps += std::max(0.0, delta);
				}
			}
		}
		if (!allocs[i].active) {
			DownlinkAction a;
			a.type = DownlinkAction::Type::kPause;
			a.consumerId = sub.consumerId;
			plan.actions.push_back(a);
			DownlinkAction priorityAction;
			priorityAction.type = DownlinkAction::Type::kSetPriority;
			priorityAction.consumerId = sub.consumerId;
			priorityAction.priority = DownlinkAllocator::ComputePriority(sub);
			plan.actions.push_back(priorityAction);
			continue;
		}
		// Resume + set layers + set priority
		{
			DownlinkAction a;
			a.type = DownlinkAction::Type::kResume;
			a.consumerId = sub.consumerId;
			plan.actions.push_back(a);
		}
		{
			DownlinkAction a;
			a.type = DownlinkAction::Type::kSetLayers;
			a.consumerId = sub.consumerId;
			a.spatialLayer = allocs[i].spatial;
			a.temporalLayer = allocs[i].temporal;
			plan.actions.push_back(a);
		}
		{
			DownlinkAction a;
			a.type = DownlinkAction::Type::kSetPriority;
			a.consumerId = sub.consumerId;
			a.priority = DownlinkAllocator::ComputePriority(sub);
			plan.actions.push_back(a);
		}
	}

	return plan;
}

} // namespace mediasoup::qos
