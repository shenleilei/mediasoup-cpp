#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace qos {

enum class Source { Camera, ScreenShare, Audio };
enum class Reason { Network, Cpu, Manual, ServerOverride, Unknown };
enum class State { Stable, EarlyWarning, Congested, Recovering };
enum class Quality { Excellent, Good, Poor, Lost };
enum class TrackKind { Audio, Video };
enum class TrackMode { AudioOnly, AudioVideo };
enum class ActionType { SetEncodingParameters, SetMaxSpatialLayer, EnterAudioOnly, ExitAudioOnly, PauseUpstream, ResumeUpstream, Noop };
enum class QualityLimitationReason { Bandwidth, Cpu, Other, None, Unknown };
enum class OverrideScope { Peer, Track };

struct EncodingParameters {
	std::optional<uint32_t> maxBitrateBps;
	std::optional<uint32_t> maxFramerate;
	std::optional<double> scaleResolutionDownBy;
	std::optional<bool> adaptivePtime;
};

struct RawSenderSnapshot {
	int64_t timestampMs = 0;
	std::string trackId;
	std::string producerId;
	Source source = Source::Camera;
	TrackKind kind = TrackKind::Video;
	uint64_t bytesSent = 0;
	uint64_t packetsSent = 0;
	uint64_t packetsLost = 0;
	uint64_t retransmittedPacketsSent = 0;
	double targetBitrateBps = 0;
	double configuredBitrateBps = 0;
	double roundTripTimeMs = -1; // -1 = unavailable
	double jitterMs = -1;
	int frameWidth = 0;
	int frameHeight = 0;
	double framesPerSecond = 0;
	QualityLimitationReason qualityLimitationReason = QualityLimitationReason::None;
};

struct DerivedSignals {
	uint64_t packetsSentDelta = 0;
	uint64_t packetsLostDelta = 0;
	uint64_t retransmittedPacketsSentDelta = 0;
	double sendBitrateBps = 0;
	double targetBitrateBps = 0;
	double bitrateUtilization = 0;
	double lossRate = 0;
	double lossEwma = 0;
	double rttMs = 0;
	double rttEwma = 0;
	double jitterMs = 0;
	double jitterEwma = 0;
	bool cpuLimited = false;
	bool bandwidthLimited = false;
	Reason reason = Reason::Unknown;
};

struct Thresholds {
	double warnLossRate = 0.04;
	double congestedLossRate = 0.08;
	double warnRttMs = 220;
	double congestedRttMs = 320;
	double warnJitterMs = 30;
	double congestedJitterMs = 60;
	double warnBitrateUtilization = 0.85;
	double congestedBitrateUtilization = 0.65;
	double stableLossRate = 0.03;
	double stableRttMs = 180;
	double stableJitterMs = 20;
	double stableBitrateUtilization = 0.92;
};

struct LadderStep {
	int level = 0;
	std::string description;
	std::optional<EncodingParameters> encodingParameters;
	std::optional<int> spatialLayer;
	bool enterAudioOnly = false;
	bool exitAudioOnly = false;
	bool pauseUpstream = false;
	bool resumeUpstream = false;
};

struct Profile {
	std::string name;
	Source source = Source::Camera;
	int levelCount = 1;
	int sampleIntervalMs = 1000;
	int snapshotIntervalMs = 2000;
	int recoveryCooldownMs = 8000;
	Thresholds thresholds;
	std::vector<LadderStep> ladder;
};

struct ProfileSelection {
	std::string camera;
	std::string screenShare;
	std::string audio;
};

struct QosPolicy {
	std::string schema = "mediasoup.qos.policy.v1";
	int sampleIntervalMs = 1000;
	int snapshotIntervalMs = 2000;
	bool allowAudioOnly = true;
	bool allowVideoPause = true;
	ProfileSelection profiles;
};

struct QosOverride {
	std::string schema = "mediasoup.qos.override.v1";
	OverrideScope scope = OverrideScope::Peer;
	std::optional<std::string> trackId;
	std::optional<int> maxLevelClamp;
	std::optional<bool> forceAudioOnly;
	std::optional<bool> disableRecovery;
	std::optional<bool> pauseUpstream;
	std::optional<bool> resumeUpstream;
	int ttlMs = 0;
	std::string reason;
};

struct RuntimeSettings {
	int sampleIntervalMs = 1000;
	int snapshotIntervalMs = 2000;
	bool allowAudioOnly = true;
	bool allowVideoPause = true;
	bool probeActive = false;
};

constexpr size_t kQosMaxTracksPerSnapshot = 32u;

