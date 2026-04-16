#include <gtest/gtest.h>

#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include "../client/qos/QosController.h"

using namespace qos;

namespace {

RawSenderSnapshot MakeSnapshot(
	int64_t timestampMs = 1000,
	uint64_t bytesSent = 0,
	uint64_t packetsSent = 0,
	uint64_t packetsLost = 0,
	double targetBitrateBps = 900000,
	double roundTripTimeMs = 80,
	double jitterMs = 8,
	QualityLimitationReason reason = QualityLimitationReason::None)
{
	RawSenderSnapshot snapshot;
	snapshot.timestampMs = timestampMs;
	snapshot.trackId = "video";
	snapshot.producerId = "producer";
	snapshot.source = Source::Camera;
	snapshot.kind = TrackKind::Video;
	snapshot.bytesSent = bytesSent;
	snapshot.packetsSent = packetsSent;
	snapshot.packetsLost = packetsLost;
	snapshot.targetBitrateBps = targetBitrateBps;
	snapshot.configuredBitrateBps = 900000;
	snapshot.roundTripTimeMs = roundTripTimeMs;
	snapshot.jitterMs = jitterMs;
	snapshot.qualityLimitationReason = reason;
	return snapshot;
}

DerivedSignals MakeSignals() {
	DerivedSignals signals;
	signals.packetsSentDelta = 100;
	signals.sendBitrateBps = 900000;
	signals.targetBitrateBps = 1000000;
	signals.bitrateUtilization = 0.95;
	signals.lossRate = 0.0;
	signals.lossEwma = 0.0;
	signals.rttMs = 120;
	signals.rttEwma = 120;
	signals.jitterMs = 10;
	signals.jitterEwma = 10;
	signals.reason = Reason::Network;
	return signals;
}

PeerTrackState MakeTrackState(
	const std::string& trackId,
	Source source = Source::Camera,
	TrackKind kind = TrackKind::Video,
	Quality quality = Quality::Excellent)
{
	PeerTrackState track;
	track.trackId = trackId;
	track.source = source;
	track.kind = kind;
	track.state = State::Stable;
	track.quality = quality;
	track.level = 0;
	return track;
}

TrackBudgetRequest MakeBudgetRequest(
	const std::string& trackId,
	Source source = Source::Camera,
	uint32_t minBitrateBps = 0,
	uint32_t desiredBitrateBps = 0,
	uint32_t maxBitrateBps = 0,
	double weight = 1.0)
{
	TrackBudgetRequest request;
	request.trackId = trackId;
	request.source = source;
	request.kind = source == Source::Audio ? TrackKind::Audio : TrackKind::Video;
	request.minBitrateBps = minBitrateBps;
	request.desiredBitrateBps = desiredBitrateBps;
	request.maxBitrateBps = maxBitrateBps;
	request.weight = weight;
	return request;
}

nlohmann::json LoadProtocolFixture(const std::string& name) {
	std::ifstream f("tests/fixtures/qos_protocol/" + name + ".json");
	if (!f.is_open()) throw std::runtime_error("failed to open fixture: " + name);
	nlohmann::json payload;
	f >> payload;
	return payload;
}

} // namespace

TEST(ClientQosSignalsTest, CounterDeltaHandlesNormalIncrementsAndResets) {
	EXPECT_EQ(counterDelta(150, 100), 50u);
	EXPECT_EQ(counterDelta(100, 150), 0u);
	EXPECT_EQ(counterDelta(100, 0), 100u);
}

TEST(ClientQosSignalsTest, ComputeBitrateBpsComputesBitsPerSecondAndGuardsInvalidDeltas) {
	EXPECT_DOUBLE_EQ(computeBitrateBps(3000, 1000, 2000, 1000), 16000.0);
	EXPECT_DOUBLE_EQ(computeBitrateBps(3000, 1000, 1000, 1000), 0.0);
	EXPECT_DOUBLE_EQ(computeBitrateBps(3000, 1000, 900, 1000), 0.0);
	EXPECT_DOUBLE_EQ(computeBitrateBps(1000, 3000, 2000, 1000), 0.0);
}

TEST(ClientQosSignalsTest, ComputeLossRateUsesLostOverSentPlusLost) {
	EXPECT_NEAR(computeLossRate(80, 20), 0.2, 1e-9);
	EXPECT_DOUBLE_EQ(computeLossRate(0, 0), 0.0);
	EXPECT_DOUBLE_EQ(computeLossRate(0, 20), 1.0);
}

TEST(ClientQosSignalsTest, ComputeEwmaUsesExpectedSmoothingBehavior) {
	EXPECT_DOUBLE_EQ(computeEwma(100, std::numeric_limits<double>::quiet_NaN(), 0.35), 100.0);
	EXPECT_NEAR(computeEwma(100, 50, 0.35), 67.5, 1e-9);
	EXPECT_DOUBLE_EQ(computeEwma(100, 50, 0.0), 50.0);
	EXPECT_DOUBLE_EQ(computeEwma(100, 50, 1.0), 100.0);
}

TEST(ClientQosSignalsTest, ComputeUtilizationPrefersTargetThenConfiguredBitrate) {
	EXPECT_NEAR(computeUtilization(400000, 800000, 900000), 0.5, 1e-9);
	EXPECT_NEAR(computeUtilization(400000, 0, 1000000), 0.4, 1e-9);
	EXPECT_DOUBLE_EQ(computeUtilization(0, 0, 0), 0.0);
	EXPECT_DOUBLE_EQ(computeUtilization(10, 0, 0), 1.0);
}

TEST(ClientQosSignalsTest, ClassifyReasonPrioritizesOverrideManualCpuAndNetwork) {
	DerivedSignals baseSignals;
	baseSignals.bitrateUtilization = 1.0;
	baseSignals.rttEwma = 10.0;

	DerivedSignals cpuAndBandwidth = baseSignals;
	cpuAndBandwidth.cpuLimited = true;
	cpuAndBandwidth.bandwidthLimited = true;
	EXPECT_EQ(classifyReason(cpuAndBandwidth, true, false, 10), Reason::ServerOverride);
	EXPECT_EQ(classifyReason(cpuAndBandwidth, false, true, 10), Reason::Manual);

	DerivedSignals cpuOnly = baseSignals;
	cpuOnly.cpuLimited = true;
	EXPECT_EQ(classifyReason(cpuOnly, false, false, 2), Reason::Cpu);

	DerivedSignals networkOnly = baseSignals;
	networkOnly.bandwidthLimited = true;
	EXPECT_EQ(classifyReason(networkOnly, false, false, 0), Reason::Network);
	EXPECT_EQ(classifyReason(baseSignals, false, false, 0), Reason::Unknown);
}

TEST(ClientQosSignalsTest, DeriveSignalsComputesDeltasRatesEwmaFlagsAndReason) {
	RawSenderSnapshot previous = MakeSnapshot(1000, 1000, 100, 10, 800000, 100, 8);
	previous.retransmittedPacketsSent = 4;
	RawSenderSnapshot current = MakeSnapshot(2000, 21000, 220, 30, 1000000, 300, 20, QualityLimitationReason::Bandwidth);
	current.retransmittedPacketsSent = 10;

	DerivedSignals previousSignals;
	previousSignals.targetBitrateBps = 800000;
	previousSignals.bitrateUtilization = 1.0;
	previousSignals.lossEwma = 0.02;
	previousSignals.rttMs = 100;
	previousSignals.rttEwma = 100;
	previousSignals.jitterMs = 8;
	previousSignals.jitterEwma = 8;

	DeriveContext context;
	context.ewmaAlpha = 0.5;
	auto derived = deriveSignals(current, &previous, &previousSignals, context);

	EXPECT_EQ(derived.packetsSentDelta, 120u);
	EXPECT_EQ(derived.packetsLostDelta, 20u);
	EXPECT_EQ(derived.retransmittedPacketsSentDelta, 6u);
	EXPECT_DOUBLE_EQ(derived.sendBitrateBps, 160000.0);
	EXPECT_DOUBLE_EQ(derived.targetBitrateBps, 1000000.0);
	EXPECT_NEAR(derived.bitrateUtilization, 0.16, 1e-9);
	EXPECT_NEAR(derived.lossRate, 20.0 / 140.0, 1e-9);
	EXPECT_NEAR(derived.lossEwma, (20.0 / 140.0 + 0.02) / 2.0, 1e-9);
	EXPECT_DOUBLE_EQ(derived.rttMs, 300.0);
	EXPECT_DOUBLE_EQ(derived.rttEwma, 200.0);
	EXPECT_DOUBLE_EQ(derived.jitterMs, 20.0);
	EXPECT_DOUBLE_EQ(derived.jitterEwma, 14.0);
	EXPECT_FALSE(derived.cpuLimited);
	EXPECT_TRUE(derived.bandwidthLimited);
	EXPECT_EQ(derived.reason, Reason::Network);
}

TEST(ClientQosSignalsTest, DeriveSignalsSupportsCpuReasonWhenEnoughSamples) {
	RawSenderSnapshot current = MakeSnapshot(2000, 10000, 100, 0, 900000, 80, 8, QualityLimitationReason::Cpu);
	RawSenderSnapshot previous = MakeSnapshot(1000, 1000, 0, 0);

	DeriveContext context;
	context.cpuSampleCount = 3;
	auto derived = deriveSignals(current, &previous, nullptr, context);

	EXPECT_TRUE(derived.cpuLimited);
	EXPECT_FALSE(derived.bandwidthLimited);
	EXPECT_EQ(derived.reason, Reason::Cpu);
}

TEST(ClientQosSignalsTest, DeriveSignalsIgnoresBandwidthReasonWhenUtilizationIsStillHealthy) {
	RawSenderSnapshot previous = MakeSnapshot(1000, 1000, 100, 0);
	RawSenderSnapshot current = MakeSnapshot(2000, 86000, 220, 0, 800000, 80, 8, QualityLimitationReason::Bandwidth);

	auto derived = deriveSignals(current, &previous, nullptr);

	EXPECT_GE(derived.bitrateUtilization, 0.85);
	EXPECT_FALSE(derived.bandwidthLimited);
}

