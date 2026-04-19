/**
 * Browser-side harness for end-to-end downlink QoS verification.
 * Verifies: client sends downlinkClientStats → server auto-pauses/resumes consumer.
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

window.__downlinkE2eHarness = {
  pub: null,
  sub: null,
  consumerId: null,
  producerId: null,
  roomId: null,

  async init(wsUrl, roomId) {
    this.roomId = roomId;

    // Publisher
    this.pub = new JsonWsClient(wsUrl);
    await this.pub.connect();
    const joinPub = await this.pub.request('join', {
      roomId, peerId: 'pub', displayName: 'pub', rtpCapabilities: RTP_CAPS,
    });
    if (!joinPub.ok) throw new Error('pub join failed: ' + JSON.stringify(joinPub));

    // Subscriber
    this.sub = new JsonWsClient(wsUrl);
    await this.sub.connect();
    const joinSub = await this.sub.request('join', {
      roomId, peerId: 'sub', displayName: 'sub', rtpCapabilities: RTP_CAPS,
    });
    if (!joinSub.ok) throw new Error('sub join failed: ' + JSON.stringify(joinSub));
    this.subPeerId = 'sub';

    // Sub recv transport
    const subRecv = await this.sub.request('createWebRtcTransport', { producing: false, consuming: true });
    if (!subRecv.ok) throw new Error('sub recv transport failed');

    // Pub send transport
    const pubSend = await this.pub.request('createWebRtcTransport', { producing: true, consuming: false });
    if (!pubSend.ok) throw new Error('pub send transport failed');

    // Pub produces video
    const prodResp = await this.pub.request('produce', {
      transportId: pubSend.data.id,
      kind: 'video',
      rtpParameters: {
        codecs: [{ mimeType: 'video/VP8', clockRate: 90000, payloadType: 101 }],
        encodings: [{ ssrc: 88880001 }],
        mid: '0',
      },
    });
    if (!prodResp.ok) throw new Error('produce failed: ' + JSON.stringify(prodResp));
    this.producerId = prodResp.data.id;

    // Sub waits for newConsumer
    const notif = await this.sub.waitNotification('newConsumer', 5000);
    if (!notif) throw new Error('newConsumer not received');
    this.consumerId = notif.data.id;

    return { consumerId: this.consumerId, producerId: this.producerId };
  },

  /**
   * Send a downlinkClientStats snapshot with given visibility.
   */
  async sendDownlinkStats(visible, seq) {
    const resp = await this.sub.request('downlinkClientStats', {
      schema: 'mediasoup.qos.downlink.client.v1',
      seq: seq,
      tsMs: Date.now(),
      subscriberPeerId: this.subPeerId,
      transport: {
        availableIncomingBitrate: 1000000,
        currentRoundTripTime: 0.02,
      },
      subscriptions: [{
        consumerId: this.consumerId,
        producerId: this.producerId,
        visible: visible,
        pinned: false,
        activeSpeaker: false,
        isScreenShare: false,
        targetWidth: visible ? 640 : 0,
        targetHeight: visible ? 480 : 0,
        packetsLost: 0,
        jitter: 0.001,
        framesPerSecond: visible ? 30 : 0,
        frameWidth: visible ? 640 : 0,
        frameHeight: visible ? 480 : 0,
        freezeRate: 0,
      }],
    });
    if (!resp.ok) throw new Error('downlinkClientStats failed: ' + JSON.stringify(resp));
    return resp;
  },

  /**
   * Read back consumer state without changing it.
   */
  async getConsumerState() {
    const resp = await this.sub.request('getConsumerState', {
      consumerId: this.consumerId,
    });
    if (!resp.ok) throw new Error('getConsumerState failed: ' + JSON.stringify(resp));
    return resp.data;
  },
};
