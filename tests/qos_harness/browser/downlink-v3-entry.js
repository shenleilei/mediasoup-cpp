/**
 * Browser-side harness for downlink QoS v3 pause/resume verification.
 * Verifies that sustained zero-demand triggers pauseUpstream,
 * and demand restoration triggers resumeUpstream.
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

  drainNotifications() {
    const out = [...this.notifications];
    this.notifications.length = 0;
    return out;
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

window.__downlinkV3Harness = {
  pub: null,
  sub: null,
  roomId: null,
  producerId: null,
  consumerId: null,
  trackId: 'camera-main',
  seq: 0,

  async init(wsUrl, roomId) {
    this.roomId = roomId;
    this.seq = 0;

    this.pub = new JsonWsClient(wsUrl);
    await this.pub.connect();
    const joinPub = await this.pub.request('join', {
      roomId,
      peerId: 'pub',
      displayName: 'pub',
      rtpCapabilities: RTP_CAPS,
    });
    if (!joinPub.ok) throw new Error('pub join failed: ' + JSON.stringify(joinPub));

    this.sub = new JsonWsClient(wsUrl);
    await this.sub.connect();
    const joinSub = await this.sub.request('join', {
      roomId,
      peerId: 'sub',
      displayName: 'sub',
      rtpCapabilities: RTP_CAPS,
    });
    if (!joinSub.ok) throw new Error('sub join failed: ' + JSON.stringify(joinSub));

    const subRecv = await this.sub.request('createWebRtcTransport', { producing: false, consuming: true });
    if (!subRecv.ok) throw new Error('sub recv transport failed');

    const pubSend = await this.pub.request('createWebRtcTransport', { producing: true, consuming: false });
    if (!pubSend.ok) throw new Error('pub send transport failed');

    const prodResp = await this.pub.request('produce', {
      transportId: pubSend.data.id,
      kind: 'video',
      rtpParameters: {
        codecs: [{ mimeType: 'video/VP8', clockRate: 90000, payloadType: 101 }],
        encodings: [{ ssrc: 99990201 }],
        mid: '0',
      },
    });
    if (!prodResp.ok) throw new Error('produce failed: ' + JSON.stringify(prodResp));
    this.producerId = prodResp.data.id;

    const consumerNotif = await this.sub.waitNotification('newConsumer', 5000);
    if (!consumerNotif) throw new Error('newConsumer not received');
    this.consumerId = consumerNotif.data.id;

    return {
      producerId: this.producerId,
      consumerId: this.consumerId,
      trackId: this.trackId,
    };
  },

  async publishPublisherStats() {
    this.seq++;
    const resp = await this.pub.request('clientStats', {
      schema: 'mediasoup.qos.client.v1',
      seq: this.seq,
      tsMs: Date.now(),
      peerState: { mode: 'audio-video', quality: 'good', stale: false },
      tracks: [{
        localTrackId: this.trackId,
        producerId: this.producerId,
        kind: 'video',
        source: 'camera',
        state: 'stable',
        reason: 'unknown',
        quality: 'good',
        ladderLevel: 2,
        signals: {
          sendBitrateBps: 900000, targetBitrateBps: 900000,
          lossRate: 0, rttMs: 40, jitterMs: 4,
          frameWidth: 1280, frameHeight: 720, framesPerSecond: 30,
          qualityLimitationReason: 'none',
        },
        lastAction: { type: 'noop', applied: true },
      }],
    });
    if (!resp.ok) throw new Error('clientStats failed');
    return resp;
  },

  async sendDownlinkStats({ visible, pinned = false, availableIncomingBitrate = 5_000_000, targetWidth = 1280, targetHeight = 720 }) {
    this.seq++;
    const resp = await this.sub.request('downlinkClientStats', {
      schema: 'mediasoup.qos.downlink.client.v1',
      seq: this.seq,
      tsMs: Date.now(),
      subscriberPeerId: 'sub',
      transport: { availableIncomingBitrate, currentRoundTripTime: 0.03 },
      subscriptions: [{
        consumerId: this.consumerId,
        producerId: this.producerId,
        visible,
        pinned,
        activeSpeaker: false,
        isScreenShare: false,
        targetWidth: visible ? targetWidth : 0,
        targetHeight: visible ? targetHeight : 0,
        packetsLost: 0, jitter: 0.001,
        framesPerSecond: visible ? 30 : 0,
        frameWidth: visible ? targetWidth : 0,
        frameHeight: visible ? targetHeight : 0,
        freezeRate: 0,
      }],
    });
    if (!resp.ok) throw new Error('downlinkClientStats failed');
    return resp;
  },

  async waitForTrackOverride(timeoutMs = 5000) {
    const notif = await this.pub.waitNotification('qosOverride', timeoutMs);
    return notif ? notif.data : null;
  },

  async waitForTrackOverrideWithReason(reasonPrefix, timeoutMs = 8000) {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      const idx = this.pub.notifications.findIndex(
        n => n.method === 'qosOverride' && n.data?.reason?.startsWith(reasonPrefix));
      if (idx !== -1) return this.pub.notifications.splice(idx, 1)[0].data;
      await new Promise(r => setTimeout(r, 50));
    }
    return null;
  },

  drainPublisherNotifications() {
    return this.pub.drainNotifications();
  },
};
