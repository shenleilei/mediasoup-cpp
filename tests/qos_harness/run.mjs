import { createRequire } from 'node:module';
import crypto from 'node:crypto';
import fs from 'node:fs';
import net from 'node:net';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn } from 'node:child_process';

const require = createRequire(import.meta.url);
const { qos } = require('../../src/client/lib');

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const host = '127.0.0.1';
const port = 14011;

class WsJsonClient {
  constructor(hostname, port, pathName = '/ws') {
    this.hostname = hostname;
    this.port = port;
    this.pathName = pathName;
    this.socket = null;
    this.pending = Buffer.alloc(0);
    this.nextId = 1;
    this.responses = [];
    this.notifications = [];
    this.waiters = [];
    this.notificationHandlers = new Set();
  }

  async connect() {
    this.socket = net.createConnection({ host: this.hostname, port: this.port });

    await new Promise((resolve, reject) => {
      this.socket.once('connect', resolve);
      this.socket.once('error', reject);
    });

    const key = crypto.randomBytes(16).toString('base64');
    const req =
      `GET ${this.pathName} HTTP/1.1\r\n` +
      `Host: ${this.hostname}:${this.port}\r\n` +
      'Upgrade: websocket\r\n' +
      'Connection: Upgrade\r\n' +
      `Sec-WebSocket-Key: ${key}\r\n` +
      'Sec-WebSocket-Version: 13\r\n' +
      `Origin: http://${this.hostname}\r\n\r\n`;

    this.socket.write(req);

    const header = await this.#readHttpHeader();
    if (!header.includes('101')) {
      throw new Error(`websocket handshake failed: ${header}`);
    }

    this.socket.on('data', chunk => this.#onData(chunk));
  }

  close() {
    if (!this.socket) {
      return;
    }
    this.socket.destroy();
    this.socket = null;
  }

  async request(method, data = {}, timeoutMs = 5000) {
    const id = this.nextId++;
    this.#sendJson({
      request: true,
      id,
      method,
      data,
    });

    return this.#waitForResponse(id, timeoutMs);
  }

  onNotification(handler) {
    this.notificationHandlers.add(handler);
    return () => this.notificationHandlers.delete(handler);
  }

  async waitNotification(method, timeoutMs = 5000) {
    const deadline = Date.now() + timeoutMs;

    while (Date.now() < deadline) {
      const index = this.notifications.findIndex(msg => msg.method === method);
      if (index !== -1) {
        return this.notifications.splice(index, 1)[0];
      }

      await new Promise(resolve => {
        const remaining = Math.max(1, deadline - Date.now());
        const timer = setTimeout(() => {
          cleanup();
          resolve();
        }, Math.min(remaining, 100));
        const cleanup = () => clearTimeout(timer);
        this.waiters.push(() => {
          cleanup();
          resolve();
        });
      });
    }

    return null;
  }

  #sendJson(jsonValue) {
    const payload = Buffer.from(JSON.stringify(jsonValue), 'utf8');
    const frame = this.#makeClientTextFrame(payload);
    this.socket.write(frame);
  }

  #makeClientTextFrame(payload) {
    const header = [];
    header.push(0x81);

    const mask = crypto.randomBytes(4);
    const length = payload.length;

    if (length < 126) {
      header.push(0x80 | length);
    } else if (length < 65536) {
      header.push(0x80 | 126, (length >> 8) & 0xff, length & 0xff);
    } else {
      throw new Error('payload too large');
    }

    const masked = Buffer.alloc(payload.length);
    for (let i = 0; i < payload.length; ++i) {
      masked[i] = payload[i] ^ mask[i % 4];
    }

    return Buffer.concat([Buffer.from(header), mask, masked]);
  }

  async #readHttpHeader() {
    while (true) {
      const marker = this.pending.indexOf('\r\n\r\n');
      if (marker !== -1) {
        const header = this.pending.slice(0, marker + 4).toString('utf8');
        this.pending = this.pending.slice(marker + 4);
        return header;
      }

      await new Promise((resolve, reject) => {
        const socket = this.socket;
        if (!socket) {
          reject(new Error('socket closed during websocket handshake'));
          return;
        }
        const onData = chunk => {
          this.pending = Buffer.concat([this.pending, chunk]);
          cleanup();
          resolve();
        };
        const onError = error => {
          cleanup();
          reject(error);
        };
        const cleanup = () => {
          socket.off('data', onData);
          socket.off('error', onError);
        };
        socket.on('data', onData);
        socket.on('error', onError);
      });
    }
  }

  #onData(chunk) {
    this.pending = Buffer.concat([this.pending, chunk]);

    while (this.pending.length >= 2) {
      const opcode = this.pending[0] & 0x0f;
      const masked = (this.pending[1] & 0x80) !== 0;
      let payloadLength = this.pending[1] & 0x7f;
      let offset = 2;

      if (payloadLength === 126) {
        if (this.pending.length < 4) {
          return;
        }
        payloadLength = this.pending.readUInt16BE(2);
        offset = 4;
      } else if (payloadLength === 127) {
        throw new Error('unsupported long websocket frame');
      }

      if (masked) {
        throw new Error('server frames must not be masked');
      }

      if (this.pending.length < offset + payloadLength) {
        return;
      }

      const payload = this.pending.slice(offset, offset + payloadLength);
      this.pending = this.pending.slice(offset + payloadLength);

      if (opcode === 0x8) {
        this.close();
        return;
      }

      if (opcode !== 0x1) {
        continue;
      }

      const parsed = JSON.parse(payload.toString('utf8'));
      if (parsed.response === true) {
        this.responses.push(parsed);
      } else if (parsed.notification === true) {
        this.notifications.push(parsed);
        for (const handler of this.notificationHandlers) {
          handler(parsed);
        }
      }

      this.#flushWaiters();
    }
  }

  #flushWaiters() {
    const waiters = this.waiters.slice();
    this.waiters = [];

    for (const waiter of waiters) {
      waiter();
    }
  }

  async #waitForResponse(id, timeoutMs) {
    const deadline = Date.now() + timeoutMs;

    while (Date.now() < deadline) {
      const index = this.responses.findIndex(resp => resp.id === id);
      if (index !== -1) {
        return this.responses.splice(index, 1)[0];
      }

      await new Promise(resolve => {
        const remaining = Math.max(1, deadline - Date.now());
        const timer = setTimeout(() => {
          cleanup();
          resolve();
        }, Math.min(remaining, 100));
        const cleanup = () => clearTimeout(timer);
        this.waiters.push(() => {
          cleanup();
          resolve();
        });
      });
    }

    return { ok: false, error: 'timeout' };
  }
}

