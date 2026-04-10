import type { QosSchemaName } from './types';

export const QOS_CLIENT_SCHEMA_V1 =
	'mediasoup.qos.client.v1' as const satisfies QosSchemaName;
export const QOS_POLICY_SCHEMA_V1 =
	'mediasoup.qos.policy.v1' as const satisfies QosSchemaName;
export const QOS_OVERRIDE_SCHEMA_V1 =
	'mediasoup.qos.override.v1' as const satisfies QosSchemaName;

export const DEFAULT_SAMPLE_INTERVAL_MS = 1000;
export const DEFAULT_SNAPSHOT_INTERVAL_MS = 2000;
export const DEFAULT_RECOVERY_COOLDOWN_MS = 8000;
export const DEFAULT_TRACE_BUFFER_SIZE = 256;

export const QOS_MAX_SEQ = Number.MAX_SAFE_INTEGER;
export const QOS_MAX_TRACKS_PER_SNAPSHOT = 32;