TEST(ClientQosSignalsTest, DeriveSignalsIgnoresBandwidthReasonAtModerateUtilization) {
	RawSenderSnapshot previous = MakeSnapshot(1000, 1000, 100, 0);
	RawSenderSnapshot current = MakeSnapshot(2000, 91000, 220, 0, 1000000, 80, 8, QualityLimitationReason::Bandwidth);

	auto derived = deriveSignals(current, &previous, nullptr);

	EXPECT_GT(derived.bitrateUtilization, 0.65);
	EXPECT_LT(derived.bitrateUtilization, 0.85);
	EXPECT_FALSE(derived.bandwidthLimited);
}

TEST(ClientQosSignalsTest, ReusesPreviousTransportMetricsWhenCurrentSnapshotOmitsThem) {
	RawSenderSnapshot previous = MakeSnapshot(1000, 10000, 150, 9, 550000, 120, 10);
	RawSenderSnapshot current = MakeSnapshot(2000, 18000, 210, 13, 550000, -1, -1);

	DerivedSignals previousSignals;
	previousSignals.lossEwma = 0.05;
	previousSignals.rttMs = 120;
	previousSignals.rttEwma = 120;
	previousSignals.jitterMs = 10;
	previousSignals.jitterEwma = 10;

	auto derived = deriveSignals(current, &previous, &previousSignals);

	EXPECT_EQ(derived.packetsSentDelta, 60u);
	EXPECT_EQ(derived.packetsLostDelta, 4u);
	EXPECT_NEAR(derived.lossRate, 4.0 / 64.0, 1e-9);
	EXPECT_DOUBLE_EQ(derived.rttMs, 120);
	EXPECT_DOUBLE_EQ(derived.rttEwma, 120);
	EXPECT_DOUBLE_EQ(derived.jitterMs, 10);
	EXPECT_DOUBLE_EQ(derived.jitterEwma, 10);
}

TEST(ClientQosSignalsTest, DeriveSignalsGuardsMissingCountersAndNegativeDeltas) {
	RawSenderSnapshot previous = MakeSnapshot(2000, 5000, 100, 10);
	previous.retransmittedPacketsSent = 5;
	RawSenderSnapshot current = MakeSnapshot(3000, 4000, 50, 5, 0, 80, 8);
	current.retransmittedPacketsSent = 1;
	current.configuredBitrateBps = 0;

	auto derived = deriveSignals(current, &previous, nullptr);

	EXPECT_DOUBLE_EQ(derived.sendBitrateBps, 0.0);
	EXPECT_EQ(derived.packetsSentDelta, 0u);
	EXPECT_EQ(derived.packetsLostDelta, 0u);
	EXPECT_EQ(derived.retransmittedPacketsSentDelta, 0u);
}

TEST(ClientQosStateMachineTest, StableTransitionsToEarlyWarningAfterTwoWarningSamples) {
	auto profile = makeCameraProfile();
	auto context = createInitialQosStateMachineContext(0);
	auto warningSignals = MakeSignals();
	warningSignals.bitrateUtilization = 0.8;
	warningSignals.bandwidthLimited = true;

	auto first = evaluateStateTransition(context, warningSignals, profile, 1000);
	EXPECT_EQ(first.context.state, State::Stable);
	EXPECT_FALSE(first.transitioned);

	auto second = evaluateStateTransition(first.context, warningSignals, profile, 2000);
	EXPECT_EQ(second.context.state, State::EarlyWarning);
	EXPECT_TRUE(second.transitioned);
}

TEST(ClientQosStateMachineTest, StableStaysStableForLowUtilizationAloneWhenNetworkSignalsAreHealthy) {
	auto profile = makeCameraProfile();
	auto context = createInitialQosStateMachineContext(0);
	auto lowUtilSignals = MakeSignals();
	lowUtilSignals.bitrateUtilization = 0.12;
	lowUtilSignals.sendBitrateBps = 120000;
	lowUtilSignals.targetBitrateBps = 1000000;
	lowUtilSignals.reason = Reason::Unknown;

	auto first = evaluateStateTransition(context, lowUtilSignals, profile, 1000);
	EXPECT_EQ(first.context.state, State::Stable);
	EXPECT_FALSE(first.transitioned);

	auto second = evaluateStateTransition(first.context, lowUtilSignals, profile, 2000);
	EXPECT_EQ(second.context.state, State::Stable);
	EXPECT_FALSE(second.transitioned);
}

TEST(ClientQosStateMachineTest, EarlyWarningTransitionsToCongestedAfterTwoCongestedSamples) {
	auto profile = makeCameraProfile();
	auto context = createInitialQosStateMachineContext(0);
	context.state = State::EarlyWarning;
	auto congestedSignals = MakeSignals();
	congestedSignals.bitrateUtilization = 0.5;
	congestedSignals.lossEwma = 0.1;

	auto first = evaluateStateTransition(context, congestedSignals, profile, 1000);
	EXPECT_EQ(first.context.state, State::EarlyWarning);
	EXPECT_FALSE(first.transitioned);

	auto second = evaluateStateTransition(first.context, congestedSignals, profile, 2000);
	EXPECT_EQ(second.context.state, State::Congested);
	EXPECT_TRUE(second.transitioned);
	EXPECT_EQ(second.context.lastCongestedAtMs, 2000);
}

TEST(ClientQosStateMachineTest, EarlyWarningTransitionsBackToStableAfterThreeHealthySamples) {
	auto profile = makeCameraProfile();
	auto context = createInitialQosStateMachineContext(0);
	context.state = State::EarlyWarning;

	context = evaluateStateTransition(context, MakeSignals(), profile, 1000).context;
	context = evaluateStateTransition(context, MakeSignals(), profile, 2000).context;
	auto result = evaluateStateTransition(context, MakeSignals(), profile, 3000);

	EXPECT_EQ(result.context.state, State::Stable);
	EXPECT_TRUE(result.transitioned);
}

TEST(ClientQosStateMachineTest, CongestedTransitionsToRecoveringAfterCooldownAndRecoverySamples) {
	auto profile = makeCameraProfile();
	auto context = createInitialQosStateMachineContext(0);
	context.state = State::Congested;
	context.enteredAtMs = 1000;
	context.lastCongestedAtMs = 1000;

	auto healthySignals = MakeSignals();
	context = evaluateStateTransition(context, healthySignals, profile, 2000).context;
	context = evaluateStateTransition(context, healthySignals, profile, 3000).context;
	context = evaluateStateTransition(context, healthySignals, profile, 4000).context;
	context = evaluateStateTransition(context, healthySignals, profile, 5000).context;
	auto beforeCooldown = evaluateStateTransition(context, healthySignals, profile, 6000);
	EXPECT_EQ(beforeCooldown.context.state, State::Congested);

	auto afterCooldown = evaluateStateTransition(beforeCooldown.context, healthySignals, profile, 9000);
	EXPECT_EQ(afterCooldown.context.state, State::Recovering);
	EXPECT_TRUE(afterCooldown.transitioned);
	EXPECT_EQ(afterCooldown.context.lastRecoveryAtMs, 9000);
}

TEST(ClientQosStateMachineTest, CongestedTransitionsToRecoveringWithWiderRecoveryJitterGate) {
	auto profile = makeCameraProfile();
	profile.thresholds.warnJitterMs = 28;
	profile.thresholds.stableJitterMs = 18;
	auto context = createInitialQosStateMachineContext(0);
	context.state = State::Congested;
	context.enteredAtMs = 1000;
	context.lastCongestedAtMs = 1000;

	auto recoverySignals = MakeSignals();
	recoverySignals.jitterMs = 24;
	recoverySignals.jitterEwma = 24;

	context = evaluateStateTransition(context, recoverySignals, profile, 5000).context;
	context = evaluateStateTransition(context, recoverySignals, profile, 6000).context;
	context = evaluateStateTransition(context, recoverySignals, profile, 7000).context;
	context = evaluateStateTransition(context, recoverySignals, profile, 8000).context;
	auto result = evaluateStateTransition(context, recoverySignals, profile, 9000);

	EXPECT_EQ(result.context.state, State::Recovering);
	EXPECT_TRUE(result.transitioned);
	EXPECT_EQ(result.context.consecutiveRecoverySamples, 5);
}

TEST(ClientQosStateMachineTest, CongestedCanUseFastRecoveryPathWhenEwmaJitterLags) {
	auto profile = makeCameraProfile();
	profile.thresholds.warnJitterMs = 28;
	profile.thresholds.stableJitterMs = 18;
	auto context = createInitialQosStateMachineContext(0);
	context.state = State::Congested;
	context.enteredAtMs = 1000;
	context.lastCongestedAtMs = 1000;

	auto fastRecoverySignals = MakeSignals();
	fastRecoverySignals.jitterMs = 24;
	fastRecoverySignals.jitterEwma = 36;
	fastRecoverySignals.targetBitrateBps = 120000;
	fastRecoverySignals.sendBitrateBps = 120000;

	context = evaluateStateTransition(context, fastRecoverySignals, profile, 9000).context;
	auto result = evaluateStateTransition(context, fastRecoverySignals, profile, 10000);

	EXPECT_EQ(result.context.state, State::Recovering);
	EXPECT_TRUE(result.transitioned);
	EXPECT_EQ(result.context.consecutiveFastRecoverySamples, 2);
}

TEST(ClientQosStateMachineTest, RecoveringTransitionsToStableAfterFiveHealthySamples) {
	auto profile = makeCameraProfile();
	auto context = createInitialQosStateMachineContext(0);
	context.state = State::Recovering;

	context = evaluateStateTransition(context, MakeSignals(), profile, 1000).context;
	context = evaluateStateTransition(context, MakeSignals(), profile, 2000).context;
	context = evaluateStateTransition(context, MakeSignals(), profile, 3000).context;
	context = evaluateStateTransition(context, MakeSignals(), profile, 4000).context;
	auto result = evaluateStateTransition(context, MakeSignals(), profile, 5000);

	EXPECT_EQ(result.context.state, State::Stable);
	EXPECT_TRUE(result.transitioned);
}