function loadScenario(name) {
  const filePath = path.join(__dirname, 'scenarios', `${name}.json`);
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function waitForPort(hostname, port, timeoutMs = 5000) {
  const deadline = Date.now() + timeoutMs;

  while (Date.now() < deadline) {
    try {
      await new Promise((resolve, reject) => {
        const socket = net.createConnection({ host: hostname, port }, () => {
          socket.destroy();
          resolve();
        });
        socket.once('error', reject);
      });
      return;
    } catch {
      await sleep(100);
    }
  }

  throw new Error(`port ${port} did not become ready`);
}

function startSfu() {
  const child = spawn(
    path.join(repoRoot, 'build', 'mediasoup-sfu'),
    [
      '--nodaemon',
      `--port=${port}`,
      '--workers=1',
      '--workerBin=./mediasoup-worker',
      '--announcedIp=127.0.0.1',
      '--listenIp=127.0.0.1',
      '--redisHost=0.0.0.0',
      '--redisPort=1',
      '--noRedisRequired',
    ],
    {
      cwd: repoRoot,
      stdio: ['ignore', 'pipe', 'pipe'],
    }
  );

  child.stdout.on('data', chunk => process.stdout.write(chunk));
  child.stderr.on('data', chunk => process.stderr.write(chunk));

  return child;
}

function stopChild(child, timeoutMs = 3000) {
  return new Promise(resolve => {
    if (!child.pid || child.exitCode !== null) {
      resolve();
      return;
    }
    const timer = setTimeout(() => {
      try { child.kill('SIGKILL'); } catch {}
      resolve();
    }, timeoutMs);
    child.once('close', () => {
      clearTimeout(timer);
      resolve();
    });
    child.kill('SIGTERM');
  });
}

function defaultRtpCapabilities() {
  return {
    codecs: [
      {
        mimeType: 'audio/opus',
        kind: 'audio',
        clockRate: 48000,
        channels: 2,
        preferredPayloadType: 100,
      },
      {
        mimeType: 'video/VP8',
        kind: 'video',
        clockRate: 90000,
        preferredPayloadType: 101,
      },
    ],
    headerExtensions: [],
  };
}

async function joinRoom(ws, roomId, peerId) {
  const resp = await ws.request('join', {
    roomId,
    peerId,
    displayName: peerId,
    rtpCapabilities: defaultRtpCapabilities(),
  });

  if (!resp.ok) {
    throw new Error(`join failed: ${JSON.stringify(resp)}`);
  }

  let qosPolicy = await ws.waitNotification('qosPolicy', 5000);
  if (!qosPolicy && resp.data?.qosPolicy) {
    qosPolicy = {
      notification: true,
      method: 'qosPolicy',
      data: resp.data.qosPolicy,
    };
  }
  if (!qosPolicy) {
    qosPolicy = {
      notification: true,
      method: 'qosPolicy',
      data: {
        schema: 'mediasoup.qos.policy.v1',
        sampleIntervalMs: 1000,
        snapshotIntervalMs: 2000,
        allowAudioOnly: true,
        allowVideoPause: true,
        profiles: {
          camera: 'default',
          screenShare: 'clarity-first',
          audio: 'speech-first',
        },
      },
    };
  }

  return { resp, qosPolicy };
}

async function runPublishSnapshotScenario() {
  const scenario = loadScenario('publish_snapshot');
  const ws = new WsJsonClient(host, port);
  await ws.connect();
  const joined = await joinRoom(ws, scenario.roomId, scenario.peerId);

  const clock = new qos.ManualQosClock();
  let index = 0;
  const statsProvider = {
    getSnapshot: async () => {
      const current = scenario.snapshots[Math.min(index, scenario.snapshots.length - 1)];
      index += 1;
      return {
        ...current,
        trackId: scenario.trackId,
        producerId: 'producer-123',
        source: scenario.source,
        kind: scenario.kind,
      };
    },
  };
  const actionExecutor = {
    async execute() {
      return { status: 'applied' };
    },
  };
  const signalChannel = new qos.QosSignalChannel(async (method, data) => {
    const resp = await ws.request(method, data);
    if (!resp.ok) {
      throw new Error(resp.error || 'clientStats failed');
    }
  });
  ws.onNotification(message => signalChannel.handleMessage(message));
  signalChannel.handleMessage(joined.qosPolicy);

  const controller = new qos.PublisherQosController({
    clock,
    profile: qos.getDefaultCameraProfile(),
    statsProvider,
    actionExecutor,
    signalChannel,
    trackId: scenario.trackId,
    kind: scenario.kind,
    producerId: 'producer-123',
    sampleIntervalMs: 1000,
    snapshotIntervalMs: 2000,
  });

  clock.advanceBy(1000);
  await controller.sampleNow();
  // Explicitly cross the 2s snapshot interval before the second sample so
  // this scenario validates periodic publish behavior rather than relying on
  // a specific state transition to trigger the second clientStats snapshot.
  clock.advanceBy(2000);
  await controller.sampleNow();
  await sleep(200);

  const statsResp = await ws.request('getStats', { peerId: scenario.peerId });
  if (!statsResp.ok) {
    throw new Error(`getStats failed: ${JSON.stringify(statsResp)}`);
  }

  const data = statsResp.data;
  if (!data.clientStats || !data.qos) {
    throw new Error(`missing qos aggregate in stats: ${JSON.stringify(data)}`);
  }
  if (data.clientStats.seq !== scenario.expect.clientStatsSeq) {
    throw new Error(`unexpected clientStats seq: ${data.clientStats.seq}`);
  }
  if (data.qos.quality !== scenario.expect.qosQuality) {
    throw new Error(`unexpected qos quality: ${data.qos.quality}`);
  }

  ws.close();
}

async function runStaleSeqScenario() {
  const scenario = loadScenario('stale_seq');
  const ws = new WsJsonClient(host, port);
  await ws.connect();
  const joined = await joinRoom(ws, scenario.roomId, scenario.peerId);

  const signalChannel = new qos.QosSignalChannel(async (method, data) => {
    const resp = await ws.request(method, data);
    if (!resp.ok) {
      throw new Error(resp.error || 'clientStats failed');
    }
  });
  ws.onNotification(message => signalChannel.handleMessage(message));
  signalChannel.handleMessage(joined.qosPolicy);

  await signalChannel.publishSnapshot(scenario.newer);
  await signalChannel.publishSnapshot(scenario.older);
  await sleep(200);

  const statsResp = await ws.request('getStats', { peerId: scenario.peerId });
  if (!statsResp.ok) {
    throw new Error(`getStats failed: ${JSON.stringify(statsResp)}`);
  }

  const data = statsResp.data;
  if (data.clientStats.seq !== 50 || data.qos.seq !== 50 || data.qos.quality !== 'good') {
    throw new Error(`stale seq handling failed: ${JSON.stringify(data)}`);
  }

  ws.close();
}

async function runOverrideScenario() {
  const scenario = loadScenario('override_force_audio_only');
  const ws = new WsJsonClient(host, port);
  await ws.connect();
  const joined = await joinRoom(ws, scenario.roomId, scenario.peerId);

  const clock = new qos.ManualQosClock();
  const statsProvider = {
    getSnapshot: async () => ({
      ...scenario.snapshot,
      trackId: scenario.trackId,
      producerId: 'producer-override',
      source: scenario.source,
      kind: scenario.kind,
    }),
  };

  const executedActions = [];
  const actionExecutor = {
    async execute(action) {
      executedActions.push(action);
      return { status: 'applied' };
    },
  };
  const signalChannel = new qos.QosSignalChannel(async (method, data) => {
    const resp = await ws.request(method, data);
    if (!resp.ok) {
      throw new Error(resp.error || `${method} failed`);
    }
  });
  ws.onNotification(message => signalChannel.handleMessage(message));
  signalChannel.handleMessage(joined.qosPolicy);

  const controller = new qos.PublisherQosController({
    clock,
    profile: qos.getDefaultCameraProfile(),
    statsProvider,
    actionExecutor,
    signalChannel,
    trackId: scenario.trackId,
    kind: scenario.kind,
    producerId: 'producer-override',
    sampleIntervalMs: 1000,
    snapshotIntervalMs: 2000,
  });

  controller.start();
  const resp = await ws.request('setQosOverride', {
    peerId: scenario.peerId,
    override: scenario.override,
  });
  if (!resp.ok) {
    throw new Error(`setQosOverride failed: ${JSON.stringify(resp)}`);
  }

  const overrideNotif = await ws.waitNotification('qosOverride', 3000);
  if (!overrideNotif) {
    throw new Error('did not receive qosOverride notification');
  }
  signalChannel.handleMessage(overrideNotif);

  clock.advanceBy(1000);
  await controller.sampleNow();

  if (!executedActions.some(action => action.type === 'enterAudioOnly')) {
    throw new Error(`override did not trigger audio-only action: ${JSON.stringify(executedActions)}`);
  }

  ws.close();
}

async function runAutomaticOverrideScenario() {
  const scenario = loadScenario('auto_override_poor');
  const ws = new WsJsonClient(host, port);
  await ws.connect();
  const joined = await joinRoom(ws, scenario.roomId, scenario.peerId);

  const signalChannel = new qos.QosSignalChannel(async (method, data) => {
    const resp = await ws.request(method, data);
    if (!resp.ok) {
      throw new Error(resp.error || `${method} failed`);
    }
  });
  ws.onNotification(message => signalChannel.handleMessage(message));
  signalChannel.handleMessage(joined.qosPolicy);

  await signalChannel.publishSnapshot(scenario.snapshot);
  const notif = await ws.waitNotification('qosOverride', 3000);
  if (!notif) {
    throw new Error('did not receive automatic qosOverride notification');
  }

  if (notif.data?.reason !== 'server_auto_poor') {
    throw new Error(`unexpected automatic qosOverride: ${JSON.stringify(notif)}`);
  }

  ws.close();
}

async function runPolicyUpdateScenario() {
  const scenario = loadScenario('policy_update');
  const ws = new WsJsonClient(host, port);
  await ws.connect();
  const joined = await joinRoom(ws, scenario.roomId, scenario.peerId);

  const clock = new qos.ManualQosClock();
  const statsProvider = {
    getSnapshot: async () => ({
      timestampMs: 1000,
      trackId: 'camera-main',
      producerId: 'producer-policy',
      source: 'camera',
      kind: 'video',
      bytesSent: 1000,
      packetsSent: 100,
      targetBitrateBps: 900000,
      configuredBitrateBps: 900000,
      roundTripTimeMs: 120,
      jitterMs: 8,
      qualityLimitationReason: 'none',
    }),
  };
  const actionExecutor = {
    async execute() {
      return { status: 'applied' };
    },
  };
  const signalChannel = new qos.QosSignalChannel(async (method, data) => {
    const resp = await ws.request(method, data);
    if (!resp.ok) {
      throw new Error(resp.error || `${method} failed`);
    }
  });
  ws.onNotification(message => signalChannel.handleMessage(message));

  const controller = new qos.PublisherQosController({
    clock,
    profile: qos.getDefaultCameraProfile(),
    statsProvider,
    actionExecutor,
    signalChannel,
    trackId: 'camera-main',
    kind: 'video',
    producerId: 'producer-policy',
  });

  signalChannel.handleMessage(joined.qosPolicy);
  const resp = await ws.request('setQosPolicy', {
    peerId: scenario.peerId,
    policy: scenario.policy,
  });
  if (!resp.ok) {
    throw new Error(`setQosPolicy failed: ${JSON.stringify(resp)}`);
  }

  const policyNotif = await ws.waitNotification('qosPolicy', 3000);
  if (!policyNotif) {
    throw new Error('did not receive qosPolicy update');
  }
  signalChannel.handleMessage(policyNotif);

  const runtime = controller.getRuntimeSettings();
  if (
    runtime.sampleIntervalMs !== 1500 ||
    runtime.snapshotIntervalMs !== 4000 ||
    runtime.allowAudioOnly !== false ||
    runtime.allowVideoPause !== false
  ) {
    throw new Error(`controller did not apply qosPolicy update: ${JSON.stringify(runtime)}`);
  }

  ws.close();
}

async function runManualClearScenario() {
  const scenario = loadScenario('manual_clear');
  const ws = new WsJsonClient(host, port);
  await ws.connect();
  const joined = await joinRoom(ws, scenario.roomId, scenario.peerId);

  const clock = new qos.ManualQosClock();
  const statsProvider = {
    getSnapshot: async () => ({
      ...scenario.snapshot,
      trackId: scenario.trackId,
      producerId: 'producer-manual-clear',
      source: scenario.source,
      kind: scenario.kind,
    }),
  };

  const executedActions = [];
  const actionExecutor = {
    async execute(action) {
      executedActions.push(action);
      return { status: 'applied' };
    },
  };
  const signalChannel = new qos.QosSignalChannel(async (method, data) => {
    const resp = await ws.request(method, data);
    if (!resp.ok) {
      throw new Error(resp.error || `${method} failed`);
    }
  });
  ws.onNotification(message => signalChannel.handleMessage(message));
  signalChannel.handleMessage(joined.qosPolicy);

  const controller = new qos.PublisherQosController({
    clock,
    profile: qos.getDefaultCameraProfile(),
    statsProvider,
    actionExecutor,
    signalChannel,
    trackId: scenario.trackId,
    kind: scenario.kind,
    producerId: 'producer-manual-clear',
    sampleIntervalMs: 1000,
    snapshotIntervalMs: 2000,
  });

  controller.start();
  let resp = await ws.request('setQosOverride', {
    peerId: scenario.peerId,
    override: scenario.override,
  });
  if (!resp.ok) {
    throw new Error(`setQosOverride failed: ${JSON.stringify(resp)}`);
  }

  const overrideNotif = await ws.waitNotification('qosOverride', 3000);
  if (!overrideNotif) {
    throw new Error('did not receive manual qosOverride notification');
  }
  signalChannel.handleMessage(overrideNotif);
  clock.advanceBy(1000);
  await controller.sampleNow();

  if (!executedActions.some(action => action.type === 'enterAudioOnly')) {
    throw new Error(`manual override did not trigger audio-only action: ${JSON.stringify(executedActions)}`);
  }

  resp = await ws.request('setQosOverride', {
    peerId: scenario.peerId,
    override: scenario.clearOverride,
  });
  if (!resp.ok) {
    throw new Error(`clear setQosOverride failed: ${JSON.stringify(resp)}`);
  }

  const clearNotif = await ws.waitNotification('qosOverride', 3000);
  if (!clearNotif) {
    throw new Error('did not receive manual clear qosOverride notification');
  }
  signalChannel.handleMessage(clearNotif);

  for (let attempt = 0; attempt < 5; attempt += 1) {
    clock.advanceBy(1000);
    await controller.sampleNow();
    if (executedActions.some(action => action.type === 'exitAudioOnly')) {
      break;
    }
  }

  if (!executedActions.some(action => action.type === 'exitAudioOnly')) {
    throw new Error(`manual clear did not restore local control: ${JSON.stringify(executedActions)}`);
  }
  if (controller.getTrackState().inAudioOnlyMode) {
    throw new Error('controller remained in audio-only mode after manual clear');
  }

  ws.close();
}

async function main() {
  const scenario = process.argv[2] || 'quick';
  const child = startSfu();

  try {
    await waitForPort(host, port, 7000);

    if (scenario === 'quick' || scenario === 'publish_snapshot') {
      await runPublishSnapshotScenario();
    }

    if (scenario === 'quick' || scenario === 'stale_seq') {
      await runStaleSeqScenario();
    }

    if (scenario === 'override_force_audio_only') {
      await runOverrideScenario();
    }

    if (scenario === 'quick' || scenario === 'policy_update') {
      await runPolicyUpdateScenario();
    }

    if (scenario === 'auto_override_poor') {
      await runAutomaticOverrideScenario();
    }

    if (scenario === 'manual_clear') {
      await runManualClearScenario();
    }

    console.log(`qos_harness ${scenario} passed`);
  } finally {
    await stopChild(child);
  }
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
