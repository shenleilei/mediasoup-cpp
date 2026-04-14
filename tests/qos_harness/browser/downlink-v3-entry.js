/**
 * Browser-side harness for downlink QoS v3 matrix verification.
 * Supports: single-sub init, single-sub dual-consumer competition (D7),
 * server-side consumer state query, per-step trace collection,
 * and resume oscillation detection.
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
  pub2: null,
  sub: null,
  sub2: null,
  roomId: null,
  producerId: null,
  producerId2: null,
  consumerId: null,
  consumerId2: null,
  trackId: 'camera-main',
  trackId2: 'camera-side',
  seq: 0,
  trace: [],
  overrideLog: [],

  async init(wsUrl, roomId) {
    this.roomId = roomId;
    this.seq = 0;
    this.trace = [];
    this.overrideLog = [];
    this.pub2 = null;
    this.sub2 = null;
    this.producerId2 = null;
    this.consumerId2 = null;

    this.pub = new JsonWsClient(wsUrl);
    await this.pub.connect();
    const joinPub = await this.pub.request('join', {
      roomId, peerId: 'pub', displayName: 'pub', rtpCapabilities: RTP_CAPS,
    });
    if (!joinPub.ok) throw new Error('pub join failed: ' + JSON.stringify(joinPub));

    this.sub = new JsonWsClient(wsUrl);
    await this.sub.connect();
    const joinSub = await this.sub.request('join', {
      roomId, peerId: 'sub', displayName: 'sub', rtpCapabilities: RTP_CAPS,
    });
    if (!joinSub.ok) throw new Error('sub join failed: ' + JSON.stringify(joinSub));

    const subRecv = await this.sub.request('createWebRtcTransport', { producing: false, consuming: true });
    if (!subRecv.ok) throw new Error('sub recv transport failed');

    const pubSend = await this.pub.request('createWebRtcTransport', { producing: true, consuming: false });
    if (!pubSend.ok) throw new Error('pub send transport failed');

    const prodResp = await this.pub.request('produce', {
      transportId: pubSend.data.id, kind: 'video',
      rtpParameters: {
        codecs: [{ mimeType: 'video/VP8', clockRate: 90000, payloadType: 101 }],
        encodings: [{ ssrc: 99990201 }], mid: '0',
      },
    });
    if (!prodResp.ok) throw new Error('produce failed: ' + JSON.stringify(prodResp));
    this.producerId = prodResp.data.id;

    const consumerNotif = await this.sub.waitNotification('newConsumer', 5000);
    if (!consumerNotif) throw new Error('newConsumer not received');
    this.consumerId = consumerNotif.data.id;

    return { producerId: this.producerId, consumerId: this.consumerId, trackId: this.trackId };
  },

  /** Add a second publisher so one subscriber holds two competing consumers. */
  async initSecondPublisher(wsUrl) {
    this.pub2 = new JsonWsClient(wsUrl);
    await this.pub2.connect();
    const joinPub2 = await this.pub2.request('join', {
      roomId: this.roomId, peerId: 'pub2', displayName: 'pub2', rtpCapabilities: RTP_CAPS,
    });
    if (!joinPub2.ok) throw new Error('pub2 join failed: ' + JSON.stringify(joinPub2));

    const pubSend2 = await this.pub2.request('createWebRtcTransport', { producing: true, consuming: false });
    if (!pubSend2.ok) throw new Error('pub2 send transport failed');

    const prodResp2 = await this.pub2.request('produce', {
      transportId: pubSend2.data.id, kind: 'video',
      rtpParameters: {
        codecs: [{ mimeType: 'video/VP8', clockRate: 90000, payloadType: 101 }],
        encodings: [{ ssrc: 99990202 }], mid: '0',
      },
    });
    if (!prodResp2.ok) throw new Error('pub2 produce failed: ' + JSON.stringify(prodResp2));
    this.producerId2 = prodResp2.data.id;

    const consumerNotif2 = await this.sub.waitNotification('newConsumer', 5000);
    if (!consumerNotif2) throw new Error('second consumer not received');
    this.consumerId2 = consumerNotif2.data.id;

    return { producerId2: this.producerId2, consumerId2: this.consumerId2, trackId2: this.trackId2 };
  },

  /** Set consumer priority via server API. */
  async setConsumerPriority(subClient, consumerId, priority) {
    const resp = await subClient.request('setConsumerPriority', { consumerId, priority });
    if (!resp.ok) throw new Error('setConsumerPriority failed');
    return resp;
  },

  async publishPublisherStats(client = this.pub, producerId = this.producerId, trackId = this.trackId) {
    this.seq++;
    const resp = await client.request('clientStats', {
      schema: 'mediasoup.qos.client.v1', seq: this.seq, tsMs: Date.now(),
      peerState: { mode: 'audio-video', quality: 'good', stale: false },
      tracks: [{
        localTrackId: trackId, producerId,
        kind: 'video', source: 'camera', state: 'stable', reason: 'unknown',
        quality: 'good', ladderLevel: 2,
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

  async sendDownlinkStatsBatch({
    subscriptions,
    availableIncomingBitrate = 5_000_000,
    currentRoundTripTime = 0.03,
  }, subClient) {
    const client = subClient || this.sub;
    const peerId = client === this.sub2 ? 'sub2' : 'sub';
    this.seq++;
    const resp = await client.request('downlinkClientStats', {
      schema: 'mediasoup.qos.downlink.client.v1', seq: this.seq, tsMs: Date.now(),
      subscriberPeerId: peerId,
      transport: { availableIncomingBitrate, currentRoundTripTime },
      subscriptions,
    });
    if (!resp.ok) throw new Error('downlinkClientStats failed');
    return resp;
  },

  async sendDownlinkStats({ visible, pinned = false, availableIncomingBitrate = 5_000_000, targetWidth = 1280, targetHeight = 720, currentRoundTripTime = 0.03, packetsLost = 0, jitter = 0.001 }, subClient, consumerId, producerId) {
    const cId = consumerId || this.consumerId;
    const pId = producerId || this.producerId;
    return this.sendDownlinkStatsBatch({
      availableIncomingBitrate,
      currentRoundTripTime,
      subscriptions: [{
        consumerId: cId, producerId: pId,
        visible, pinned, activeSpeaker: false, isScreenShare: false,
        targetWidth: visible ? targetWidth : 0, targetHeight: visible ? targetHeight : 0,
        packetsLost, jitter,
        framesPerSecond: visible ? 30 : 0,
        frameWidth: visible ? targetWidth : 0, frameHeight: visible ? targetHeight : 0,
        freezeRate: 0,
      }],
    }, subClient);
  },

  /** Query server-side consumer state (paused, preferredSpatialLayer, priority). */
  async queryConsumerState(subClient, consumerId) {
    const client = subClient || this.sub;
    const cId = consumerId || this.consumerId;
    const resp = await client.request('getConsumerState', { consumerId: cId });
    if (!resp.ok) return null;
    return resp.data;
  },

  /** Record a trace entry with timestamp, override events, and consumer state. */
  recordTraceEntry(phase, consumerState, overrides) {
    this.trace.push({
      tsMs: Date.now(), phase,
      consumerState: consumerState || null,
      overrides: overrides || [],
    });
  },

  publisherClients() {
    return [this.pub, this.pub2].filter(Boolean);
  },

  matchesOverride(entry, matcher = {}) {
    if (!entry) return false;
    if (matcher.reasonPrefix && !entry.reason?.startsWith(matcher.reasonPrefix)) return false;
    if (matcher.pauseUpstream === true && entry.pauseUpstream !== true) return false;
    if (matcher.resumeUpstream === true && entry.resumeUpstream !== true) return false;
    if (matcher.hasMaxLevelClamp === true && typeof entry.maxLevelClamp !== 'number') return false;
    return true;
  },

  /** Drain publisher overrides and log them. */
  drainAndLogOverrides() {
    const notifs = this.publisherClients().flatMap(client => client.drainNotifications());
    const overrides = notifs
      .filter(n => n.method === 'qosOverride')
      .map(n => ({ tsMs: Date.now(), ...n.data }));
    this.overrideLog.push(...overrides);
    return overrides;
  },

  async waitForTrackOverride(timeoutMs = 5000) {
    const notif = await this.pub.waitNotification('qosOverride', timeoutMs);
    return notif ? notif.data : null;
  },

  async waitForOverride(matcher = {}, timeoutMs = 8000, afterTsMs = 0) {
    const findInLog = () => this.overrideLog.find(
      o => o.tsMs >= afterTsMs && this.matchesOverride(o, matcher),
    ) || null;

    const existing = findInLog();
    if (existing) return existing;

    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      for (const client of this.publisherClients()) {
        const idx = client.notifications.findIndex(
          n => n.method === 'qosOverride' && this.matchesOverride(n.data, matcher),
        );
        if (idx !== -1) {
          const data = client.notifications.splice(idx, 1)[0].data;
          const entry = { tsMs: Date.now(), ...data };
          this.overrideLog.push(entry);
          return entry;
        }
      }
      await new Promise(r => setTimeout(r, 50));
    }
    return null;
  },

  async waitForTrackOverrideWithReason(reasonPrefix, timeoutMs = 8000, afterTsMs = 0) {
    return this.waitForOverride({ reasonPrefix }, timeoutMs, afterTsMs);
  },

  drainPublisherNotifications() {
    return this.publisherClients().flatMap(client => client.drainNotifications());
  },

  /** Check for pause/resume oscillation in the override log within a time window. */
  detectOscillation(windowOrRange = 5000) {
    const now = Date.now();
    const startMs = typeof windowOrRange === 'number'
      ? now - windowOrRange
      : (windowOrRange?.startMs ?? 0);
    const endMs = typeof windowOrRange === 'number'
      ? now
      : (windowOrRange?.endMs ?? now);
    const recent = this.overrideLog.filter(o => o.tsMs >= startMs && o.tsMs <= endMs);
    let pauseCount = 0, resumeCount = 0;
    for (const o of recent) {
      if (o.pauseUpstream === true) pauseCount++;
      if (o.resumeUpstream === true) resumeCount++;
    }
    return { oscillation: pauseCount >= 2 || resumeCount >= 2, pauseCount, resumeCount };
  },

  getTrace() { return this.trace; },
  getOverrideLog() { return this.overrideLog; },
};