TEST(ClientQosStateMachineTest, RecoveringStaysRecoveringUntilJitterReachesStrictStableThreshold) {
	auto profile = makeCameraProfile();
	profile.thresholds.warnJitterMs = 28;
	profile.thresholds.stableJitterMs = 18;
	auto context = createInitialQosStateMachineContext(0);
	context.state = State::Recovering;

	auto recoverySignals = MakeSignals();
	recoverySignals.jitterMs = 24;
	recoverySignals.jitterEwma = 24;

	context = evaluateStateTransition(context, recoverySignals, profile, 1000).context;
	context = evaluateStateTransition(context, recoverySignals, profile, 2000).context;
	context = evaluateStateTransition(context, recoverySignals, profile, 3000).context;
	context = evaluateStateTransition(context, recoverySignals, profile, 4000).context;
	auto result = evaluateStateTransition(context, recoverySignals, profile, 5000);

	EXPECT_EQ(result.context.state, State::Recovering);
	EXPECT_FALSE(result.transitioned);
	EXPECT_EQ(result.context.consecutiveHealthySamples, 0);
	EXPECT_EQ(result.context.consecutiveRecoverySamples, 5);
}

TEST(ClientQosStateMachineTest, RecoveringTransitionsBackToCongestedAfterTwoCongestedSamples) {
	auto profile = makeCameraProfile();
	auto context = createInitialQosStateMachineContext(0);
	context.state = State::Recovering;
	auto congestedSignals = MakeSignals();
	congestedSignals.bitrateUtilization = 0.5;
	congestedSignals.lossEwma = 0.12;

	context = evaluateStateTransition(context, congestedSignals, profile, 1000).context;
	auto result = evaluateStateTransition(context, congestedSignals, profile, 2000);

	EXPECT_EQ(result.context.state, State::Congested);
	EXPECT_TRUE(result.transitioned);
}

TEST(ClientQosStateMachineTest, StableTransitionsToEarlyWarningAfterTwoHighJitterSamplesWhenConfigured) {
	auto profile = makeCameraProfile();
	profile.thresholds.warnJitterMs = 30;
	profile.thresholds.stableJitterMs = 20;
	auto context = createInitialQosStateMachineContext(0);
	auto jitterSignals = MakeSignals();
	jitterSignals.jitterMs = 36;
	jitterSignals.jitterEwma = 36;

	auto first = evaluateStateTransition(context, jitterSignals, profile, 1000);
	EXPECT_EQ(first.context.state, State::Stable);

	auto second = evaluateStateTransition(first.context, jitterSignals, profile, 2000);
	EXPECT_EQ(second.context.state, State::EarlyWarning);
	EXPECT_TRUE(second.transitioned);
}

TEST(ClientQosStateMachineTest, StableNeverJumpsDirectlyToCongested) {
	auto profile = makeCameraProfile();
	auto context = createInitialQosStateMachineContext(0);
	auto severeSignals = MakeSignals();
	severeSignals.bitrateUtilization = 0.2;
	severeSignals.lossEwma = 0.2;

	auto first = evaluateStateTransition(context, severeSignals, profile, 1000);
	EXPECT_EQ(first.context.state, State::Stable);

	auto second = evaluateStateTransition(first.context, severeSignals, profile, 2000);
	EXPECT_EQ(second.context.state, State::EarlyWarning);
}

TEST(ClientQosStateMachineTest, CongestedDoesNotJumpDirectlyToStable) {
	auto profile = makeCameraProfile();
	auto context = createInitialQosStateMachineContext(0);
	context.state = State::Congested;
	context.enteredAtMs = 1000;
	context.lastCongestedAtMs = 1000;

	context = evaluateStateTransition(context, MakeSignals(), profile, 10000).context;
	context = evaluateStateTransition(context, MakeSignals(), profile, 11000).context;
	context = evaluateStateTransition(context, MakeSignals(), profile, 12000).context;
	context = evaluateStateTransition(context, MakeSignals(), profile, 13000).context;
	auto result = evaluateStateTransition(context, MakeSignals(), profile, 14000);

	EXPECT_EQ(result.context.state, State::Recovering);
	EXPECT_NE(result.context.state, State::Stable);
}

TEST(ClientQosStateMachineTest, MapStateToQualityReturnsExpectedValues) {
	auto healthy = MakeSignals();
	auto lostLike = MakeSignals();
	lostLike.sendBitrateBps = 0;

	EXPECT_EQ(mapStateToQuality(State::Stable, healthy), Quality::Excellent);
	EXPECT_EQ(mapStateToQuality(State::EarlyWarning, healthy), Quality::Good);
	EXPECT_EQ(mapStateToQuality(State::Recovering, healthy), Quality::Good);
	EXPECT_EQ(mapStateToQuality(State::Congested, healthy), Quality::Poor);
	EXPECT_EQ(mapStateToQuality(State::Congested, lostLike), Quality::Lost);
}

TEST(ClientQosPlannerTest, StableRecoversOneLevelAtATime) {
	auto profile = makeCameraProfile();
	PlannerInput input;
	input.source = Source::Camera;
	input.profile = &profile;
	input.state = State::Stable;
	input.currentLevel = 2;

	auto actions = planActions(input);

	ASSERT_EQ(actions.size(), 1u);
	EXPECT_EQ(actions[0].type, ActionType::SetEncodingParameters);
	EXPECT_EQ(actions[0].level, 1);
	ASSERT_TRUE(actions[0].encodingParameters.has_value());
	ASSERT_TRUE(actions[0].encodingParameters->maxBitrateBps.has_value());
	EXPECT_EQ(*actions[0].encodingParameters->maxBitrateBps, 850000u);
}

TEST(ClientQosPlannerTest, DegradesAudioSourceWithBitrateFocusedActions) {
	auto profile = makeAudioProfile();
	PlannerInput input;
	input.source = Source::Audio;
	input.profile = &profile;
	input.state = State::Congested;
	input.currentLevel = 1;

	auto actions = planActions(input);

	ASSERT_EQ(actions.size(), 1u);
	EXPECT_EQ(actions[0].type, ActionType::SetEncodingParameters);
	EXPECT_EQ(actions[0].level, 2);
	ASSERT_TRUE(actions[0].encodingParameters.has_value());
	ASSERT_TRUE(actions[0].encodingParameters->maxBitrateBps.has_value());
	EXPECT_EQ(*actions[0].encodingParameters->maxBitrateBps, 24000u);
}

TEST(ClientQosPlannerTest, ScreenSharePausesUpstreamAtMaxLevel) {
	auto profile = makeScreenShareProfile();
	PlannerInput input;
	input.source = Source::ScreenShare;
	input.profile = &profile;
	input.state = State::Congested;
	input.currentLevel = 3;

	auto actions = planActions(input);

	ASSERT_EQ(actions.size(), 1u);
	EXPECT_EQ(actions[0].type, ActionType::PauseUpstream);
	EXPECT_EQ(actions[0].level, 4);
}

TEST(ClientQosPlannerTest, DegradesOnCongestedState) {
	auto profile = makeCameraProfile();
	PlannerInput input;
	input.source = Source::Camera;
	input.profile = &profile;
	input.state = State::Congested;
	input.currentLevel = 1;

	auto actions = planActions(input);

	ASSERT_EQ(actions.size(), 1u);
	EXPECT_EQ(actions[0].type, ActionType::SetEncodingParameters);
	EXPECT_EQ(actions[0].level, 2);
}

TEST(ClientQosPlannerTest, CameraAtMaxLevelEntersAudioOnlyOnCongestion) {
	auto profile = makeCameraProfile();
	PlannerInput input;
	input.source = Source::Camera;
	input.profile = &profile;
	input.state = State::Congested;
	input.currentLevel = 4;
	input.inAudioOnlyMode = false;

	auto actions = planActions(input);

	ASSERT_EQ(actions.size(), 1u);
	EXPECT_EQ(actions[0].type, ActionType::EnterAudioOnly);
	EXPECT_EQ(actions[0].level, 4);
}

TEST(ClientQosPlannerTest, ExitsAudioOnlyWhenRecoveringFromMaxLevel) {
	auto profile = makeCameraProfile();
	PlannerInput input;
	input.source = Source::Camera;
	input.profile = &profile;
	input.state = State::Recovering;
	input.currentLevel = 4;
	input.inAudioOnlyMode = true;

	auto actions = planActions(input);

	ASSERT_GE(actions.size(), 2u);
	EXPECT_EQ(actions[0].type, ActionType::ExitAudioOnly);
	EXPECT_EQ(actions[0].level, 3);
	EXPECT_EQ(actions[1].type, ActionType::SetEncodingParameters);
	EXPECT_EQ(actions[1].level, 3);
}

TEST(ClientQosPlannerTest, ResumesScreenshareOnRecoveryFromPausedState) {
	auto profile = makeScreenShareProfile();
	PlannerInput input;
	input.source = Source::ScreenShare;
	input.profile = &profile;
	input.state = State::Recovering;
	input.currentLevel = 4;

	auto actions = planActions(input);

	ASSERT_GE(actions.size(), 2u);
	EXPECT_EQ(actions[0].type, ActionType::ResumeUpstream);
	EXPECT_EQ(actions[0].level, 3);
	EXPECT_EQ(actions[1].type, ActionType::SetEncodingParameters);
	EXPECT_EQ(actions[1].level, 3);
}

TEST(ClientQosPlannerTest, AppliesOverrideClamp) {
	auto profile = makeCameraProfile();
	PlannerInput input;
	input.source = Source::Camera;
	input.profile = &profile;
	input.state = State::Congested;
	input.currentLevel = 2;
	input.overrideClampLevel = 2;

	auto actions = planActions(input);

	ASSERT_EQ(actions.size(), 1u);
	EXPECT_EQ(actions[0].type, ActionType::Noop);
	EXPECT_EQ(actions[0].level, 2);
}

TEST(ClientQosPlannerTest, PlanRecoveryForcesRecoveringPath) {
	auto profile = makeCameraProfile();
	PlannerInput input;
	input.source = Source::Camera;
	input.profile = &profile;
	input.state = State::Congested;
	input.currentLevel = 2;

	auto actions = planRecovery(input);

	ASSERT_EQ(actions.size(), 1u);
	EXPECT_EQ(actions[0].level, 1);
}

TEST(ClientQosProbeTest, BeginProbeInitializesProbeContext) {
	auto probe = beginProbe(4, 3, true, false, 1000);

	EXPECT_TRUE(probe.active);
	EXPECT_EQ(probe.startedAtMs, 1000);
	EXPECT_EQ(probe.previousLevel, 4);
	EXPECT_EQ(probe.targetLevel, 3);
	EXPECT_TRUE(probe.previousAudioOnlyMode);
	EXPECT_FALSE(probe.targetAudioOnlyMode);
	EXPECT_EQ(probe.requiredHealthySamples, 3);
	EXPECT_EQ(probe.requiredBadSamples, 2);
}

