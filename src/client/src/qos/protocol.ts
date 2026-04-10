import {
	QOS_CLIENT_SCHEMA_V1,
	QOS_MAX_SEQ,
	QOS_MAX_TRACKS_PER_SNAPSHOT,
	QOS_OVERRIDE_SCHEMA_V1,
	QOS_POLICY_SCHEMA_V1,
} from './constants';
import type {
	ClientQosSnapshot,
	ClientQosTrackAction,
	ClientQosTrackSignals,
	ClientQosTrackSnapshot,
	QosOverride,
	QosPolicy,
	QosQuality,
	QosQualityLimitationReason,
	QosReason,
	QosSource,
	QosState,
	QosTrackKind,
	QosTrackMode,
} from './types';

type Dict = Record<string, unknown>;

function isObject(value: unknown): value is Dict {
	return typeof value === 'object' && value !== null && !Array.isArray(value);
}

function asObject(value: unknown, path: string): Dict {
	if (!isObject(value)) {
		throw new TypeError(`${path} must be an object`);
	}

	return value;
}

function asString(value: unknown, path: string): string {
	if (typeof value !== 'string') {
		throw new TypeError(`${path} must be a string`);
	}

	return value;
}

function asNonEmptyString(value: unknown, path: string): string {
	const s = asString(value, path);

	if (s.length === 0) {
		throw new TypeError(`${path} must be a non-empty string`);
	}

	return s;
}

function asBoolean(value: unknown, path: string): boolean {
	if (typeof value !== 'boolean') {
		throw new TypeError(`${path} must be a boolean`);
	}

	return value;
}

function asFiniteNumber(value: unknown, path: string): number {
	if (typeof value !== 'number' || !Number.isFinite(value)) {
		throw new TypeError(`${path} must be a finite number`);
	}

	return value;
}

function asNonNegativeNumber(value: unknown, path: string): number {
	const n = asFiniteNumber(value, path);

	if (n < 0) {
		throw new TypeError(`${path} must be >= 0`);
	}

	return n;
}

function asInt(value: unknown, path: string): number {
	const n = asFiniteNumber(value, path);

	if (!Number.isInteger(n)) {
		throw new TypeError(`${path} must be an integer`);
	}

	return n;
}

function asNonNegativeInt(value: unknown, path: string): number {
	const n = asInt(value, path);

	if (n < 0) {
		throw new TypeError(`${path} must be >= 0`);
	}

	return n;
}

function optionalNumber(
	obj: Dict,
	key: string,
	path: string,
	options?: { integer?: boolean; nonNegative?: boolean }
): number | undefined {
	if (!(key in obj) || obj[key] === undefined) {
		return undefined;
	}

	const value = obj[key];

	if (options?.integer && options?.nonNegative) {
		return asNonNegativeInt(value, `${path}.${key}`);
	}

	if (options?.integer) {
		return asInt(value, `${path}.${key}`);
	}

	if (options?.nonNegative) {
		return asNonNegativeNumber(value, `${path}.${key}`);
	}

	return asFiniteNumber(value, `${path}.${key}`);
}

function optionalBoolean(
	obj: Dict,
	key: string,
	path: string
): boolean | undefined {
	if (!(key in obj) || obj[key] === undefined) {
		return undefined;
	}

	return asBoolean(obj[key], `${path}.${key}`);
}

function optionalString(
	obj: Dict,
	key: string,
	path: string
): string | undefined {
	if (!(key in obj) || obj[key] === undefined) {
		return undefined;
	}

	return asString(obj[key], `${path}.${key}`);
}

function asStringEnum<T extends string>(
	value: unknown,
	path: string,
	allowed: readonly T[]
): T {
	const s = asString(value, path);

	if (!allowed.includes(s as T)) {
		throw new TypeError(`${path} must be one of: ${allowed.join(', ')}`);
	}

	return s as T;
}

function parseQualityLimitationReason(
	value: unknown,
	path: string
): QosQualityLimitationReason {
	return asStringEnum(value, path, [
		'bandwidth',
		'cpu',
		'other',
		'none',
		'unknown',
	]);
}

