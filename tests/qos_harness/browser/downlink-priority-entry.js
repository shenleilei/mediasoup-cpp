/**
 * Browser-side harness for downlink priority competition with real WebRTC media.
 * 2 publishers (canvas video via mediasoup-client Device) + 1 subscriber.
 * Subscriber uses DownlinkSampler to collect real getStats() and sends
 * downlinkClientStats snapshots with actual receive metrics.
 */
import { Device } from '../../../src/client/lib/Device.js';
import { DownlinkSampler } from '../../../src/client/lib/qos/downlinkSampler.js';

// ── signaling helper ──

class WsClient {
  constructor(url) { this.url = url; this.ws = null; this.nextId = 1; this.pending = new Map(); this.notifications = []; }
  async connect() {
    this.ws = new WebSocket(this.url);
    await new Promise((ok, fail) => {
      const t = setTimeout(() => fail(new Error('ws timeout')), 5000);
      this.ws.onopen = () => { clearTimeout(t); ok(); };
      this.ws.onerror = () => { clearTimeout(t); fail(new Error('ws error')); };
      this.ws.onmessage = e => {
        const m = JSON.parse(e.data);
        if (m.response) { const cb = this.pending.get(m.id); if (cb) { this.pending.delete(m.id); cb(m); } }
        else if (m.notification) this.notifications.push(m);
      };
    });
  }
  request(method, data = {}) {
    const id = this.nextId++;
    const p = new Promise(r => this.pending.set(id, r));
    this.ws.send(JSON.stringify({ request: true, id, method, data }));
    return p;
  }
  async waitNotif(method, ms = 5000) {
    const dl = Date.now() + ms;
    while (Date.now() < dl) {
      const i = this.notifications.findIndex(n => n.method === method);
      if (i !== -1) return this.notifications.splice(i, 1)[0];
      await new Promise(r => setTimeout(r, 50));
    }
    return null;
  }
}

// ── canvas video source ──

function createCanvasTrack(label, fps = 30) {
  const c = document.createElement('canvas');
  c.width = 640; c.height = 360;
  document.body.appendChild(c);
  const ctx = c.getContext('2d');
  let tick = 0;
  const draw = () => {
    tick++;
    // high-entropy pattern so encoder produces real bitrate
    for (let y = 0; y < 18; y++) for (let x = 0; x < 32; x++) {
      const s = ((tick * 1103515245) ^ (x * 73856093) ^ (y * 19349663)) >>> 0;
      ctx.fillStyle = `rgb(${s & 0xff},${(s >> 8) & 0xff},${(s >> 16) & 0xff})`;
      ctx.fillRect(x * 20, y * 20, 20, 20);
    }
    ctx.fillStyle = '#fff'; ctx.font = '24px sans-serif';
    ctx.fillText(`${label} #${tick}`, 10, 30);
    requestAnimationFrame(draw);
  };
  draw();
  return c.captureStream(fps).getVideoTracks()[0];
}

// ── publisher helper ──

async function setupPublisher(wsUrl, roomId, peerId, ssrc) {
  const ws = new WsClient(wsUrl);
  await ws.connect();
  const join = await ws.request('join', {
    roomId, peerId, displayName: peerId,
    rtpCapabilities: { codecs: [
      { mimeType: 'audio/opus', kind: 'audio', clockRate: 48000, channels: 2, preferredPayloadType: 100 },
      { mimeType: 'video/VP8', kind: 'video', clockRate: 90000, preferredPayloadType: 101 },
    ], headerExtensions: [] },
  });
  if (!join.ok) throw new Error(`${peerId} join failed`);

  const device = new Device();
  await device.load({ routerRtpCapabilities: join.data.routerRtpCapabilities });

  const tData = await ws.request('createWebRtcTransport', { producing: true, consuming: false });
  if (!tData.ok) throw new Error(`${peerId} send transport failed`);

  const transport = device.createSendTransport(tData.data);
  transport.on('connect', async ({ dtlsParameters }, cb, eb) => {
    try { await ws.request('connectWebRtcTransport', { transportId: transport.id, dtlsParameters }); cb(); }
    catch (e) { eb(e); }
  });
  transport.on('produce', async ({ kind, rtpParameters }, cb, eb) => {
    try {
      const r = await ws.request('produce', { transportId: transport.id, kind, rtpParameters });
      cb({ id: r.data.id });
    } catch (e) { eb(e); }
  });

  const track = createCanvasTrack(peerId);
  const producer = await transport.produce({ track });
  return { ws, device, transport, producer, producerId: producer.id };
}

// ── main harness ──