TEST(ClientQosProbeTest, ProbeSucceedsAfterThreeHealthySamples) {
	auto profile = makeCameraProfile();
	auto probe = beginProbe(2, 1, false, false, 1000);
	auto healthySignals = MakeSignals();

	EXPECT_EQ(evaluateProbe(probe, healthySignals, profile), ProbeResult::Inconclusive);
	EXPECT_EQ(evaluateProbe(probe, healthySignals, profile), ProbeResult::Inconclusive);
	EXPECT_EQ(evaluateProbe(probe, healthySignals, profile), ProbeResult::Successful);
}

TEST(ClientQosProbeTest, ProbeFailsAfterTwoBadSamples) {
	auto profile = makeCameraProfile();
	auto probe = beginProbe(2, 1, false, false, 1000);
	auto badSignals = MakeSignals();
	badSignals.lossEwma = 0.2;
	badSignals.bandwidthLimited = true;

	EXPECT_EQ(evaluateProbe(probe, badSignals, profile), ProbeResult::Inconclusive);
	EXPECT_EQ(evaluateProbe(probe, badSignals, profile), ProbeResult::Failed);
}

TEST(ClientQosProbeTest, ProbeStaysInconclusiveForMixedSamplesBelowCompletionThresholds) {
	auto profile = makeCameraProfile();
	auto probe = beginProbe(2, 1, false, false, 1000);
	auto healthySignals = MakeSignals();
	auto badSignals = MakeSignals();
	badSignals.lossEwma = 0.2;
	badSignals.bandwidthLimited = true;

	EXPECT_EQ(evaluateProbe(probe, healthySignals, profile), ProbeResult::Inconclusive);
	EXPECT_EQ(evaluateProbe(probe, badSignals, profile), ProbeResult::Inconclusive);
	EXPECT_EQ(evaluateProbe(probe, healthySignals, profile), ProbeResult::Inconclusive);
}

TEST(ClientQosCoordinatorTest, PrioritizesAudioThenScreenShareThenCamera) {
	auto decision = buildPeerDecision({
		MakeTrackState("camera", Source::Camera, TrackKind::Video),
		MakeTrackState("audio", Source::Audio, TrackKind::Audio),
		MakeTrackState("screen", Source::ScreenShare, TrackKind::Video),
	});

	EXPECT_EQ(decision.prioritizedTrackIds, (std::vector<std::string>{"audio", "screen", "camera"}));
	EXPECT_TRUE(decision.keepAudioAlive);
	EXPECT_TRUE(decision.preferScreenShare);
}

TEST(ClientQosCoordinatorTest, SacrificesCameraFirstWhenScreenShareExists) {
	auto decision = buildPeerDecision({
		MakeTrackState("camera", Source::Camera, TrackKind::Video),
		MakeTrackState("screen", Source::ScreenShare, TrackKind::Video),
		MakeTrackState("audio", Source::Audio, TrackKind::Audio),
	});

	EXPECT_EQ(decision.sacrificialTrackIds, (std::vector<std::string>{"camera"}));
}

TEST(ClientQosCoordinatorTest, DerivesPeerQualityFromWorstTrack) {
	auto decision = buildPeerDecision({
		MakeTrackState("camera", Source::Camera, TrackKind::Video, Quality::Excellent),
		MakeTrackState("audio", Source::Audio, TrackKind::Audio, Quality::Poor),
	});

	EXPECT_EQ(decision.peerQuality, Quality::Poor);
	EXPECT_FALSE(decision.allowVideoRecovery);
}

TEST(ClientQosCoordinatorTest, HandlesEmptyTrackSet) {
	auto decision = buildPeerDecision({});

	EXPECT_EQ(decision.peerQuality, Quality::Lost);
	EXPECT_TRUE(decision.prioritizedTrackIds.empty());
	EXPECT_TRUE(decision.sacrificialTrackIds.empty());
}

TEST(ClientQosCoordinatorTest, CoordinatorTracksUpsertRemoveAndDecision) {
	PeerQosCoordinator coordinator;
	coordinator.upsertTrack(MakeTrackState("audio", Source::Audio, TrackKind::Audio));
	coordinator.upsertTrack(MakeTrackState("camera", Source::Camera, TrackKind::Video));

	auto decision = coordinator.getDecision();
	EXPECT_EQ(decision.prioritizedTrackIds, (std::vector<std::string>{"audio", "camera"}));

	coordinator.removeTrack("camera");
	decision = coordinator.getDecision();
	EXPECT_EQ(decision.prioritizedTrackIds, (std::vector<std::string>{"audio"}));

	coordinator.clear();
	EXPECT_TRUE(coordinator.getDecision().prioritizedTrackIds.empty());
}

TEST(ClientQosBudgetAllocatorTest, PrioritizesAudioThenScreenShareThenCameraWhenBudgetIsTight) {
	auto decision = allocatePeerTrackBudgets(1500000, {
		MakeBudgetRequest("audio", Source::Audio, 32000, 64000, 64000),
		MakeBudgetRequest("screen", Source::ScreenShare, 400000, 900000, 900000),
		MakeBudgetRequest("camera-main", Source::Camera, 150000, 600000, 600000),
		MakeBudgetRequest("camera-side", Source::Camera, 150000, 600000, 600000),
	});

	ASSERT_EQ(decision.allocations.size(), 4u);
	EXPECT_EQ(decision.totalBudgetBps, 1500000u);
	EXPECT_EQ(decision.allocatedBudgetBps, 1500000u);
	EXPECT_EQ(decision.allocations[0].allocatedBitrateBps, 64000u);
	EXPECT_EQ(decision.allocations[1].allocatedBitrateBps, 900000u);
	EXPECT_EQ(decision.allocations[2].allocatedBitrateBps, 268000u);
	EXPECT_EQ(decision.allocations[3].allocatedBitrateBps, 268000u);
	EXPECT_FALSE(decision.allocations[0].capped);
	EXPECT_FALSE(decision.allocations[1].capped);
	EXPECT_TRUE(decision.allocations[2].capped);
	EXPECT_TRUE(decision.allocations[3].capped);
}

TEST(ClientQosBudgetAllocatorTest, DistributesCameraBudgetByWeightWithinSameTier) {
	auto decision = allocatePeerTrackBudgets(1400000, {
		MakeBudgetRequest("camera-main", Source::Camera, 100000, 800000, 800000, 4.0),
		MakeBudgetRequest("camera-side-a", Source::Camera, 100000, 800000, 800000, 2.0),
		MakeBudgetRequest("camera-side-b", Source::Camera, 100000, 800000, 800000, 1.0),
	});

	ASSERT_EQ(decision.allocations.size(), 3u);
	EXPECT_EQ(decision.allocations[0].allocatedBitrateBps, 728571u);
	EXPECT_EQ(decision.allocations[1].allocatedBitrateBps, 414286u);
	EXPECT_EQ(decision.allocations[2].allocatedBitrateBps, 257143u);
	EXPECT_EQ(decision.allocatedBudgetBps, 1400000u);
}

TEST(ClientQosBudgetAllocatorTest, BuildsCoordinationOverridesFromBudgetAllocations) {
	auto overrides = buildCoordinationOverrides(
		{
			MakeTrackState("screen", Source::ScreenShare, TrackKind::Video),
			MakeTrackState("camera-main", Source::Camera, TrackKind::Video),
			MakeTrackState("camera-side", Source::Camera, TrackKind::Video),
		},
		PeerBudgetDecision{
			1500000,
			1500000,
			{
				{"screen", 900000, false},
				{"camera-main", 600000, true},
				{"camera-side", 0, true},
			}
		});

	ASSERT_EQ(overrides.size(), 3u);
	ASSERT_TRUE(overrides.count("screen") > 0);
	ASSERT_TRUE(overrides.count("camera-main") > 0);
	ASSERT_TRUE(overrides.count("camera-side") > 0);
	ASSERT_TRUE(overrides["screen"].maxBitrateCapBps.has_value());
	EXPECT_EQ(*overrides["screen"].maxBitrateCapBps, 900000u);
	ASSERT_TRUE(overrides["camera-main"].maxBitrateCapBps.has_value());
	EXPECT_EQ(*overrides["camera-main"].maxBitrateCapBps, 600000u);
	EXPECT_TRUE(overrides["camera-main"].disableRecovery.value_or(false));
	EXPECT_TRUE(overrides["camera-side"].forceAudioOnly.value_or(false));
	EXPECT_TRUE(overrides["camera-side"].disableRecovery.value_or(false));
}

TEST(ClientQosBudgetAllocatorTest, RestoresPausedScreenShareWhenBudgetReturns) {
	auto screen = MakeTrackState("screen", Source::ScreenShare, TrackKind::Video);
	screen.level = 4;

	auto overrides = buildCoordinationOverrides(
		{screen},
		PeerBudgetDecision{
			900000,
			900000,
			{
				{"screen", 900000, false},
			}
		});

	ASSERT_EQ(overrides.size(), 1u);
	EXPECT_TRUE(overrides["screen"].resumeUpstream.value_or(false));
	ASSERT_TRUE(overrides["screen"].maxBitrateCapBps.has_value());
	EXPECT_EQ(*overrides["screen"].maxBitrateCapBps, 900000u);
}

TEST(ClientQosProtocolTest, ParsesAndValidatesValidClientSnapshotFixture) {
	auto fixture = LoadProtocolFixture("valid_client_v1");

	EXPECT_TRUE(isClientQosSnapshot(fixture));
	auto parsed = parseClientQosSnapshot(fixture);
	EXPECT_EQ(parsed["schema"], "mediasoup.qos.client.v1");
	EXPECT_EQ(parsed["seq"], 42);
	ASSERT_TRUE(parsed["tracks"].is_array());
	EXPECT_EQ(parsed["tracks"].size(), 1u);

	auto serialized = serializeClientQosSnapshot(parsed);
	EXPECT_TRUE(isClientQosSnapshot(serialized));
}

