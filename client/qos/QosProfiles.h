#pragma once
#include "QosConstants.h"
#include "QosTypes.h"
#include <optional>
#include <string>
#include <vector>

namespace qos {

inline Thresholds makeDefaultThresholds() {
	Thresholds thresholds;
	thresholds.warnLossRate = 0.04;
	thresholds.congestedLossRate = 0.08;
	thresholds.warnRttMs = 220;
	thresholds.congestedRttMs = 320;
	thresholds.warnJitterMs = 30;
	thresholds.congestedJitterMs = 60;
	thresholds.warnBitrateUtilization = 0.85;
	thresholds.congestedBitrateUtilization = 0.65;
	thresholds.stableLossRate = 0.03;
	thresholds.stableRttMs = 180;
	thresholds.stableJitterMs = 20;
	thresholds.stableBitrateUtilization = 0.92;
	return thresholds;
}

inline Profile makeCameraProfile() {
	Profile p;
	p.name = "default-camera";
	p.source = Source::Camera;
	p.levelCount = 5;
	p.sampleIntervalMs = DEFAULT_SAMPLE_INTERVAL_MS;
	p.snapshotIntervalMs = DEFAULT_SNAPSHOT_INTERVAL_MS;
	p.recoveryCooldownMs = DEFAULT_RECOVERY_COOLDOWN_MS;
	p.thresholds = makeDefaultThresholds();
	p.ladder = {
		{0, "Use publish initial encoding settings.", EncodingParameters{900000, 30, 1.0, {}}, {}, false, false, false, false},
		{1, "Reduce bitrate and frame rate slightly.", EncodingParameters{850000, 24, 1.0, {}}, {}, false, false, false, false},
		{2, "Further reduce bitrate/frame rate and start downscaling resolution.", EncodingParameters{700000, 20, 1.5, {}}, {}, false, false, false, false},
		{3, "Aggressive bitrate and fps reduction, force lowest spatial layer if available.", EncodingParameters{450000, 12, 2.0, {}}, 0, false, false, false, false},
		{4, "Enter audio-only mode and pause video upstream.", {}, {}, true, false, false, false},
	};
	return p;
}

inline Profile makeScreenShareProfile() {
	Profile p;
	p.name = "default-screenshare";
	p.source = Source::ScreenShare;
	p.levelCount = 5;
	p.sampleIntervalMs = DEFAULT_SAMPLE_INTERVAL_MS;
	p.snapshotIntervalMs = DEFAULT_SNAPSHOT_INTERVAL_MS;
	p.recoveryCooldownMs = 10000;
	p.thresholds = makeDefaultThresholds();
	p.ladder = {
		{0, "Use publish initial encoding settings.", EncodingParameters{1800000, 15, 1.0, {}}, {}, false, false, false, false},
		{1, "Lower capture/send framerate to 10.", EncodingParameters{1800000, 10, 1.0, {}}, {}, false, false, false, false},
		{2, "Lower framerate to 5 while prioritizing text readability.", EncodingParameters{1350000, 5, 1.0, {}}, {}, false, false, false, false},
		{3, "Further limit bitrate and framerate before resolution downgrade.", EncodingParameters{990000, 3, 1.25, {}}, {}, false, false, false, true},
		{4, "Pause screen share and rely on coordinator decision.", {}, {}, false, false, true, false},
	};
	return p;
}

inline Profile makeAudioProfile() {
	Profile p;
	p.name = "default-audio";
	p.source = Source::Audio;
	p.levelCount = 5;
	p.sampleIntervalMs = DEFAULT_SAMPLE_INTERVAL_MS;
	p.snapshotIntervalMs = DEFAULT_SNAPSHOT_INTERVAL_MS;
	p.recoveryCooldownMs = DEFAULT_RECOVERY_COOLDOWN_MS;
	p.thresholds = makeDefaultThresholds();
	p.ladder = {
		{0, "Use publish initial encoding settings.", EncodingParameters{510000, {}, {}, {}}, {}, false, false, false, false},
		{1, "Reduce max bitrate to around 32kbps, enable adaptive ptime if available.", EncodingParameters{32000, {}, {}, true}, {}, false, false, false, false},
		{2, "Reduce max bitrate to around 24kbps.", EncodingParameters{24000, {}, {}, {}}, {}, false, false, false, false},
		{3, "Reduce max bitrate to around 16kbps.", EncodingParameters{16000, {}, {}, {}}, {}, false, false, false, false},
		{4, "Keep audio alive at minimum quality, avoid auto-mute.", EncodingParameters{16000, {}, {}, true}, {}, false, false, false, false},
	};
	return p;
}

inline Profile makeConservativeCameraProfile() {
	Profile p = makeCameraProfile();
	p.name = "conservative";
	p.recoveryCooldownMs = 12000;
	p.thresholds.warnLossRate = 0.03;
	p.thresholds.congestedLossRate = 0.06;
	p.thresholds.warnRttMs = 180;
	p.thresholds.congestedRttMs = 260;
	p.ladder[2].encodingParameters = EncodingParameters{550000, 15, 1.5, {}};
	return p;
}

inline std::optional<Profile> resolveProfileByName(Source source, const std::string& name) {
	if (name.empty()) return std::nullopt;
	switch (source) {
		case Source::Camera:
			if (name == "default" || name == "default-camera") return makeCameraProfile();
			if (name == "conservative") return makeConservativeCameraProfile();
			break;
		case Source::ScreenShare:
			if (name == "default" || name == "default-screenshare" || name == "clarity-first") return makeScreenShareProfile();
			break;
		case Source::Audio:
			if (name == "default" || name == "default-audio" || name == "speech-first") return makeAudioProfile();
			break;
	}
	return std::nullopt;
}

inline Profile resolveProfile(Source source, const std::optional<Profile>& override = std::nullopt) {
	if (override.has_value()) return *override;
	switch (source) {
		case Source::ScreenShare: return makeScreenShareProfile();
		case Source::Audio: return makeAudioProfile();
		default: return makeCameraProfile();
	}
}

inline const Profile& getProfile(Source source) {
	static Profile camera = makeCameraProfile();
	static Profile screen = makeScreenShareProfile();
	static Profile audio = makeAudioProfile();
	switch (source) {
		case Source::ScreenShare: return screen;
		case Source::Audio: return audio;
		default: return camera;
	}
}

} // namespace qos