function parseTrackSignals(
	value: unknown,
	path: string
): ClientQosTrackSignals {
	const obj = asObject(value, path);

	const sendBitrateBps = asNonNegativeNumber(
		obj.sendBitrateBps,
		`${path}.sendBitrateBps`
	);
	const targetBitrateBps = optionalNumber(obj, 'targetBitrateBps', path, {
		nonNegative: true,
	});
	const lossRate = optionalNumber(obj, 'lossRate', path, { nonNegative: true });
	const rttMs = optionalNumber(obj, 'rttMs', path, { nonNegative: true });
	const jitterMs = optionalNumber(obj, 'jitterMs', path, { nonNegative: true });
	const frameWidth = optionalNumber(obj, 'frameWidth', path, {
		integer: true,
		nonNegative: true,
	});
	const frameHeight = optionalNumber(obj, 'frameHeight', path, {
		integer: true,
		nonNegative: true,
	});
	const framesPerSecond = optionalNumber(obj, 'framesPerSecond', path, {
		nonNegative: true,
	});

	let qualityLimitationReason: QosQualityLimitationReason | undefined;

	if (
		'qualityLimitationReason' in obj &&
		obj.qualityLimitationReason !== undefined
	) {
		qualityLimitationReason = parseQualityLimitationReason(
			obj.qualityLimitationReason,
			`${path}.qualityLimitationReason`
		);
	}

	return {
		sendBitrateBps,
		targetBitrateBps,
		lossRate,
		rttMs,
		jitterMs,
		frameWidth,
		frameHeight,
		framesPerSecond,
		qualityLimitationReason,
	};
}

function parseTrackAction(value: unknown, path: string): ClientQosTrackAction {
	const obj = asObject(value, path);
	const type = asStringEnum(obj.type, `${path}.type`, [
		'setEncodingParameters',
		'setMaxSpatialLayer',
		'enterAudioOnly',
		'exitAudioOnly',
		'pauseUpstream',
		'resumeUpstream',
		'noop',
	]);
	const applied = asBoolean(obj.applied, `${path}.applied`);

	return { type, applied };
}

function parseTrackSnapshot(
	value: unknown,
	path: string
): ClientQosTrackSnapshot {
	const obj = asObject(value, path);

	const localTrackId = asNonEmptyString(
		obj.localTrackId,
		`${path}.localTrackId`
	);
	const producerIdRaw = optionalString(obj, 'producerId', path);
	const producerId =
		producerIdRaw === undefined || producerIdRaw.length > 0
			? producerIdRaw
			: (() => {
					throw new TypeError(
						`${path}.producerId must be non-empty when provided`
					);
				})();
	const kind = asStringEnum<QosTrackKind>(obj.kind, `${path}.kind`, [
		'audio',
		'video',
	]);
	const source = asStringEnum<QosSource>(obj.source, `${path}.source`, [
		'camera',
		'screenShare',
		'audio',
	]);
	const state = asStringEnum<QosState>(obj.state, `${path}.state`, [
		'stable',
		'early_warning',
		'congested',
		'recovering',
	]);
	const reason = asStringEnum<QosReason>(obj.reason, `${path}.reason`, [
		'network',
		'cpu',
		'manual',
		'server_override',
		'unknown',
	]);
	const quality = asStringEnum<QosQuality>(obj.quality, `${path}.quality`, [
		'excellent',
		'good',
		'poor',
		'lost',
	]);
	const ladderLevel = asNonNegativeInt(obj.ladderLevel, `${path}.ladderLevel`);
	const signals = parseTrackSignals(obj.signals, `${path}.signals`);

	let lastAction: ClientQosTrackAction | undefined;

	if ('lastAction' in obj && obj.lastAction !== undefined) {
		lastAction = parseTrackAction(obj.lastAction, `${path}.lastAction`);
	}

	return {
		localTrackId,
		producerId,
		kind,
		source,
		state,
		reason,
		quality,
		ladderLevel,
		signals,
		lastAction,
	};
}

export function parseClientQosSnapshot(payload: unknown): ClientQosSnapshot {
	const obj = asObject(payload, 'clientQosSnapshot');

	const schema = asString(obj.schema, 'clientQosSnapshot.schema');

	if (schema !== QOS_CLIENT_SCHEMA_V1) {
		throw new TypeError(
			`clientQosSnapshot.schema must be '${QOS_CLIENT_SCHEMA_V1}', got '${schema}'`
		);
	}

	const seq = asNonNegativeInt(obj.seq, 'clientQosSnapshot.seq');

	if (seq > QOS_MAX_SEQ) {
		throw new TypeError(`clientQosSnapshot.seq must be <= ${QOS_MAX_SEQ}`);
	}

	const tsMs = asNonNegativeNumber(obj.tsMs, 'clientQosSnapshot.tsMs');

	const peerStateObj = asObject(obj.peerState, 'clientQosSnapshot.peerState');
	const mode = asStringEnum<QosTrackMode>(
		peerStateObj.mode,
		'clientQosSnapshot.peerState.mode',
		['audio-only', 'audio-video', 'video-only']
	);
	const quality = asStringEnum<QosQuality>(
		peerStateObj.quality,
		'clientQosSnapshot.peerState.quality',
		['excellent', 'good', 'poor', 'lost']
	);
	const stale = asBoolean(
		peerStateObj.stale,
		'clientQosSnapshot.peerState.stale'
	);

	const tracksRaw = obj.tracks;

	if (!Array.isArray(tracksRaw)) {
		throw new TypeError('clientQosSnapshot.tracks must be an array');
	}
	if (tracksRaw.length > QOS_MAX_TRACKS_PER_SNAPSHOT) {
		throw new TypeError(
			`clientQosSnapshot.tracks length must be <= ${QOS_MAX_TRACKS_PER_SNAPSHOT}`
		);
	}

	const tracks = tracksRaw.map((item, index) =>
		parseTrackSnapshot(item, `clientQosSnapshot.tracks[${index}]`)
	);

	return {
		schema: QOS_CLIENT_SCHEMA_V1,
		seq,
		tsMs,
		peerState: {
			mode,
			quality,
			stale,
		},
		tracks,
	};
}