TEST(ClientQosProtocolTest, RejectsInvalidClientSnapshotFixtures) {
	auto missingSchema = LoadProtocolFixture("invalid_missing_schema");
	auto badSeq = LoadProtocolFixture("invalid_bad_seq");

	EXPECT_FALSE(isClientQosSnapshot(missingSchema));
	EXPECT_FALSE(isClientQosSnapshot(badSeq));
	EXPECT_THROW(parseClientQosSnapshot(missingSchema), std::invalid_argument);
	EXPECT_THROW(parseClientQosSnapshot(badSeq), std::invalid_argument);
}

TEST(ClientQosProtocolTest, RejectsSnapshotsThatExceedTrackLimit) {
	auto payload = LoadProtocolFixture("valid_client_v1");
	payload["tracks"] = nlohmann::json::array();
	for (size_t i = 0; i <= qos::kQosMaxTracksPerSnapshot; ++i) {
		auto track = LoadProtocolFixture("valid_client_v1")["tracks"][0];
		track["localTrackId"] = "video-" + std::to_string(i);
		payload["tracks"].push_back(track);
	}

	EXPECT_FALSE(isClientQosSnapshot(payload));
	EXPECT_THROW(parseClientQosSnapshot(payload), std::invalid_argument);
}

TEST(ClientQosProtocolTest, ParsesAndValidatesValidPolicyAndOverrideFixtures) {
	auto policyPayload = LoadProtocolFixture("valid_policy_v1");
	auto overridePayload = LoadProtocolFixture("valid_override_v1");

	EXPECT_TRUE(isQosPolicy(policyPayload));
	EXPECT_TRUE(isQosOverride(overridePayload));

	auto policy = parseQosPolicy(policyPayload);
	auto overrideData = parseQosOverride(overridePayload);

	EXPECT_EQ(policy.schema, "mediasoup.qos.policy.v1");
	EXPECT_EQ(policy.sampleIntervalMs, 1000);
	EXPECT_EQ(overrideData.schema, "mediasoup.qos.override.v1");
	ASSERT_TRUE(overrideData.maxLevelClamp.has_value());
	EXPECT_EQ(*overrideData.maxLevelClamp, 2);
}

TEST(ClientQosProtocolTest, RejectsMutuallyExclusivePauseAndResumeOverrideFlags) {
	auto overridePayload = LoadProtocolFixture("valid_override_v1");
	overridePayload["pauseUpstream"] = true;
	overridePayload["resumeUpstream"] = true;

	EXPECT_THROW(parseQosOverride(overridePayload), std::invalid_argument);
}

TEST(ClientQosProtocolTest, RejectsTrackScopedOverrideWithoutTrackId) {
	auto overridePayload = LoadProtocolFixture("valid_override_v1");
	overridePayload["scope"] = "track";
	overridePayload.erase("trackId");

	EXPECT_FALSE(isQosOverride(overridePayload));
	EXPECT_THROW(parseQosOverride(overridePayload), std::invalid_argument);
}

TEST(ClientQosProtocolTest, SerializeSnapshotIncludesServerCompatibleTrackFields) {
	PeerTrackState track;
	track.trackId = "video";
	track.producerId = "producer-1";
	track.kind = TrackKind::Video;
	track.source = Source::Camera;
	track.state = State::EarlyWarning;
	track.quality = Quality::Good;
	track.level = 1;
	track.reason = Reason::Network;

	DerivedSignals signals = MakeSignals();
	signals.reason = Reason::Network;

	PlannedAction lastAction;
	lastAction.type = ActionType::SetEncodingParameters;
	lastAction.level = 1;

	RawSenderSnapshot raw = MakeSnapshot(2000, 21000, 220, 30, 1000000, 300, 20, QualityLimitationReason::Bandwidth);
	raw.frameWidth = 640;
	raw.frameHeight = 360;
	raw.framesPerSecond = 24.0;

	auto snapshot = serializeSnapshot(
		7,
		1234,
		Quality::Good,
		false,
		{track},
		{{"video", signals}},
		{{"video", lastAction}},
		{{"video", raw}},
		{{"video", false}});

	ASSERT_TRUE(snapshot["tracks"].is_array());
	ASSERT_EQ(snapshot["tracks"].size(), 1u);
	auto serializedTrack = snapshot["tracks"][0];

	EXPECT_EQ(serializedTrack["producerId"], "producer-1");
	EXPECT_EQ(serializedTrack["reason"], "network");
	EXPECT_DOUBLE_EQ(serializedTrack["signals"]["bitrateUtilization"].get<double>(), 0.95);
	EXPECT_DOUBLE_EQ(serializedTrack["signals"]["lossEwma"].get<double>(), 0.0);
	EXPECT_DOUBLE_EQ(serializedTrack["signals"]["rttEwma"].get<double>(), 120.0);
	EXPECT_DOUBLE_EQ(serializedTrack["signals"]["jitterEwma"].get<double>(), 10.0);
	EXPECT_EQ(serializedTrack["signals"]["frameWidth"], 640);
	EXPECT_EQ(serializedTrack["signals"]["frameHeight"], 360);
	EXPECT_DOUBLE_EQ(serializedTrack["signals"]["framesPerSecond"].get<double>(), 24.0);
	EXPECT_EQ(serializedTrack["signals"]["qualityLimitationReason"], "bandwidth");
	EXPECT_FALSE(serializedTrack["lastAction"]["applied"].get<bool>());
	EXPECT_EQ(snapshot["peerState"]["mode"], "audio-video");
}

TEST(ClientQosProtocolTest, SerializeSnapshotSupportsVideoOnlyModeWhenPeerHasNoAudioTrack) {
	PeerTrackState track;
	track.trackId = "video";
	track.kind = TrackKind::Video;
	track.source = Source::Camera;

	auto snapshot = serializeSnapshot(
		1,
		1234,
		Quality::Excellent,
		false,
		{track},
		{},
		{},
		{},
		{},
		false);

	EXPECT_EQ(snapshot["peerState"]["mode"], "video-only");
}

TEST(ClientQosProtocolTest, SerializeSnapshotRejectsMoreThanSupportedTrackCount) {
	std::vector<PeerTrackState> tracks;
	std::map<std::string, DerivedSignals> signals;
	std::map<std::string, PlannedAction> actions;
	for (size_t i = 0; i <= qos::kQosMaxTracksPerSnapshot; ++i) {
		PeerTrackState track;
		track.trackId = "video-" + std::to_string(i);
		track.kind = TrackKind::Video;
		track.source = Source::Camera;
		tracks.push_back(track);
		signals[track.trackId] = MakeSignals();
		actions[track.trackId] = PlannedAction{};
	}

	EXPECT_THROW(
		serializeSnapshot(1, 1000, Quality::Good, false, tracks, signals, actions),
		std::invalid_argument);
}

TEST(ClientQosProfilesTest, ExposeSourceSpecificDefaultsAndResolveHelper) {
	auto camera = makeCameraProfile();
	auto screenShare = makeScreenShareProfile();
	auto audio = makeAudioProfile();
	auto resolvedCamera = resolveProfile(Source::Camera);
	auto resolvedScreenShare = resolveProfile(Source::ScreenShare);
	auto resolvedAudio = resolveProfile(Source::Audio);

	EXPECT_EQ(camera.name, "default-camera");
	EXPECT_EQ(screenShare.name, "default-screenshare");
	EXPECT_EQ(audio.name, "default-audio");
	EXPECT_EQ(resolvedCamera.name, "default-camera");
	EXPECT_EQ(resolvedScreenShare.name, "default-screenshare");
	EXPECT_EQ(resolvedAudio.name, "default-audio");

	auto conservative = resolveProfileByName(Source::Camera, "conservative");
	ASSERT_TRUE(conservative.has_value());
	EXPECT_EQ(conservative->name, "conservative");
}

TEST(ClientQosProfilesTest, MatchCurrentJsThresholdsAndLadders) {
	auto camera = makeCameraProfile();
	EXPECT_DOUBLE_EQ(camera.thresholds.congestedRttMs, 320);
	EXPECT_DOUBLE_EQ(camera.thresholds.warnJitterMs, 30);
	EXPECT_DOUBLE_EQ(camera.thresholds.stableJitterMs, 20);
	EXPECT_DOUBLE_EQ(camera.thresholds.stableBitrateUtilization, 0.92);
	ASSERT_EQ(camera.ladder.size(), 5u);
	ASSERT_TRUE(camera.ladder[1].encodingParameters.has_value());
	EXPECT_EQ(*camera.ladder[1].encodingParameters->maxBitrateBps, 850000u);
	ASSERT_TRUE(camera.ladder[2].encodingParameters.has_value());
	EXPECT_EQ(*camera.ladder[2].encodingParameters->maxFramerate, 20u);
	EXPECT_DOUBLE_EQ(*camera.ladder[2].encodingParameters->scaleResolutionDownBy, 1.5);
	EXPECT_EQ(*camera.ladder[3].spatialLayer, 0);
	EXPECT_TRUE(camera.ladder[4].enterAudioOnly);

	auto screenShare = makeScreenShareProfile();
	ASSERT_EQ(screenShare.ladder.size(), 5u);
	ASSERT_TRUE(screenShare.ladder[2].encodingParameters.has_value());
	EXPECT_EQ(*screenShare.ladder[2].encodingParameters->maxBitrateBps, 1350000u);
	ASSERT_TRUE(screenShare.ladder[3].encodingParameters.has_value());
	EXPECT_EQ(*screenShare.ladder[3].encodingParameters->maxBitrateBps, 990000u);
	EXPECT_DOUBLE_EQ(*screenShare.ladder[3].encodingParameters->scaleResolutionDownBy, 1.25);
	EXPECT_TRUE(screenShare.ladder[3].resumeUpstream);
	EXPECT_TRUE(screenShare.ladder[4].pauseUpstream);

	auto audio = makeAudioProfile();
	ASSERT_EQ(audio.ladder.size(), 5u);
	ASSERT_TRUE(audio.ladder[1].encodingParameters.has_value());
	EXPECT_EQ(*audio.ladder[1].encodingParameters->maxBitrateBps, 32000u);
	EXPECT_TRUE(*audio.ladder[1].encodingParameters->adaptivePtime);
	ASSERT_TRUE(audio.ladder[4].encodingParameters.has_value());
	EXPECT_EQ(*audio.ladder[4].encodingParameters->maxBitrateBps, 16000u);
	EXPECT_TRUE(*audio.ladder[4].encodingParameters->adaptivePtime);

	auto conservative = resolveProfileByName(Source::Camera, "conservative");
	ASSERT_TRUE(conservative.has_value());
	ASSERT_TRUE(conservative->ladder[2].encodingParameters.has_value());
	EXPECT_EQ(*conservative->ladder[2].encodingParameters->maxBitrateBps, 550000u);
	EXPECT_EQ(*conservative->ladder[2].encodingParameters->maxFramerate, 15u);
	EXPECT_DOUBLE_EQ(*conservative->ladder[2].encodingParameters->scaleResolutionDownBy, 1.5);
}

