#pragma once
#include "QosTypes.h"
#include <vector>
#include <unordered_map>
#include <string>

namespace qos {

inline Profile makeCameraProfile() {
	Profile p;
	p.name = "default"; p.source = Source::Camera; p.levelCount = 5;
	p.sampleIntervalMs = 1000; p.snapshotIntervalMs = 2000; p.recoveryCooldownMs = 8000;
	p.thresholds = {0.04, 0.08, 220, 400, 1e9, 1e9, 0.85, 0.65, 0.01, 150, 1e9, 0.9};
	p.ladder = {
		{0, "high quality",    EncodingParameters{900000, 30, 1.0, {}}, {}, false, false, false, false},
		{1, "medium quality",  EncodingParameters{500000, 24, 1.0, {}}, {}, false, false, false, false},
		{2, "low quality",     EncodingParameters{250000, 15, 2.0, {}}, {}, false, false, false, false},
		{3, "very low quality", EncodingParameters{100000, 10, 4.0, {}}, {}, false, false, false, false},
		{4, "audio only",      {}, {}, true, false, false, false},
	};
	return p;
}

inline Profile makeScreenShareProfile() {
	Profile p;
	p.name = "clarity-first"; p.source = Source::ScreenShare; p.levelCount = 5;
	p.sampleIntervalMs = 1000; p.snapshotIntervalMs = 2000; p.recoveryCooldownMs = 8000;
	p.thresholds = {0.04, 0.08, 220, 400, 1e9, 1e9, 0.85, 0.65, 0.01, 150, 1e9, 0.9};
	p.ladder = {
		{0, "full clarity",    EncodingParameters{1800000, 15, 1.0, {}}, {}, false, false, false, false},
		{1, "reduced clarity",  EncodingParameters{1000000, 10, 1.0, {}}, {}, false, false, false, false},
		{2, "low clarity",     EncodingParameters{500000, 5, 2.0, {}}, {}, false, false, false, false},
		{3, "minimal clarity",  EncodingParameters{200000, 3, 4.0, {}}, {}, false, false, false, false},
		{4, "paused",          {}, {}, false, false, true, false},
	};
	return p;
}

inline Profile makeAudioProfile() {
	Profile p;
	p.name = "speech-first"; p.source = Source::Audio; p.levelCount = 5;
	p.sampleIntervalMs = 1000; p.snapshotIntervalMs = 2000; p.recoveryCooldownMs = 8000;
	p.thresholds = {0.05, 0.12, 300, 500, 1e9, 1e9, 0.85, 0.65, 0.02, 200, 1e9, 0.9};
	p.ladder = {
		{0, "high quality",    EncodingParameters{510000, {}, {}, true}, {}, false, false, false, false},
		{1, "medium quality",  EncodingParameters{128000, {}, {}, true}, {}, false, false, false, false},
		{2, "low quality",     EncodingParameters{64000, {}, {}, true}, {}, false, false, false, false},
		{3, "very low quality", EncodingParameters{32000, {}, {}, true}, {}, false, false, false, false},
		{4, "minimal quality",  EncodingParameters{16000, {}, {}, true}, {}, false, false, false, false},
	};
	return p;
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
