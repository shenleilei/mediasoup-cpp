import * as qos from '../../../src/client/lib/qos/index.js';

class JsonWsClient {
  constructor(url) {
    this.url = url;
    this.ws = null;
    this.nextId = 1;
    this.pending = new Map();
    this.notifications = [];
  }

  async connect() {
    this.ws = new WebSocket(this.url);
    await new Promise((resolve, reject) => {
      const timeout = setTimeout(() => reject(new Error('websocket connect timeout')), 5000);
      this.ws.addEventListener('open', () => {
        clearTimeout(timeout);
        resolve();
      }, { once: true });
      this.ws.addEventListener('error', () => {
        clearTimeout(timeout);
        reject(new Error('websocket connect failed'));
      }, { once: true });
      this.ws.addEventListener('message', event => {
        const parsed = JSON.parse(event.data);
        if (parsed.response === true) {
          const deferred = this.pending.get(parsed.id);
          if (deferred) {
            this.pending.delete(parsed.id);
            deferred(parsed);
          }
        } else if (parsed.notification === true) {
          this.notifications.push(parsed);
          if (this.onNotification) {
            this.onNotification(parsed);
          }
        }
      });
    });
  }

  async request(method, data = {}) {
    const id = this.nextId++;
    const payload = {
      request: true,
      id,
      method,
      data,
    };
    const response = new Promise(resolve => {
      this.pending.set(id, resolve);
    });
    this.ws.send(JSON.stringify(payload));
    return response;
  }

  async waitNotification(method, timeoutMs = 3000) {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      const index = this.notifications.findIndex(msg => msg.method === method);
      if (index !== -1) {
        return this.notifications.splice(index, 1)[0];
      }
      await new Promise(resolve => setTimeout(resolve, 50));
    }
    return null;
  }
}

function createStatsProvider() {
  let current = {
    timestampMs: 1000,
    trackId: 'camera-main',
    producerId: 'browser-signal-producer',
    source: 'camera',
    kind: 'video',
    bytesSent: 1000,
    packetsSent: 100,
    packetsLost: 0,
    targetBitrateBps: 900000,
    configuredBitrateBps: 900000,
    roundTripTimeMs: 120,
    jitterMs: 8,
    frameWidth: 640,
    frameHeight: 360,
    framesPerSecond: 24,
    qualityLimitationReason: 'none',
  };

  return {
    set(snapshot) {
      current = {
        ...current,
        ...snapshot,
      };
    },
    async getSnapshot() {
      return current;
    },
  };
}

async function initHarness(url, roomId, peerId) {
  const ws = new JsonWsClient(url);
  await ws.connect();

  const joinResp = await ws.request('join', {
    roomId,
    peerId,
    displayName: peerId,
    rtpCapabilities: {
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
    },
  });
  if (!joinResp.ok) {
    throw new Error(`join failed: ${JSON.stringify(joinResp)}`);
  }

  const statsProvider = createStatsProvider();
  const signalChannel = new qos.QosSignalChannel(async (method, data) => {
    const resp = await ws.request(method, data);
    if (!resp.ok) {
      throw new Error(resp.error || `${method} failed`);
    }
  });
  ws.onNotification = message => signalChannel.handleMessage(message);
  const clock = new qos.ManualQosClock();

  const controller = new qos.PublisherQosController({
    clock,
    profile: qos.getDefaultCameraProfile(),
    statsProvider,
    actionExecutor: {
      async execute() {
        return { status: 'applied' };
      },
    },
    signalChannel,
    trackId: 'camera-main',
    kind: 'video',
    producerId: 'browser-signal-producer',
  });

  const joinPolicy = await ws.waitNotification('qosPolicy', 3000);
  if (joinPolicy) {
    signalChannel.handleMessage(joinPolicy);
  }

  return { ws, controller, statsProvider, signalChannel, clock };
}

window.__qosServerHarness = {
  async init(url, roomId, peerId) {
    this.instance = await initHarness(url, roomId, peerId);
    return true;
  },
  async publishSnapshot(snapshot, repeats = 1) {
    this.instance.statsProvider.set(snapshot);
    for (let i = 0; i < repeats; ++i) {
      this.instance.clock.advanceBy(1000);
      await this.instance.controller.sampleNow();
    }
    return this.instance.controller.getRuntimeSettings();
  },
  async setPolicy(policy) {
    const resp = await this.instance.ws.request('setQosPolicy', {
      peerId: 'alice',
      policy,
    });
    if (!resp.ok) {
      throw new Error(resp.error || 'setQosPolicy failed');
    }
    const notif = await this.instance.ws.waitNotification('qosPolicy', 3000);
    if (!notif) {
      throw new Error('qosPolicy notification not received');
    }
    this.instance.signalChannel.handleMessage(notif);
    return this.instance.controller.getRuntimeSettings();
  },
  async waitOverride(expectedReasons = [], timeoutMs = 3000) {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      const notif = await this.instance.ws.waitNotification('qosOverride', 200);
      if (!notif) {
        continue;
      }
      this.instance.signalChannel.handleMessage(notif);
      const reason = notif.data?.reason;
      if (
        expectedReasons.length === 0 ||
        expectedReasons.includes(reason)
      ) {
        return {
          notif,
          runtime: this.instance.controller.getRuntimeSettings(),
        };
      }
    }
    throw new Error('qosOverride notification not received');
  },
  async sendRawClientSnapshot(snapshot) {
    const resp = await this.instance.ws.request('clientStats', snapshot);
    if (!resp.ok) {
      throw new Error(resp.error || 'clientStats failed');
    }
    return true;
  },
  async getControllerState() {
    return {
      track: this.instance.controller.getTrackState(),
      runtime: this.instance.controller.getRuntimeSettings(),
      trace: this.instance.controller.getTraceEntries(),
    };
  },
  async getServerStats() {
    const resp = await this.instance.ws.request('getStats', {
      peerId: 'alice',
    });
    if (!resp.ok) {
      throw new Error(resp.error || 'getStats failed');
    }
    return resp.data;
  },
};