TEST(ClientQosExecutorTest, AppliesActionThroughSink) {
	std::vector<PlannedAction> applied;
	QosActionExecutor executor([&](const PlannedAction& action) {
		applied.push_back(action);
		return true;
	});

	PlannedAction action;
	action.type = ActionType::SetEncodingParameters;
	action.level = 2;
	action.encodingParameters = EncodingParameters{250000, 15, 2.0, {}};

	EXPECT_TRUE(executor.execute(action));
	ASSERT_EQ(applied.size(), 1u);
	EXPECT_EQ(applied[0].type, ActionType::SetEncodingParameters);
}

TEST(ClientQosExecutorTest, SkipsNoopActions) {
	bool called = false;
	QosActionExecutor executor([&](const PlannedAction&) {
		called = true;
		return true;
	});

	EXPECT_TRUE(executor.execute(PlannedAction{}));
	EXPECT_FALSE(called);
}

TEST(ClientQosExecutorTest, SkipsIdempotentRepeatActionsByDefault) {
	int callCount = 0;
	QosActionExecutor executor([&](const PlannedAction&) {
		++callCount;
		return true;
	});

	PlannedAction action;
	action.type = ActionType::SetEncodingParameters;
	action.level = 1;
	action.encodingParameters = EncodingParameters{500000, 24, 1.0, {}};

	EXPECT_TRUE(executor.execute(action));
	EXPECT_TRUE(executor.execute(action));
	EXPECT_EQ(callCount, 1);
}

TEST(ClientQosExecutorTest, ReappliesEncodingActionWhenScaleChanges) {
	int callCount = 0;
	QosActionExecutor executor([&](const PlannedAction&) {
		++callCount;
		return true;
	});

	PlannedAction action;
	action.type = ActionType::SetEncodingParameters;
	action.level = 1;
	action.encodingParameters = EncodingParameters{500000, 24, 1.0, {}};

	PlannedAction scaledAction = action;
	scaledAction.encodingParameters->scaleResolutionDownBy = 1.5;

	EXPECT_TRUE(executor.execute(action));
	EXPECT_TRUE(executor.execute(scaledAction));
	EXPECT_EQ(callCount, 2);
}

TEST(ClientQosExecutorTest, ResetAllowsRepeatingPreviouslyAppliedAction) {
	int callCount = 0;
	QosActionExecutor executor([&](const PlannedAction&) {
		++callCount;
		return true;
	});

	PlannedAction action;
	action.type = ActionType::SetEncodingParameters;
	action.level = 1;
	action.encodingParameters = EncodingParameters{500000, 24, 1.0, {}};

	EXPECT_TRUE(executor.execute(action));
	executor.reset();
	EXPECT_TRUE(executor.execute(action));
	EXPECT_EQ(callCount, 2);
}

TEST(ClientQosControllerTest, WarmupSamplesDelayFirstReactiveAction) {
	std::vector<PlannedAction> appliedActions;
	PublisherQosController::Options options;
	options.source = Source::Camera;
	options.trackId = "video";
	options.producerId = "producer";
	options.initialLevel = 0;
	options.actionSink = [&](const PlannedAction& action) {
		appliedActions.push_back(action);
		return true;
	};

	PublisherQosController controller(options);

	for (int i = 0; i < 5; ++i) {
		controller.onSample(MakeSnapshot(1000LL * (i + 1), 120000ULL * (i + 1), 100ULL * (i + 1), 0));
	}

	EXPECT_TRUE(appliedActions.empty());
	EXPECT_EQ(controller.currentLevel(), 0);

	controller.onSample(MakeSnapshot(6000, 610000, 600, 120, 900000, 500, 80, QualityLimitationReason::Bandwidth));

	EXPECT_TRUE(appliedActions.empty());
	EXPECT_EQ(controller.currentState(), State::Stable);

	controller.onSample(MakeSnapshot(7000, 620000, 700, 140, 900000, 500, 80, QualityLimitationReason::Bandwidth));

	ASSERT_EQ(appliedActions.size(), 1u);
	EXPECT_EQ(appliedActions[0].type, ActionType::SetEncodingParameters);
	EXPECT_EQ(appliedActions[0].level, 1);
	EXPECT_EQ(controller.currentLevel(), 1);
	EXPECT_EQ(controller.currentState(), State::EarlyWarning);
}

TEST(ClientQosControllerTest, CanDegradeAfterTwoWarningSamplesWhenWarmupDisabled) {
	std::vector<PlannedAction> appliedActions;
	int64_t monotonicNowMs = 0;
	PublisherQosController::Options options;
	options.source = Source::Camera;
	options.trackId = "video";
	options.producerId = "producer";
	options.initialLevel = 0;
	options.warmupSamples = 0;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	options.actionSink = [&](const PlannedAction& action) {
		appliedActions.push_back(action);
		return true;
	};

	PublisherQosController controller(options);

	monotonicNowMs = 1000;
	controller.onSample(MakeSnapshot(1000, 1000, 100, 0, 900000, 120, 8, QualityLimitationReason::Bandwidth));
	EXPECT_TRUE(appliedActions.empty());
	EXPECT_EQ(controller.currentState(), State::Stable);

	monotonicNowMs = 2000;
	controller.onSample(MakeSnapshot(2000, 2000, 200, 0, 900000, 120, 8, QualityLimitationReason::Bandwidth));

	ASSERT_EQ(appliedActions.size(), 1u);
	EXPECT_EQ(appliedActions[0].type, ActionType::SetEncodingParameters);
	EXPECT_EQ(appliedActions[0].level, 1);
	EXPECT_EQ(controller.currentLevel(), 1);
	EXPECT_EQ(controller.currentState(), State::EarlyWarning);
}

TEST(ClientQosControllerTest, SnapshotReportingUsesConfiguredIntervalAndImmediateTransition) {
	std::vector<nlohmann::json> snapshots;
	int64_t monotonicNowMs = 0;
	int64_t wallNowMs = 1700000000000;
	PublisherQosController::Options options;
	options.source = Source::Camera;
	options.trackId = "video";
	options.producerId = "producer";
	options.initialLevel = 0;
	options.warmupSamples = 0;
	options.snapshotIntervalMs = 5000;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	options.wallNowMs = [&]() { return wallNowMs; };
	options.actionSink = [&](const PlannedAction&) { return true; };
	options.sendSnapshot = [&](const nlohmann::json& snapshot) {
		snapshots.push_back(snapshot);
	};

	PublisherQosController controller(options);

	monotonicNowMs = 1000;
	wallNowMs = 1700000001000;
	controller.onSample(MakeSnapshot(1000, 20000, 100, 0));

	monotonicNowMs = 2000;
	wallNowMs = 1700000002000;
	controller.onSample(MakeSnapshot(2000, 40000, 200, 0));

	monotonicNowMs = 3000;
	wallNowMs = 1700000003000;
	controller.onSample(MakeSnapshot(3000, 41000, 300, 0, 900000, 120, 8, QualityLimitationReason::Bandwidth));

	monotonicNowMs = 4000;
	wallNowMs = 1700000004000;
	controller.onSample(MakeSnapshot(4000, 42000, 400, 0, 900000, 120, 8, QualityLimitationReason::Bandwidth));

	monotonicNowMs = 9000;
	wallNowMs = 1700000009000;
	controller.onSample(MakeSnapshot(9000, 82000, 500, 0));

	ASSERT_EQ(snapshots.size(), 2u);
	EXPECT_EQ(snapshots[0]["tsMs"].get<int64_t>(), 1700000004000);
	EXPECT_EQ(snapshots[1]["tsMs"].get<int64_t>(), 1700000009000);
	EXPECT_EQ(snapshots[0]["tracks"][0]["state"].get<std::string>(), "early_warning");
	EXPECT_EQ(snapshots[1]["tracks"][0]["state"].get<std::string>(), "early_warning");
}

TEST(ClientQosControllerTest, OverrideClampSuppressesDegradeUntilCleared) {
	std::vector<PlannedAction> appliedActions;
	int64_t monotonicNowMs = 0;
	PublisherQosController::Options options;
	options.source = Source::Camera;
	options.trackId = "video";
	options.producerId = "producer";
	options.initialLevel = 0;
	options.warmupSamples = 0;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	options.actionSink = [&](const PlannedAction& action) {
		appliedActions.push_back(action);
		return true;
	};

	PublisherQosController controller(options);
	controller.setOverrideClampLevel(0);

	monotonicNowMs = 1000;
	controller.onSample(MakeSnapshot(1000, 1000, 100, 0, 900000, 120, 8, QualityLimitationReason::Bandwidth));

	monotonicNowMs = 2000;
	controller.onSample(MakeSnapshot(2000, 2000, 200, 0, 900000, 120, 8, QualityLimitationReason::Bandwidth));

	EXPECT_TRUE(appliedActions.empty());
	EXPECT_EQ(controller.currentLevel(), 0);

	controller.clearOverride();

	monotonicNowMs = 3000;
	controller.onSample(MakeSnapshot(3000, 3000, 300, 0, 900000, 120, 8, QualityLimitationReason::Bandwidth));

	ASSERT_EQ(appliedActions.size(), 1u);
	EXPECT_EQ(appliedActions[0].type, ActionType::SetEncodingParameters);
	EXPECT_EQ(appliedActions[0].level, 1);
	EXPECT_EQ(controller.currentLevel(), 1);
}