export function parseQosPolicy(payload: unknown): QosPolicy {
	const obj = asObject(payload, 'qosPolicy');

	const schema = asString(obj.schema, 'qosPolicy.schema');

	if (schema !== QOS_POLICY_SCHEMA_V1) {
		throw new TypeError(
			`qosPolicy.schema must be '${QOS_POLICY_SCHEMA_V1}', got '${schema}'`
		);
	}

	const sampleIntervalMs = asNonNegativeInt(
		obj.sampleIntervalMs,
		'qosPolicy.sampleIntervalMs'
	);
	const snapshotIntervalMs = asNonNegativeInt(
		obj.snapshotIntervalMs,
		'qosPolicy.snapshotIntervalMs'
	);
	const allowAudioOnly = asBoolean(
		obj.allowAudioOnly,
		'qosPolicy.allowAudioOnly'
	);
	const allowVideoPause = asBoolean(
		obj.allowVideoPause,
		'qosPolicy.allowVideoPause'
	);

	const profilesObj = asObject(obj.profiles, 'qosPolicy.profiles');
	const camera = asNonEmptyString(
		profilesObj.camera,
		'qosPolicy.profiles.camera'
	);
	const screenShare = asNonEmptyString(
		profilesObj.screenShare,
		'qosPolicy.profiles.screenShare'
	);
	const audio = asNonEmptyString(profilesObj.audio, 'qosPolicy.profiles.audio');

	return {
		schema: QOS_POLICY_SCHEMA_V1,
		sampleIntervalMs,
		snapshotIntervalMs,
		allowAudioOnly,
		allowVideoPause,
		profiles: {
			camera,
			screenShare,
			audio,
		},
	};
}

export function parseQosOverride(payload: unknown): QosOverride {
	const obj = asObject(payload, 'qosOverride');

	const schema = asString(obj.schema, 'qosOverride.schema');

	if (schema !== QOS_OVERRIDE_SCHEMA_V1) {
		throw new TypeError(
			`qosOverride.schema must be '${QOS_OVERRIDE_SCHEMA_V1}', got '${schema}'`
		);
	}

	const scope = asStringEnum(obj.scope, 'qosOverride.scope', ['peer', 'track']);

	let trackId: string | null | undefined;

	if ('trackId' in obj) {
		if (obj.trackId === null || obj.trackId === undefined) {
			trackId = obj.trackId;
		} else {
			trackId = asNonEmptyString(obj.trackId, 'qosOverride.trackId');
		}
	}

	const maxLevelClamp = optionalNumber(obj, 'maxLevelClamp', 'qosOverride', {
		integer: true,
		nonNegative: true,
	});
	const forceAudioOnly = optionalBoolean(obj, 'forceAudioOnly', 'qosOverride');
	const disableRecovery = optionalBoolean(
		obj,
		'disableRecovery',
		'qosOverride'
	);
	const ttlMs = asNonNegativeInt(obj.ttlMs, 'qosOverride.ttlMs');
	const reason = asNonEmptyString(obj.reason, 'qosOverride.reason');

	return {
		schema: QOS_OVERRIDE_SCHEMA_V1,
		scope,
		trackId,
		maxLevelClamp,
		forceAudioOnly,
		disableRecovery,
		ttlMs,
		reason,
	};
}

export function isClientQosSnapshot(
	value: unknown
): value is ClientQosSnapshot {
	try {
		parseClientQosSnapshot(value);

		return true;
	} catch {
		return false;
	}
}

export function isQosPolicy(value: unknown): value is QosPolicy {
	try {
		parseQosPolicy(value);

		return true;
	} catch {
		return false;
	}
}

export function isQosOverride(value: unknown): value is QosOverride {
	try {
		parseQosOverride(value);

		return true;
	} catch {
		return false;
	}
}

export function serializeClientQosSnapshot(
	snapshot: ClientQosSnapshot
): unknown {
	// Re-parse to guarantee output shape and fail early on invalid caller data.
	return parseClientQosSnapshot(snapshot);
}
