"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getDefaultCameraProfile = getDefaultCameraProfile;
exports.getDefaultScreenShareProfile = getDefaultScreenShareProfile;
exports.getDefaultAudioProfile = getDefaultAudioProfile;
exports.resolveProfile = resolveProfile;
const constants_1 = require("./constants");
const DEFAULT_THRESHOLDS = {
    warnLossRate: 0.04,
    congestedLossRate: 0.08,
    warnRttMs: 220,
    congestedRttMs: 320,
    warnJitterMs: 30,
    congestedJitterMs: 60,
    warnBitrateUtilization: 0.85,
    congestedBitrateUtilization: 0.65,
    stableLossRate: 0.03,
    stableRttMs: 180,
    stableJitterMs: 20,
    stableBitrateUtilization: 0.92,
};
const DEFAULT_CAMERA_PROFILE = {
    name: 'default-camera',
    source: 'camera',
    levelCount: 5,
    sampleIntervalMs: constants_1.DEFAULT_SAMPLE_INTERVAL_MS,
    snapshotIntervalMs: constants_1.DEFAULT_SNAPSHOT_INTERVAL_MS,
    recoveryCooldownMs: constants_1.DEFAULT_RECOVERY_COOLDOWN_MS,
    thresholds: DEFAULT_THRESHOLDS,
    ladder: [
        {
            level: 0,
            description: 'Use publish initial encoding settings.',
            encodingParameters: {
                maxBitrateBps: 900000,
                maxFramerate: 30,
                scaleResolutionDownBy: 1,
            },
        },
        {
            level: 1,
            description: 'Reduce bitrate and frame rate slightly.',
            encodingParameters: {
                maxBitrateBps: 850000,
                maxFramerate: 24,
                scaleResolutionDownBy: 1,
            },
        },
        {
            level: 2,
            description: 'Further reduce bitrate/frame rate and start downscaling resolution.',
            encodingParameters: {
                maxBitrateBps: 700000,
                maxFramerate: 20,
                scaleResolutionDownBy: 1.5,
            },
        },
        {
            level: 3,
            description: 'Aggressive bitrate and fps reduction, force lowest spatial layer if available.',
            encodingParameters: {
                maxBitrateBps: 450000,
                maxFramerate: 12,
                scaleResolutionDownBy: 2,
            },
            spatialLayer: 0,
        },
        {
            level: 4,
            description: 'Enter audio-only mode and pause video upstream.',
            enterAudioOnly: true,
        },
    ],
};
const DEFAULT_SCREENSHARE_PROFILE = {
    name: 'default-screenshare',
    source: 'screenShare',
    levelCount: 5,
    sampleIntervalMs: constants_1.DEFAULT_SAMPLE_INTERVAL_MS,
    snapshotIntervalMs: constants_1.DEFAULT_SNAPSHOT_INTERVAL_MS,
    recoveryCooldownMs: 10000,
    thresholds: DEFAULT_THRESHOLDS,
    ladder: [
        {
            level: 0,
            description: 'Use publish initial encoding settings.',
            encodingParameters: {
                maxBitrateBps: 1800000,
                maxFramerate: 15,
                scaleResolutionDownBy: 1,
            },
        },
        {
            level: 1,
            description: 'Lower capture/send framerate to 10.',
            encodingParameters: {
                maxBitrateBps: 1800000,
                maxFramerate: 10,
                scaleResolutionDownBy: 1,
            },
        },
        {
            level: 2,
            description: 'Lower framerate to 5 while prioritizing text readability.',
            encodingParameters: {
                maxBitrateBps: 1350000,
                maxFramerate: 5,
                scaleResolutionDownBy: 1,
            },
        },
        {
            level: 3,
            description: 'Further limit bitrate and framerate before resolution downgrade.',
            encodingParameters: {
                maxBitrateBps: 990000,
                maxFramerate: 3,
                scaleResolutionDownBy: 1.25,
            },
            resumeUpstream: true,
        },
        {
            level: 4,
            description: 'Pause screen share and rely on coordinator decision.',
            pauseUpstream: true,
        },
    ],
};
const DEFAULT_AUDIO_PROFILE = {
    name: 'default-audio',
    source: 'audio',
    levelCount: 5,
    sampleIntervalMs: constants_1.DEFAULT_SAMPLE_INTERVAL_MS,
    snapshotIntervalMs: constants_1.DEFAULT_SNAPSHOT_INTERVAL_MS,
    recoveryCooldownMs: constants_1.DEFAULT_RECOVERY_COOLDOWN_MS,
    thresholds: DEFAULT_THRESHOLDS,
    ladder: [
        {
            level: 0,
            description: 'Use publish initial encoding settings.',
            encodingParameters: {
                maxBitrateBps: 510000,
            },
        },
        {
            level: 1,
            description: 'Reduce max bitrate to around 32kbps, enable adaptive ptime if available.',
            encodingParameters: {
                maxBitrateBps: 32000,
                adaptivePtime: true,
            },
        },
        {
            level: 2,
            description: 'Reduce max bitrate to around 24kbps.',
            encodingParameters: {
                maxBitrateBps: 24000,
            },
        },
        {
            level: 3,
            description: 'Reduce max bitrate to around 16kbps.',
            encodingParameters: {
                maxBitrateBps: 16000,
            },
        },
        {
            level: 4,
            description: 'Keep audio alive at minimum quality, avoid auto-mute.',
            encodingParameters: {
                maxBitrateBps: 16000,
                adaptivePtime: true,
            },
        },
    ],
};
function cloneProfile(profile) {
    return {
        ...profile,
        thresholds: { ...profile.thresholds },
        ladder: profile.ladder.map(step => ({ ...step })),
    };
}
// Named profile variants per source.
const CONSERVATIVE_THRESHOLDS = {
    ...DEFAULT_THRESHOLDS,
    warnLossRate: 0.03,
    congestedLossRate: 0.06,
    warnRttMs: 180,
    congestedRttMs: 260,
};
const CONSERVATIVE_CAMERA_PROFILE = {
    ...DEFAULT_CAMERA_PROFILE,
    name: 'conservative',
    recoveryCooldownMs: 12000,
    thresholds: CONSERVATIVE_THRESHOLDS,
    ladder: [
        DEFAULT_CAMERA_PROFILE.ladder[0],
        DEFAULT_CAMERA_PROFILE.ladder[1],
        { ...DEFAULT_CAMERA_PROFILE.ladder[2], encodingParameters: { maxBitrateBps: 550000, maxFramerate: 15, scaleResolutionDownBy: 1.5 } },
        DEFAULT_CAMERA_PROFILE.ladder[3],
        DEFAULT_CAMERA_PROFILE.ladder[4],
    ],
};
const PROFILE_REGISTRY = {
    camera: {
        'default': DEFAULT_CAMERA_PROFILE,
        'conservative': CONSERVATIVE_CAMERA_PROFILE,
    },
    screenShare: {
        'default': DEFAULT_SCREENSHARE_PROFILE,
        'clarity-first': DEFAULT_SCREENSHARE_PROFILE,
    },
    audio: {
        'default': DEFAULT_AUDIO_PROFILE,
        'speech-first': DEFAULT_AUDIO_PROFILE,
    },
};
function resolveProfileByName(source, name) {
    const sourceProfiles = PROFILE_REGISTRY[source];
    if (!sourceProfiles) return undefined;
    const profile = sourceProfiles[name];
    return profile ? cloneProfile(profile) : undefined;
}
exports.resolveProfileByName = resolveProfileByName;
function getDefaultCameraProfile() {
    return cloneProfile(DEFAULT_CAMERA_PROFILE);
}
function getDefaultScreenShareProfile() {
    return cloneProfile(DEFAULT_SCREENSHARE_PROFILE);
}
function getDefaultAudioProfile() {
    return cloneProfile(DEFAULT_AUDIO_PROFILE);
}
function resolveProfile(source, override) {
    let base;
    switch (source) {
        case 'camera': {
            base = getDefaultCameraProfile();
            break;
        }
        case 'screenShare': {
            base = getDefaultScreenShareProfile();
            break;
        }
        case 'audio': {
            base = getDefaultAudioProfile();
            break;
        }
        default: {
            // Keep this branch for exhaustive safety in case source is widened later.
            base = getDefaultCameraProfile();
        }
    }
    if (!override) {
        return base;
    }
    return {
        ...base,
        ...override,
        thresholds: {
            ...base.thresholds,
            ...(override.thresholds ?? {}),
        },
        ladder: override.ladder
            ? override.ladder.map(step => ({ ...step }))
            : base.ladder,
    };
}