struct StateMachineContext {
	State state = State::Stable;
	int64_t enteredAtMs = 0;
	int64_t lastCongestedAtMs = 0;
	int64_t lastRecoveryAtMs = 0;
	int consecutiveHealthySamples = 0;
	int consecutiveRecoverySamples = 0;
	int consecutiveFastRecoverySamples = 0;
	int consecutiveWarningSamples = 0;
	int consecutiveCongestedSamples = 0;
};

struct StateTransitionResult {
	StateMachineContext context;
	Quality quality;
	bool transitioned = false;
};

struct PlannedAction {
	ActionType type = ActionType::Noop;
	int level = 0;
	std::optional<EncodingParameters> encodingParameters;
	std::optional<int> spatialLayer;
	std::string reason;
};

struct PlannerInput {
	Source source = Source::Camera;
	const Profile* profile = nullptr;
	State state = State::Stable;
	int currentLevel = 0;
	int previousLevel = 0;
	std::optional<int> overrideClampLevel;
	bool inAudioOnlyMode = false;
	DerivedSignals signals;
};

struct ProbeContext {
	bool active = false;
	int64_t startedAtMs = 0;
	int previousLevel = 0;
	int targetLevel = 0;
	bool previousAudioOnlyMode = false;
	bool targetAudioOnlyMode = false;
	int healthySamples = 0;
	int badSamples = 0;
	int requiredHealthySamples = 3;
	int requiredBadSamples = 2;
};

enum class ProbeResult { Successful, Failed, Inconclusive };

struct PeerTrackState {
	std::string trackId;
	Source source = Source::Camera;
	TrackKind kind = TrackKind::Video;
	State state = State::Stable;
	Quality quality = Quality::Excellent;
	int level = 0;
	bool inAudioOnlyMode = false;
	std::string producerId;
	Reason reason = Reason::Unknown;
};

struct PeerDecision {
	Quality peerQuality = Quality::Lost;
	std::vector<std::string> prioritizedTrackIds;
	std::vector<std::string> sacrificialTrackIds;
	bool keepAudioAlive = false;
	bool preferScreenShare = false;
	bool allowVideoRecovery = false;
};

struct CoordinationOverride {
	std::optional<int> maxLevelClamp;
	std::optional<bool> disableRecovery;
	std::optional<bool> forceAudioOnly;
	std::optional<bool> pauseUpstream;
	std::optional<bool> resumeUpstream;
	std::optional<uint32_t> maxBitrateCapBps;
};

struct TrackBudgetRequest {
	std::string trackId;
	Source source = Source::Camera;
	TrackKind kind = TrackKind::Video;
	uint32_t minBitrateBps = 0;
	uint32_t desiredBitrateBps = 0;
	uint32_t maxBitrateBps = 0;
	double weight = 1.0;
	bool paused = false;
};

struct TrackBudgetAllocation {
	std::string trackId;
	uint32_t allocatedBitrateBps = 0;
	bool capped = false;
};

struct PeerBudgetDecision {
	uint32_t totalBudgetBps = 0;
	uint32_t allocatedBudgetBps = 0;
	std::vector<TrackBudgetAllocation> allocations;
};

// Quality/State string helpers
inline const char* qualityStr(Quality q) {
	switch (q) { case Quality::Excellent: return "excellent"; case Quality::Good: return "good"; case Quality::Poor: return "poor"; case Quality::Lost: return "lost"; }
	return "unknown";
}
inline const char* stateStr(State s) {
	switch (s) { case State::Stable: return "stable"; case State::EarlyWarning: return "early_warning"; case State::Congested: return "congested"; case State::Recovering: return "recovering"; }
	return "unknown";
}
inline const char* actionStr(ActionType a) {
	switch (a) { case ActionType::SetEncodingParameters: return "setEncodingParameters"; case ActionType::SetMaxSpatialLayer: return "setMaxSpatialLayer"; case ActionType::EnterAudioOnly: return "enterAudioOnly"; case ActionType::ExitAudioOnly: return "exitAudioOnly"; case ActionType::PauseUpstream: return "pauseUpstream"; case ActionType::ResumeUpstream: return "resumeUpstream"; case ActionType::Noop: return "noop"; }
	return "noop";
}
inline const char* sourceStr(Source s) {
	switch (s) { case Source::Camera: return "camera"; case Source::ScreenShare: return "screenShare"; case Source::Audio: return "audio"; }
	return "camera";
}
inline const char* reasonStr(Reason r) {
	switch (r) { case Reason::Network: return "network"; case Reason::Cpu: return "cpu"; case Reason::Manual: return "manual"; case Reason::ServerOverride: return "server_override"; case Reason::Unknown: return "unknown"; }
	return "unknown";
}

} // namespace qos
