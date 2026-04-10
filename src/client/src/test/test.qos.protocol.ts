import * as fs from 'node:fs';
import * as path from 'node:path';

import * as protocol from '../qos/protocol';
import * as profiles from '../qos/profiles';
import * as clock from '../qos/clock';
import * as trace from '../qos/trace';

function loadFixture(name: string): any {
	const fixturePath = path.join(
		__dirname,
		'fixtures',
		'qos_protocol',
		`${name}.json`
	);

	return JSON.parse(fs.readFileSync(fixturePath, 'utf8'));
}

function createTraceBuffer(capacity: number): any {
	const MaybeCtor = (trace as any).QosTraceBuffer;

	if (typeof MaybeCtor === 'function') {
		return new MaybeCtor(capacity);
	}

	if (typeof (trace as any).createQosTraceBuffer === 'function') {
		return (trace as any).createQosTraceBuffer(capacity);
	}

	throw new Error('No trace buffer constructor/factory exported by qos/trace');
}

test('qos protocol parses and validates valid client snapshot fixture', () => {
	const fixture = loadFixture('valid_client_v1');

	expect(typeof (protocol as any).isClientQosSnapshot).toBe('function');
	expect(typeof (protocol as any).parseClientQosSnapshot).toBe('function');
	expect(typeof (protocol as any).serializeClientQosSnapshot).toBe('function');

	expect((protocol as any).isClientQosSnapshot(fixture)).toBe(true);

	const parsed = (protocol as any).parseClientQosSnapshot(fixture);

	expect(parsed.schema).toBe('mediasoup.qos.client.v1');
	expect(parsed.seq).toBe(42);
	expect(Array.isArray(parsed.tracks)).toBe(true);
	expect(parsed.tracks).toHaveLength(1);

	const serialized = (protocol as any).serializeClientQosSnapshot(parsed);

	expect((protocol as any).isClientQosSnapshot(serialized)).toBe(true);
});

test('qos protocol rejects invalid client snapshot fixtures', () => {
	const missingSchema = loadFixture('invalid_missing_schema');
	const badSeq = loadFixture('invalid_bad_seq');

	expect((protocol as any).isClientQosSnapshot(missingSchema)).toBe(false);
	expect((protocol as any).isClientQosSnapshot(badSeq)).toBe(false);

	expect(() =>
		(protocol as any).parseClientQosSnapshot(missingSchema)
	).toThrow();
	expect(() => (protocol as any).parseClientQosSnapshot(badSeq)).toThrow();
});

test('qos protocol parses and validates valid policy and override fixtures', () => {
	const policy = loadFixture('valid_policy_v1');
	const override = loadFixture('valid_override_v1');

	expect(typeof (protocol as any).isQosPolicy).toBe('function');
	expect(typeof (protocol as any).isQosOverride).toBe('function');
	expect(typeof (protocol as any).parseQosPolicy).toBe('function');
	expect(typeof (protocol as any).parseQosOverride).toBe('function');

	expect((protocol as any).isQosPolicy(policy)).toBe(true);
	expect((protocol as any).isQosOverride(override)).toBe(true);

	const parsedPolicy = (protocol as any).parseQosPolicy(policy);
	const parsedOverride = (protocol as any).parseQosOverride(override);

	expect(parsedPolicy.schema).toBe('mediasoup.qos.policy.v1');
	expect(parsedPolicy.sampleIntervalMs).toBe(1000);
	expect(parsedOverride.schema).toBe('mediasoup.qos.override.v1');
	expect(parsedOverride.maxLevelClamp).toBe(2);
});

test('qos profiles expose source-specific defaults and resolve helper', () => {
	expect(typeof (profiles as any).getDefaultCameraProfile).toBe('function');
	expect(typeof (profiles as any).getDefaultScreenShareProfile).toBe(
		'function'
	);
	expect(typeof (profiles as any).getDefaultAudioProfile).toBe('function');
	expect(typeof (profiles as any).resolveProfile).toBe('function');

	const camera = (profiles as any).getDefaultCameraProfile();
	const screenShare = (profiles as any).getDefaultScreenShareProfile();
	const audio = (profiles as any).getDefaultAudioProfile();

	expect(camera).toBeDefined();
	expect(screenShare).toBeDefined();
	expect(audio).toBeDefined();

	const resolvedCamera = (profiles as any).resolveProfile('camera');
	const resolvedScreenShare = (profiles as any).resolveProfile('screenShare');
	const resolvedAudio = (profiles as any).resolveProfile('audio');

	expect(resolvedCamera).toBeDefined();
	expect(resolvedScreenShare).toBeDefined();
	expect(resolvedAudio).toBeDefined();
});

test('qos trace buffer supports append/read/clear and capacity trimming', () => {
	const buffer = createTraceBuffer(2);

	expect(typeof buffer.appendTraceEntry).toBe('function');
	expect(typeof buffer.getTraceEntries).toBe('function');
	expect(typeof buffer.clearTrace).toBe('function');

	buffer.appendTraceEntry({ seq: 1 });
	buffer.appendTraceEntry({ seq: 2 });
	buffer.appendTraceEntry({ seq: 3 });

	const entries = buffer.getTraceEntries();

	expect(entries).toHaveLength(2);
	expect(entries[0].seq).toBe(2);
	expect(entries[1].seq).toBe(3);

	buffer.clearTrace();
	expect(buffer.getTraceEntries()).toHaveLength(0);
});

test('qos clock exports system and fake/manual clocks', () => {
	expect(typeof (clock as any).SystemQosClock).toBe('function');

	const systemClock = new (clock as any).SystemQosClock();

	expect(typeof systemClock.nowMs).toBe('function');
	expect(typeof systemClock.setInterval).toBe('function');
	expect(typeof systemClock.clearInterval).toBe('function');

	const before = systemClock.nowMs();
	const after = systemClock.nowMs();

	expect(after).toBeGreaterThanOrEqual(before);

	const FakeClockCtor =
		(clock as any).ManualQosClock ?? (clock as any).FakeQosClock;

	expect(typeof FakeClockCtor).toBe('function');

	const fakeClock = new FakeClockCtor();

	expect(typeof fakeClock.nowMs).toBe('function');
	expect(typeof fakeClock.setInterval).toBe('function');
	expect(typeof fakeClock.clearInterval).toBe('function');
});