TEST(ClientQosControllerTest, AppliesForceAudioOnlyOverrideWhenVideoPauseAllowed) {
	std::vector<PlannedAction> appliedActions;
	int64_t monotonicNowMs = 0;
	PublisherQosController::Options options;
	options.source = Source::Camera;
	options.trackId = "video";
	options.producerId = "producer";
	options.initialLevel = 0;
	options.warmupSamples = 0;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	options.actionSink = [&](const PlannedAction& action) {
		appliedActions.push_back(action);
		return true;
	};

	PublisherQosController controller(options);
	QosOverride overrideData;
	overrideData.scope = OverrideScope::Peer;
	overrideData.forceAudioOnly = true;
	overrideData.ttlMs = 10000;
	overrideData.reason = "room-protection";
	controller.handleOverride(overrideData);

	monotonicNowMs = 1000;
	controller.onSample(MakeSnapshot(1000, 1000, 100, 0));

	// filterActions injects enterAudioOnly at current level (JS parity)
	bool hasEnterAudioOnly = false;
	for (auto& a : appliedActions) {
		if (a.type == ActionType::EnterAudioOnly) { hasEnterAudioOnly = true; break; }
	}
	EXPECT_TRUE(hasEnterAudioOnly);
	EXPECT_TRUE(controller.getTrackState().inAudioOnlyMode);
}

TEST(ClientQosControllerTest, IgnoresForceAudioOnlyOverrideWhenPolicyDisablesVideoPause) {
	std::vector<PlannedAction> appliedActions;
	int64_t monotonicNowMs = 0;
	PublisherQosController::Options options;
	options.source = Source::Camera;
	options.trackId = "video";
	options.producerId = "producer";
	options.initialLevel = 0;
	options.warmupSamples = 0;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	options.actionSink = [&](const PlannedAction& action) {
		appliedActions.push_back(action);
		return true;
	};

	PublisherQosController controller(options);
	QosPolicy policy;
	policy.sampleIntervalMs = 1000;
	policy.snapshotIntervalMs = 1000;
	policy.allowAudioOnly = false;
	policy.allowVideoPause = false;
	policy.profiles = {"conservative", "clarity-first", "speech-first"};
	controller.handlePolicy(policy);

	QosOverride overrideData;
	overrideData.scope = OverrideScope::Peer;
	overrideData.forceAudioOnly = true;
	overrideData.ttlMs = 10000;
	overrideData.reason = "room-protection";
	controller.handleOverride(overrideData);

	monotonicNowMs = 1000;
	controller.onSample(MakeSnapshot(1000, 1000, 100, 0));

	EXPECT_TRUE(appliedActions.empty());
	EXPECT_FALSE(controller.getTrackState().inAudioOnlyMode);
}

TEST(ClientQosControllerTest, CameraClampFallsBackToMaxVideoLevelWhenAudioOnlyDisabled) {
	std::vector<PlannedAction> appliedActions;
	int64_t monotonicNowMs = 0;
	PublisherQosController::Options options;
	options.source = Source::Camera;
	options.trackId = "video";
	options.producerId = "producer";
	options.initialLevel = 4;
	options.warmupSamples = 0;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	options.actionSink = [&](const PlannedAction& action) {
		appliedActions.push_back(action);
		return true;
	};

	PublisherQosController controller(options);
	QosPolicy policy;
	policy.allowAudioOnly = false;
	policy.allowVideoPause = false;
	controller.handlePolicy(policy);

	monotonicNowMs = 1000;
	controller.onSample(MakeSnapshot(1000, 1000, 100, 0, 900000, 400, 80, QualityLimitationReason::Bandwidth));

	ASSERT_FALSE(appliedActions.empty());
	EXPECT_EQ(appliedActions.front().type, ActionType::SetEncodingParameters);
	EXPECT_EQ(appliedActions.front().level, 3);
	EXPECT_EQ(controller.currentLevel(), 3);
	EXPECT_FALSE(controller.getTrackState().inAudioOnlyMode);
}

TEST(ClientQosControllerTest, PolicyUpdatesRuntimeSettings) {
	PublisherQosController controller(PublisherQosController::Options{});

	QosPolicy policy;
	policy.sampleIntervalMs = 1500;
	policy.snapshotIntervalMs = 4000;
	policy.allowAudioOnly = false;
	policy.allowVideoPause = false;
	policy.profiles = {"conservative", "clarity-first", "speech-first"};
	controller.handlePolicy(policy);

	auto runtime = controller.getRuntimeSettings();
	EXPECT_EQ(runtime.sampleIntervalMs, 1500);
	EXPECT_EQ(runtime.snapshotIntervalMs, 4000);
	EXPECT_FALSE(runtime.allowAudioOnly);
	EXPECT_FALSE(runtime.allowVideoPause);
	EXPECT_FALSE(runtime.probeActive);
}

TEST(ClientQosControllerTest, HandlePolicyWithProfilesSwitchesActualProfileParameters) {
	PublisherQosController controller(PublisherQosController::Options{});

	EXPECT_EQ(controller.profileConfig().name, "default-camera");

	QosPolicy policy;
	policy.sampleIntervalMs = 1000;
	policy.snapshotIntervalMs = 2000;
	policy.allowAudioOnly = true;
	policy.allowVideoPause = true;
	policy.profiles = {"conservative", "clarity-first", "speech-first"};
	controller.handlePolicy(policy);

	EXPECT_EQ(controller.profileConfig().name, "conservative");
	EXPECT_EQ(controller.profileConfig().recoveryCooldownMs, 12000);
	EXPECT_DOUBLE_EQ(controller.profileConfig().thresholds.congestedLossRate, 0.06);
}

TEST(ClientQosControllerTest, FailedActionDoesNotAdvanceLocalLevelAndMarksSnapshotUnapplied) {
	std::vector<nlohmann::json> snapshots;
	int64_t monotonicNowMs = 0;
	int64_t wallNowMs = 1700000000000;
	PublisherQosController::Options options;
	options.source = Source::Camera;
	options.trackId = "video";
	options.producerId = "producer";
	options.initialLevel = 0;
	options.warmupSamples = 0;
	options.snapshotIntervalMs = 1000;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	options.wallNowMs = [&]() { return wallNowMs; };
	options.actionSink = [&](const PlannedAction& action) {
		return action.type == ActionType::Noop;
	};
	options.sendSnapshot = [&](const nlohmann::json& snapshot) {
		snapshots.push_back(snapshot);
	};

	PublisherQosController controller(options);

	monotonicNowMs = 1000;
	wallNowMs = 1700000001000;
	controller.onSample(MakeSnapshot(1000, 1000, 100, 0, 900000, 120, 8, QualityLimitationReason::Bandwidth));

	monotonicNowMs = 2000;
	wallNowMs = 1700000002000;
	controller.onSample(MakeSnapshot(2000, 2000, 200, 0, 900000, 120, 8, QualityLimitationReason::Bandwidth));

	ASSERT_EQ(controller.currentState(), State::EarlyWarning);
	EXPECT_EQ(controller.currentLevel(), 0);
	EXPECT_FALSE(controller.lastActionWasApplied());

	ASSERT_GE(snapshots.size(), 2u);
	const auto& lastSnapshot = snapshots.back();
	EXPECT_EQ(lastSnapshot["tracks"][0]["ladderLevel"].get<int>(), 0);
	EXPECT_FALSE(lastSnapshot["tracks"][0]["lastAction"]["applied"].get<bool>());
	EXPECT_EQ(lastSnapshot["tracks"][0]["lastAction"]["type"], "setEncodingParameters");
}

TEST(ClientQosControllerTest, SnapshotUsesVideoOnlyModeWhenPeerHasNoAudioTrack) {
	std::vector<nlohmann::json> snapshots;
	int64_t monotonicNowMs = 1000;
	int64_t wallNowMs = 1700000000000;
	PublisherQosController::Options options;
	options.source = Source::Camera;
	options.trackId = "video";
	options.producerId = "producer";
	options.warmupSamples = 0;
	options.snapshotIntervalMs = 0;
	options.peerHasAudioTrack = false;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	options.wallNowMs = [&]() { return wallNowMs; };
	options.actionSink = [](const PlannedAction&) {
		return true;
	};
	options.sendSnapshot = [&](const nlohmann::json& snapshot) {
		snapshots.push_back(snapshot);
	};

	PublisherQosController controller(options);
	controller.onSample(MakeSnapshot(1000, 1000, 100, 0));
	monotonicNowMs = 2000;
	wallNowMs = 1700000001000;
	controller.onSample(MakeSnapshot(2000, 2000, 200, 0));

	ASSERT_FALSE(snapshots.empty());
	EXPECT_EQ(snapshots.back()["peerState"]["mode"], "video-only");
}

TEST(ClientQosControllerTest, OverrideClearRemovesMatchingOverridesByReasonPrefix) {
	int64_t monotonicNowMs = 1000;
	PublisherQosController::Options options;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	PublisherQosController controller(options);

	QosOverride autoPoor;
	autoPoor.scope = OverrideScope::Peer;
	autoPoor.reason = "server_auto_poor";
	autoPoor.ttlMs = 10000;
	autoPoor.maxLevelClamp = 2;
	controller.handleOverride(autoPoor);

	QosOverride roomPressure;
	roomPressure.scope = OverrideScope::Peer;
	roomPressure.reason = "server_room_pressure";
	roomPressure.ttlMs = 8000;
	roomPressure.maxLevelClamp = 1;
	controller.handleOverride(roomPressure);

	auto active = controller.getActiveOverride(monotonicNowMs);
	ASSERT_TRUE(active.has_value());
	ASSERT_TRUE(active->maxLevelClamp.has_value());
	EXPECT_EQ(*active->maxLevelClamp, 1);

	QosOverride clearAuto;
	clearAuto.scope = OverrideScope::Peer;
	clearAuto.reason = "server_auto_clear";
	clearAuto.ttlMs = 0;
	controller.handleOverride(clearAuto);

	active = controller.getActiveOverride(monotonicNowMs);
	ASSERT_TRUE(active.has_value());
	EXPECT_EQ(active->reason, "server_room_pressure");
	ASSERT_TRUE(active->maxLevelClamp.has_value());
	EXPECT_EQ(*active->maxLevelClamp, 1);

	QosOverride clearRoom;
	clearRoom.scope = OverrideScope::Peer;
	clearRoom.reason = "server_room_pressure_clear";
	clearRoom.ttlMs = 0;
	controller.handleOverride(clearRoom);

	EXPECT_FALSE(controller.getActiveOverride(monotonicNowMs).has_value());
}

