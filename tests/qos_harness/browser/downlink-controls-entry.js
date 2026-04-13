/**
 * Browser-side harness for downlink consumer control verification.
 * Simulates publisher + subscriber in the same page via SFU.
 */

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
      const timeout = setTimeout(() => reject(new Error('ws connect timeout')), 5000);
      this.ws.addEventListener('open', () => { clearTimeout(timeout); resolve(); }, { once: true });
      this.ws.addEventListener('error', () => { clearTimeout(timeout); reject(new Error('ws connect failed')); }, { once: true });
      this.ws.addEventListener('message', event => {
        const msg = JSON.parse(event.data);
        if (msg.response === true) {
          const cb = this.pending.get(msg.id);
          if (cb) { this.pending.delete(msg.id); cb(msg); }
        } else if (msg.notification === true) {
          this.notifications.push(msg);
        }
      });
    });
  }

  async request(method, data = {}) {
    const id = this.nextId++;
    const resp = new Promise(resolve => this.pending.set(id, resolve));
    this.ws.send(JSON.stringify({ request: true, id, method, data }));
    return resp;
  }

  async waitNotification(method, timeoutMs = 5000) {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      const idx = this.notifications.findIndex(n => n.method === method);
      if (idx !== -1) return this.notifications.splice(idx, 1)[0];
      await new Promise(r => setTimeout(r, 50));
    }
    return null;
  }
}

const RTP_CAPS = {
  codecs: [
    { mimeType: 'audio/opus', kind: 'audio', clockRate: 48000, channels: 2, preferredPayloadType: 100 },
    { mimeType: 'video/VP8', kind: 'video', clockRate: 90000, preferredPayloadType: 101 },
  ],
  headerExtensions: [],
};

async function initHarness(wsUrl, roomId) {
  // Publisher peer
  const pub = new JsonWsClient(wsUrl);
  await pub.connect();
  const joinPub = await pub.request('join', {
    roomId, peerId: 'pub', displayName: 'pub', rtpCapabilities: RTP_CAPS,
  });
  if (!joinPub.ok) throw new Error('pub join failed: ' + JSON.stringify(joinPub));

  // Subscriber peer
  const sub = new JsonWsClient(wsUrl);
  await sub.connect();
  const joinSub = await sub.request('join', {
    roomId, peerId: 'sub', displayName: 'sub', rtpCapabilities: RTP_CAPS,
  });
  if (!joinSub.ok) throw new Error('sub join failed: ' + JSON.stringify(joinSub));

  // Sub creates recv transport
  const subRecv = await sub.request('createWebRtcTransport', { producing: false, consuming: true });
  if (!subRecv.ok) throw new Error('sub recv transport failed');

  // Pub creates send transport
  const pubSend = await pub.request('createWebRtcTransport', { producing: true, consuming: false });
  if (!pubSend.ok) throw new Error('pub send transport failed');

  // Pub produces video
  const prodResp = await pub.request('produce', {
    transportId: pubSend.data.id,
    kind: 'video',
    rtpParameters: {
      codecs: [{ mimeType: 'video/VP8', clockRate: 90000, payloadType: 101 }],
      encodings: [{ ssrc: 77770001 }],
      mid: '0',
    },
  });
  if (!prodResp.ok) throw new Error('produce failed: ' + JSON.stringify(prodResp));

  // Sub waits for newConsumer notification
  const consumerNotif = await sub.waitNotification('newConsumer', 5000);
  if (!consumerNotif) throw new Error('newConsumer not received');
  const consumerId = consumerNotif.data.id;

  return { pub, sub, consumerId, roomId };
}

async function getSubscriberVideoStats(instance) {
  const resp = await instance.sub.request('getStats', { peerId: 'sub' });
  return resp.ok ? resp.data : null;
}

window.__downlinkControlsHarness = {
  instance: null,

  async init(wsUrl, roomId) {
    this.instance = await initHarness(wsUrl, roomId);
    return { consumerId: this.instance.consumerId };
  },

  async getConsumerId() {
    return this.instance.consumerId;
  },

  async pauseConsumer() {
    const resp = await this.instance.sub.request('pauseConsumer', {
      consumerId: this.instance.consumerId,
    });
    if (!resp.ok) throw new Error('pauseConsumer failed: ' + JSON.stringify(resp));
    return resp.data;
  },

  async resumeConsumer() {
    const resp = await this.instance.sub.request('resumeConsumer', {
      consumerId: this.instance.consumerId,
    });
    if (!resp.ok) throw new Error('resumeConsumer failed: ' + JSON.stringify(resp));
    return resp.data;
  },

  async requestConsumerKeyFrame() {
    const resp = await this.instance.sub.request('requestConsumerKeyFrame', {
      consumerId: this.instance.consumerId,
    });
    if (!resp.ok) throw new Error('requestConsumerKeyFrame failed: ' + JSON.stringify(resp));
    return resp.data;
  },

  async setConsumerPreferredLayers(spatialLayer, temporalLayer) {
    const resp = await this.instance.sub.request('setConsumerPreferredLayers', {
      consumerId: this.instance.consumerId,
      spatialLayer,
      temporalLayer,
    });
    if (!resp.ok) throw new Error('setConsumerPreferredLayers failed: ' + JSON.stringify(resp));
    return resp.data;
  },

  async setConsumerPriority(priority) {
    const resp = await this.instance.sub.request('setConsumerPriority', {
      consumerId: this.instance.consumerId,
      priority,
    });
    if (!resp.ok) throw new Error('setConsumerPriority failed: ' + JSON.stringify(resp));
    return resp.data;
  },

  async getStats() {
    return getSubscriberVideoStats(this.instance);
  },
};