window.__priorityHarness = {
  sub: null,
  subDevice: null,
  recvTransport: null,
  sampler: null,
  consumers: [], // [{label, consumerId, producerId, msConsumer}]
  subPeerId: 'sub',
  seq: 0,

  async init(wsUrl, roomId) {
    this.seq = 0;
    this.consumers = [];

    // Publishers
    this._pub1 = await setupPublisher(wsUrl, roomId, 'pub1', 99990001);
    this._pub2 = await setupPublisher(wsUrl, roomId, 'pub2', 99990002);

    // Subscriber
    this.sub = new WsClient(wsUrl);
    await this.sub.connect();
    const join = await this.sub.request('join', {
      roomId, peerId: this.subPeerId, displayName: 'sub',
      rtpCapabilities: { codecs: [
        { mimeType: 'audio/opus', kind: 'audio', clockRate: 48000, channels: 2, preferredPayloadType: 100 },
        { mimeType: 'video/VP8', kind: 'video', clockRate: 90000, preferredPayloadType: 101 },
      ], headerExtensions: [] },
    });
    if (!join.ok) throw new Error('sub join failed');

    this.subDevice = new Device();
    await this.subDevice.load({ routerRtpCapabilities: join.data.routerRtpCapabilities });

    const tData = await this.sub.request('createWebRtcTransport', { producing: false, consuming: true });
    if (!tData.ok) throw new Error('sub recv transport failed');

    this.recvTransport = this.subDevice.createRecvTransport(tData.data);
    this.recvTransport.on('connect', async ({ dtlsParameters }, cb, eb) => {
      try { await this.sub.request('connectWebRtcTransport', { transportId: this.recvTransport.id, dtlsParameters }); cb(); }
      catch (e) { eb(e); }
    });

    this.sampler = new DownlinkSampler(this.recvTransport);

    // Consume pre-created consumers from transport response
    for (const c of (tData.data.consumers || [])) {
      await this._consumeOne(c);
    }
    // Consume any newConsumer notifications
    for (let i = 0; i < 10; i++) {
      const n = await this.sub.waitNotif('newConsumer', 1000);
      if (!n) break;
      await this._consumeOne(n.data);
    }

    if (this.consumers.length < 2) throw new Error(`expected 2 consumers, got ${this.consumers.length}`);

    // Label: first = high priority, second = low priority
    this.consumers[0].label = 'high';
    this.consumers[1].label = 'low';

    return { consumers: this.consumers.map(c => ({ label: c.label, consumerId: c.consumerId, producerId: c.producerId })) };
  },

  async _consumeOne(data) {
    const msConsumer = await this.recvTransport.consume({
      id: data.id, producerId: data.producerId,
      kind: data.kind, rtpParameters: data.rtpParameters,
    });
    // Attach to hidden video element to keep decoder running
    const el = document.createElement('video');
    el.srcObject = new MediaStream([msConsumer.track]);
    el.autoplay = true; el.muted = true; el.playsInline = true;
    el.style.cssText = 'width:1px;height:1px;position:absolute;opacity:0';
    document.body.appendChild(el);
    el.play().catch(() => {});

    this.consumers.push({
      label: '', consumerId: data.id, producerId: data.producerId, msConsumer,
    });
  },

  /**
   * Send a real downlinkClientStats snapshot using DownlinkSampler (real getStats).
   * hints: [{consumerId, pinned, isScreenShare, targetWidth}]
   */
  async sendRealSnapshot(hints) {
    for (const h of hints) {
      this.sampler.setHints(h.consumerId, {
        producerId: h.producerId,
        visible: true, pinned: !!h.pinned, activeSpeaker: false,
        isScreenShare: !!h.isScreenShare,
        targetWidth: h.targetWidth || 640, targetHeight: h.targetHeight || 360,
      });
    }
    const { transport, subscriptions } = await this.sampler.sample(this.subPeerId);
    this.seq++;
    const resp = await this.sub.request('downlinkClientStats', {
      schema: 'mediasoup.qos.downlink.client.v1',
      seq: this.seq,
      tsMs: Date.now(),
      subscriberPeerId: this.subPeerId,
      transport,
      subscriptions,
    });
    if (!resp.ok) throw new Error('downlinkClientStats failed: ' + JSON.stringify(resp));
    return { transport, subscriptions };
  },

  /** Read server-side consumer state (read-only). */
  async getState(consumerId) {
    const r = await this.sub.request('getConsumerState', { consumerId });
    if (!r.ok) throw new Error('getConsumerState failed');
    return r.data;
  },

  /** Sample both consumers: server state + real subscriber stats. */
  async sampleBoth() {
    const [h, l] = await Promise.all([
      this.getState(this.consumers[0].consumerId),
      this.getState(this.consumers[1].consumerId),
    ]);
    // Also get real subscriber-side stats
    const { transport, subscriptions } = await this.sampler.sample(this.subPeerId);
    return {
      ts: Date.now(),
      high: { paused: h.paused, spatial: h.preferredSpatialLayer, temporal: h.preferredTemporalLayer, priority: h.priority },
      low:  { paused: l.paused, spatial: l.preferredSpatialLayer, temporal: l.preferredTemporalLayer, priority: l.priority },
      realTransport: transport,
      realSubscriptions: subscriptions,
    };
  },
};