TEST(ClientQosControllerTest, ServerTtlExpiredClearsAllActiveOverrides) {
	int64_t monotonicNowMs = 1000;
	PublisherQosController::Options options;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	PublisherQosController controller(options);

	QosOverride lost;
	lost.scope = OverrideScope::Peer;
	lost.reason = "server_auto_lost";
	lost.ttlMs = 15000;
	lost.forceAudioOnly = true;
	controller.handleOverride(lost);

	QosOverride pressure;
	pressure.scope = OverrideScope::Peer;
	pressure.reason = "server_room_pressure";
	pressure.ttlMs = 8000;
	pressure.maxLevelClamp = 1;
	controller.handleOverride(pressure);

	EXPECT_TRUE(controller.getActiveOverride(monotonicNowMs).has_value());

	QosOverride clearAll;
	clearAll.scope = OverrideScope::Peer;
	clearAll.reason = "server_ttl_expired";
	clearAll.ttlMs = 0;
	controller.handleOverride(clearAll);

	EXPECT_FALSE(controller.getActiveOverride(monotonicNowMs).has_value());
}

TEST(ClientQosControllerTest, ManualClearRemovesManualOverridesWithoutClearingServerOverrides) {
	int64_t monotonicNowMs = 1000;
	PublisherQosController::Options options;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	PublisherQosController controller(options);

	QosOverride manual;
	manual.scope = OverrideScope::Peer;
	manual.reason = "manual_protection";
	manual.ttlMs = 10000;
	manual.forceAudioOnly = true;
	controller.handleOverride(manual);

	QosOverride server;
	server.scope = OverrideScope::Peer;
	server.reason = "server_room_pressure";
	server.ttlMs = 8000;
	server.maxLevelClamp = 1;
	controller.handleOverride(server);

	auto active = controller.getActiveOverride(monotonicNowMs);
	ASSERT_TRUE(active.has_value());
	EXPECT_TRUE(active->forceAudioOnly.value_or(false));
	ASSERT_TRUE(active->maxLevelClamp.has_value());
	EXPECT_EQ(*active->maxLevelClamp, 1);

	QosOverride clearManual;
	clearManual.scope = OverrideScope::Peer;
	clearManual.reason = "manual_clear";
	clearManual.ttlMs = 0;
	controller.handleOverride(clearManual);

	active = controller.getActiveOverride(monotonicNowMs);
	ASSERT_TRUE(active.has_value());
	EXPECT_FALSE(active->forceAudioOnly.value_or(false));
	ASSERT_TRUE(active->maxLevelClamp.has_value());
	EXPECT_EQ(*active->maxLevelClamp, 1);
	EXPECT_EQ(active->reason, "server_room_pressure");
}

TEST(ClientQosControllerTest, ManualClearExitsOverrideDrivenAudioOnlyAndRestoresLocalControl) {
	std::vector<PlannedAction> appliedActions;
	int64_t monotonicNowMs = 0;
	PublisherQosController::Options options;
	options.source = Source::Camera;
	options.trackId = "video";
	options.producerId = "producer";
	options.initialLevel = 0;
	options.warmupSamples = 0;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	options.actionSink = [&](const PlannedAction& action) {
		appliedActions.push_back(action);
		return true;
	};

	PublisherQosController controller(options);
	QosOverride manual;
	manual.scope = OverrideScope::Peer;
	manual.reason = "manual_protection";
	manual.ttlMs = 10000;
	manual.forceAudioOnly = true;
	controller.handleOverride(manual);

	monotonicNowMs = 1000;
	controller.onSample(MakeSnapshot(1000, 1000, 100, 0));
	EXPECT_TRUE(controller.getTrackState().inAudioOnlyMode);

	QosOverride clearManual;
	clearManual.scope = OverrideScope::Peer;
	clearManual.reason = "manual_clear";
	clearManual.ttlMs = 0;
	controller.handleOverride(clearManual);

	monotonicNowMs = 2000;
	controller.onSample(MakeSnapshot(2000, 2000, 200, 0));

	EXPECT_TRUE(appliedActions.size() >= 2u);
	EXPECT_TRUE(std::any_of(appliedActions.begin(), appliedActions.end(), [](const PlannedAction& action) {
		return action.type == ActionType::ExitAudioOnly;
	}));
	EXPECT_FALSE(controller.getTrackState().inAudioOnlyMode);
}

TEST(ClientQosControllerTest, TrackScopedOverrideIgnoresOtherTrackId) {
	int64_t monotonicNowMs = 1000;
	PublisherQosController::Options options;
	options.trackId = "track-1";
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	PublisherQosController controller(options);

	QosOverride trackOverride;
	trackOverride.scope = OverrideScope::Track;
	trackOverride.trackId = std::string("track-2");
	trackOverride.reason = "downlink_v2_room_demand";
	trackOverride.ttlMs = 5000;
	trackOverride.maxLevelClamp = 1;
	controller.handleOverride(trackOverride);

	EXPECT_FALSE(controller.getActiveOverride(monotonicNowMs).has_value());
}

TEST(ClientQosControllerTest, TrackScopedOverrideAppliesOnMatchingTrackId) {
	int64_t monotonicNowMs = 1000;
	PublisherQosController::Options options;
	options.trackId = "track-1";
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	PublisherQosController controller(options);

	QosOverride trackOverride;
	trackOverride.scope = OverrideScope::Track;
	trackOverride.trackId = std::string("track-1");
	trackOverride.reason = "downlink_v2_room_demand";
	trackOverride.ttlMs = 5000;
	trackOverride.maxLevelClamp = 1;
	controller.handleOverride(trackOverride);

	auto active = controller.getActiveOverride(monotonicNowMs);
	ASSERT_TRUE(active.has_value());
	ASSERT_TRUE(active->maxLevelClamp.has_value());
	EXPECT_EQ(*active->maxLevelClamp, 1);
	EXPECT_EQ(active->reason, "downlink_v2_room_demand");
}

TEST(ClientQosControllerTest, TrackScopedClearPreservesUnrelatedManualOverride) {
	int64_t monotonicNowMs = 1000;
	PublisherQosController::Options options;
	options.trackId = "track-1";
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	PublisherQosController controller(options);

	QosOverride manual;
	manual.scope = OverrideScope::Peer;
	manual.reason = "manual_protection";
	manual.ttlMs = 10000;
	manual.forceAudioOnly = true;
	controller.handleOverride(manual);

	QosOverride trackOverride;
	trackOverride.scope = OverrideScope::Track;
	trackOverride.trackId = std::string("track-1");
	trackOverride.reason = "downlink_v2_room_demand";
	trackOverride.ttlMs = 5000;
	trackOverride.maxLevelClamp = 1;
	controller.handleOverride(trackOverride);

	auto active = controller.getActiveOverride(monotonicNowMs);
	ASSERT_TRUE(active.has_value());
	EXPECT_TRUE(active->forceAudioOnly.value_or(false));
	ASSERT_TRUE(active->maxLevelClamp.has_value());
	EXPECT_EQ(*active->maxLevelClamp, 1);

	QosOverride clearTrack;
	clearTrack.scope = OverrideScope::Track;
	clearTrack.trackId = std::string("track-1");
	clearTrack.reason = "downlink_v2_demand_restored";
	clearTrack.ttlMs = 0;
	controller.handleOverride(clearTrack);

	active = controller.getActiveOverride(monotonicNowMs);
	ASSERT_TRUE(active.has_value());
	EXPECT_TRUE(active->forceAudioOnly.value_or(false));
	EXPECT_FALSE(active->maxLevelClamp.has_value());
	EXPECT_EQ(active->reason, "manual_protection");
}

TEST(ClientQosControllerTest, CoordinationBitrateCapInjectsEncodingActionAtCurrentLevel) {
	std::vector<PlannedAction> appliedActions;
	int64_t monotonicNowMs = 1000;
	PublisherQosController::Options options;
	options.source = Source::Camera;
	options.trackId = "video";
	options.producerId = "producer";
	options.initialLevel = 0;
	options.warmupSamples = 0;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	options.actionSink = [&](const PlannedAction& action) {
		appliedActions.push_back(action);
		return true;
	};

	PublisherQosController controller(options);
	CoordinationOverride override;
	override.maxBitrateCapBps = 300000;
	controller.setCoordinationOverride(override);
	controller.onSample(MakeSnapshot(1000, 1000, 100, 0));

	ASSERT_EQ(appliedActions.size(), 1u);
	EXPECT_EQ(appliedActions[0].type, ActionType::SetEncodingParameters);
	ASSERT_TRUE(appliedActions[0].encodingParameters.has_value());
	ASSERT_TRUE(appliedActions[0].encodingParameters->maxBitrateBps.has_value());
	EXPECT_EQ(*appliedActions[0].encodingParameters->maxBitrateBps, 300000u);
}

TEST(ClientQosControllerTest, ClearingCoordinationBitrateCapRestoresCurrentLevelEncodingParameters) {
	std::vector<PlannedAction> appliedActions;
	int64_t monotonicNowMs = 1000;
	PublisherQosController::Options options;
	options.source = Source::Camera;
	options.trackId = "video";
	options.producerId = "producer";
	options.initialLevel = 0;
	options.warmupSamples = 0;
	options.monotonicNowMs = [&]() { return monotonicNowMs; };
	options.actionSink = [&](const PlannedAction& action) {
		appliedActions.push_back(action);
		return true;
	};

	PublisherQosController controller(options);
	CoordinationOverride override;
	override.maxBitrateCapBps = 300000;
	controller.setCoordinationOverride(override);
	controller.onSample(MakeSnapshot(1000, 1000, 100, 0));

	controller.setCoordinationOverride(std::nullopt);
	monotonicNowMs = 2000;
	controller.onSample(MakeSnapshot(2000, 2000, 200, 0));

	ASSERT_EQ(appliedActions.size(), 2u);
	EXPECT_EQ(appliedActions[1].type, ActionType::SetEncodingParameters);
	ASSERT_TRUE(appliedActions[1].encodingParameters.has_value());
	ASSERT_TRUE(appliedActions[1].encodingParameters->maxBitrateBps.has_value());
	EXPECT_EQ(*appliedActions[1].encodingParameters->maxBitrateBps, 900000u);
}
