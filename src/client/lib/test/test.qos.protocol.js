"use strict";
var __createBinding = (this && this.__createBinding) || (Object.create ? (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    var desc = Object.getOwnPropertyDescriptor(m, k);
    if (!desc || ("get" in desc ? !m.__esModule : desc.writable || desc.configurable)) {
      desc = { enumerable: true, get: function() { return m[k]; } };
    }
    Object.defineProperty(o, k2, desc);
}) : (function(o, m, k, k2) {
    if (k2 === undefined) k2 = k;
    o[k2] = m[k];
}));
var __setModuleDefault = (this && this.__setModuleDefault) || (Object.create ? (function(o, v) {
    Object.defineProperty(o, "default", { enumerable: true, value: v });
}) : function(o, v) {
    o["default"] = v;
});
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (k !== "default" && Object.prototype.hasOwnProperty.call(mod, k)) __createBinding(result, mod, k);
    __setModuleDefault(result, mod);
    return result;
};
Object.defineProperty(exports, "__esModule", { value: true });
const fs = __importStar(require("node:fs"));
const path = __importStar(require("node:path"));
const downlinkProtocol = __importStar(require("../qos/downlinkProtocol"));
const protocol = __importStar(require("../qos/protocol"));
const profiles = __importStar(require("../qos/profiles"));
const clock = __importStar(require("../qos/clock"));
const trace = __importStar(require("../qos/trace"));
function loadFixture(name) {
    const fixturePath = path.resolve(__dirname, '../../../../tests/fixtures/qos_protocol', `${name}.json`);
    return JSON.parse(fs.readFileSync(fixturePath, 'utf8'));
}
function createTraceBuffer(capacity) {
    const MaybeCtor = trace.QosTraceBuffer;
    if (typeof MaybeCtor === 'function') {
        return new MaybeCtor(capacity);
    }
    if (typeof trace.createQosTraceBuffer === 'function') {
        return trace.createQosTraceBuffer(capacity);
    }
    throw new Error('No trace buffer constructor/factory exported by qos/trace');
}
test('qos protocol parses and validates valid client snapshot fixture', () => {
    const fixture = loadFixture('valid_client_v1');
    expect(typeof protocol.isClientQosSnapshot).toBe('function');
    expect(typeof protocol.parseClientQosSnapshot).toBe('function');
    expect(typeof protocol.serializeClientQosSnapshot).toBe('function');
    expect(protocol.isClientQosSnapshot(fixture)).toBe(true);
    const parsed = protocol.parseClientQosSnapshot(fixture);
    expect(parsed.schema).toBe('mediasoup.qos.client.v1');
    expect(parsed.seq).toBe(42);
    expect(Array.isArray(parsed.tracks)).toBe(true);
    expect(parsed.tracks).toHaveLength(1);
    const serialized = protocol.serializeClientQosSnapshot(parsed);
    expect(protocol.isClientQosSnapshot(serialized)).toBe(true);
});
test('qos protocol rejects invalid client snapshot fixtures', () => {
    const missingSchema = loadFixture('invalid_missing_schema');
    const badSeq = loadFixture('invalid_bad_seq');
    expect(protocol.isClientQosSnapshot(missingSchema)).toBe(false);
    expect(protocol.isClientQosSnapshot(badSeq)).toBe(false);
    expect(() => protocol.parseClientQosSnapshot(missingSchema)).toThrow();
    expect(() => protocol.parseClientQosSnapshot(badSeq)).toThrow();
});
test('qos protocol parses and validates valid policy and override fixtures', () => {
    const policy = loadFixture('valid_policy_v1');
    const override = loadFixture('valid_override_v1');
    expect(typeof protocol.isQosPolicy).toBe('function');
    expect(typeof protocol.isQosOverride).toBe('function');
    expect(typeof protocol.parseQosPolicy).toBe('function');
    expect(typeof protocol.parseQosOverride).toBe('function');
    expect(protocol.isQosPolicy(policy)).toBe(true);
    expect(protocol.isQosOverride(override)).toBe(true);
    const parsedPolicy = protocol.parseQosPolicy(policy);
    const parsedOverride = protocol.parseQosOverride(override);
    expect(parsedPolicy.schema).toBe('mediasoup.qos.policy.v1');
    expect(parsedPolicy.sampleIntervalMs).toBe(1000);
    expect(parsedOverride.schema).toBe('mediasoup.qos.override.v1');
    expect(parsedOverride.maxLevelClamp).toBe(2);
});
test('qos protocol rejects mutually exclusive pause and resume override flags', () => {
    const override = loadFixture('valid_override_v1');
    override.pauseUpstream = true;
    override.resumeUpstream = true;
    expect(() => protocol.parseQosOverride(override)).toThrow(/mutually exclusive/);
});
test('downlink protocol normalizes legacy schema without mutating payload', () => {
    const payload = {
        schema: downlinkProtocol.DOWNLINK_SCHEMA_V1_LEGACY,
        seq: 42,
        subscriberPeerId: 'alice',
        tsMs: 123,
        subscriptions: [],
    };
    const parsed = downlinkProtocol.parseDownlinkSnapshot(payload);
    expect(parsed.schema).toBe(downlinkProtocol.DOWNLINK_SCHEMA_V1);
    expect(payload.schema).toBe(downlinkProtocol.DOWNLINK_SCHEMA_V1_LEGACY);
});
test('downlink protocol rejects unsafe seq values', () => {
    expect(() => downlinkProtocol.parseDownlinkSnapshot({
        schema: downlinkProtocol.DOWNLINK_SCHEMA_V1,
        seq: Number.MAX_SAFE_INTEGER + 1,
        subscriberPeerId: 'alice',
        tsMs: 123,
        subscriptions: [],
    })).toThrow(/safe integer/);
});
test('qos profiles expose source-specific defaults and resolve helper', () => {
    expect(typeof profiles.getDefaultCameraProfile).toBe('function');
    expect(typeof profiles.getDefaultScreenShareProfile).toBe('function');
    expect(typeof profiles.getDefaultAudioProfile).toBe('function');
    expect(typeof profiles.resolveProfile).toBe('function');
    const camera = profiles.getDefaultCameraProfile();
    const screenShare = profiles.getDefaultScreenShareProfile();
    const audio = profiles.getDefaultAudioProfile();
    expect(camera).toBeDefined();
    expect(screenShare).toBeDefined();
    expect(audio).toBeDefined();
    const resolvedCamera = profiles.resolveProfile('camera');
    const resolvedScreenShare = profiles.resolveProfile('screenShare');
    const resolvedAudio = profiles.resolveProfile('audio');
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
    expect(typeof clock.SystemQosClock).toBe('function');
    const systemClock = new clock.SystemQosClock();
    expect(typeof systemClock.nowMs).toBe('function');
    expect(typeof systemClock.setInterval).toBe('function');
    expect(typeof systemClock.clearInterval).toBe('function');
    const before = systemClock.nowMs();
    const after = systemClock.nowMs();
    expect(after).toBeGreaterThanOrEqual(before);
    const FakeClockCtor = clock.ManualQosClock ?? clock.FakeQosClock;
    expect(typeof FakeClockCtor).toBe('function');
    const fakeClock = new FakeClockCtor();
    expect(typeof fakeClock.nowMs).toBe('function');
    expect(typeof fakeClock.setInterval).toBe('function');
    expect(typeof fakeClock.clearInterval).toBe('function');
});
