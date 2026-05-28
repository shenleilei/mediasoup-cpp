import crypto from 'node:crypto';
import dgram from 'node:dgram';
import net from 'node:net';

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function parseArgs(argv) {
  const opts = {
    wsUrl: 'ws://127.0.0.1:1770/ws',
    maxRooms: 12,
    step: 3,
    churnCycles: 20,
    churnBatch: 2,
    sampleMs: 4000,
    settleMs: 1000,
    roomPrefix: `plain_room_${Date.now()}`,
    payloadSize: 1200,
    ppsPerRoom: 300,
    recvRatio: 0.9,
    explicitConnect: true,
  };

  for (const arg of argv) {
    if (!arg.startsWith('--')) continue;
    const [key, rawValue = ''] = arg.slice(2).split('=');
    const int = value => Number.parseInt(value, 10);
    const float = value => Number.parseFloat(value);
    switch (key) {
      case 'ws-url': opts.wsUrl = rawValue || opts.wsUrl; break;
      case 'max-rooms': opts.maxRooms = Math.max(1, int(rawValue)); break;
      case 'step': opts.step = Math.max(1, int(rawValue)); break;
      case 'churn-cycles': opts.churnCycles = Math.max(0, int(rawValue)); break;
      case 'churn-batch': opts.churnBatch = Math.max(1, int(rawValue)); break;
      case 'sample-ms': opts.sampleMs = Math.max(1, int(rawValue)); break;
      case 'settle-ms': opts.settleMs = Math.max(0, int(rawValue)); break;
      case 'room-prefix': opts.roomPrefix = rawValue || opts.roomPrefix; break;
      case 'payload-size': opts.payloadSize = Math.max(64, int(rawValue)); break;
      case 'pps': opts.ppsPerRoom = Math.max(1, int(rawValue)); break;
      case 'recv-ratio': opts.recvRatio = Math.min(1, Math.max(0, float(rawValue))); break;
      case 'explicit-connect': opts.explicitConnect = true; break;
      case 'no-explicit-connect': opts.explicitConnect = false; break;
      default:
        throw new Error(`unknown option: --${key}`);
    }
  }

  return opts;
}

class WsJsonClient {
  constructor(host, port, path = '/ws') {
    this.host = host;
    this.port = port;
    this.path = path;
    this.socket = null;
    this.pending = Buffer.alloc(0);
    this.nextId = 1;
    this.responses = new Map();
    this.notifications = [];
  }

  async connect() {
    this.socket = net.createConnection({ host: this.host, port: this.port });
    await new Promise((resolve, reject) => {
      this.socket.once('connect', resolve);
      this.socket.once('error', reject);
    });

    const key = crypto.randomBytes(16).toString('base64');
    const req =
      `GET ${this.path} HTTP/1.1\r\n` +
      `Host: ${this.host}:${this.port}\r\n` +
      'Upgrade: websocket\r\n' +
      'Connection: Upgrade\r\n' +
      `Sec-WebSocket-Key: ${key}\r\n` +
      'Sec-WebSocket-Version: 13\r\n' +
      `Origin: http://${this.host}\r\n\r\n`;
    this.socket.write(req);

    const header = await this._readHttpHeader();
    if (!header.includes('101')) {
      throw new Error(`websocket handshake failed: ${header}`);
    }

    this.socket.on('data', chunk => this._onData(chunk));
  }

  close() {
    try {
      this.socket?.destroy();
    } catch {}
    this.socket = null;
  }

  async request(method, data = {}, timeoutMs = 10000) {
    const id = this.nextId++;
    this._sendJson({ request: true, id, method, data });
    return await this._waitResponse(id, timeoutMs);
  }

  async waitNotification(method, timeoutMs = 5000) {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      const idx = this.notifications.findIndex(n => n.method === method);
      if (idx !== -1) return this.notifications.splice(idx, 1)[0];
      await sleep(25);
    }
    return null;
  }

  _sendJson(value) {
    const payload = Buffer.from(JSON.stringify(value), 'utf8');
    this._sendFrame(0x1, payload);
  }

  _sendControlFrame(opcode, payload = Buffer.alloc(0)) {
    if (payload.length > 125) {
      throw new Error('control frame payload too large');
    }
    this._sendFrame(opcode, payload);
  }

  _sendFrame(opcode, payload) {
    if (!this.socket || this.socket.destroyed) return;
    const mask = crypto.randomBytes(4);
    const header = [0x80 | opcode];
    if (payload.length < 126) {
      header.push(0x80 | payload.length);
    } else if (payload.length < 65536) {
      header.push(0x80 | 126, (payload.length >> 8) & 0xff, payload.length & 0xff);
    } else {
      throw new Error('payload too large');
    }
    const masked = Buffer.alloc(payload.length);
    for (let i = 0; i < payload.length; ++i) {
      masked[i] = payload[i] ^ mask[i % 4];
    }
    this.socket.write(Buffer.concat([Buffer.from(header), mask, masked]));
  }

  async _waitResponse(id, timeoutMs) {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      if (this.responses.has(id)) {
        const msg = this.responses.get(id);
        this.responses.delete(id);
        return msg;
      }
      await sleep(10);
    }
    throw new Error(`request ${id} timed out`);
  }

  async _readHttpHeader() {
    while (true) {
      const idx = this.pending.indexOf('\r\n\r\n');
      if (idx !== -1) {
        const header = this.pending.slice(0, idx + 4).toString('utf8');
        this.pending = this.pending.slice(idx + 4);
        return header;
      }
      await new Promise((resolve, reject) => {
        const socket = this.socket;
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

  _onData(chunk) {
    this.pending = Buffer.concat([this.pending, chunk]);
    while (this.pending.length >= 2) {
      const opcode = this.pending[0] & 0x0f;
      const masked = (this.pending[1] & 0x80) !== 0;
      let payloadLength = this.pending[1] & 0x7f;
      let offset = 2;
      if (payloadLength === 126) {
        if (this.pending.length < 4) return;
        payloadLength = this.pending.readUInt16BE(2);
        offset = 4;
      } else if (payloadLength === 127) {
        throw new Error('unsupported long websocket frame');
      }
      if (masked) throw new Error('server frame must not be masked');
      if (this.pending.length < offset + payloadLength) return;
      const payload = this.pending.slice(offset, offset + payloadLength);
      this.pending = this.pending.slice(offset + payloadLength);
      if (opcode === 0x8) {
        this._sendControlFrame(0x8, payload);
        this.close();
        return;
      }
      if (opcode === 0x9) {
        this._sendControlFrame(0xA, payload);
        continue;
      }
      if (opcode === 0xA) continue;
      if (opcode !== 0x1) continue;
      const msg = JSON.parse(payload.toString('utf8'));
      if (msg.response === true) {
        this.responses.set(msg.id, msg);
      } else if (msg.notification === true) {
        this.notifications.push(msg);
      }
    }
  }
}

function createUdpReceiver() {
  const sock = dgram.createSocket('udp4');
  let port = 0;
  let packets = 0;
  sock.on('message', () => { packets += 1; });
  return new Promise((resolve, reject) => {
    sock.once('error', reject);
    sock.bind(0, '127.0.0.1', () => {
      port = sock.address().port;
      sock.off('error', reject);
      resolve({ sock, port, get packets() { return packets; } });
    });
  });
}

function createUdpSender() {
  const sock = dgram.createSocket('udp4');
  return new Promise((resolve, reject) => {
    sock.once('error', reject);
    sock.bind(0, '127.0.0.1', () => {
      const port = sock.address().port;
      sock.off('error', reject);
      resolve({
        sock,
        port,
        close() {
          try { sock.close(); } catch {}
        },
        send(buf, port, host) {
          sock.send(buf, port, host);
        },
      });
    });
  });
}

function buildRtpPacket({ pt, seq, ts, ssrc, payloadSize, marker }) {
  const buf = Buffer.alloc(payloadSize);
  buf[0] = 0x80;
  buf[1] = pt & 0x7f;
  if (marker) buf[1] |= 0x80;
  buf.writeUInt16BE(seq & 0xffff, 2);
  buf.writeUInt32BE(ts >>> 0, 4);
  buf.writeUInt32BE(ssrc >>> 0, 8);
  for (let i = 12; i < payloadSize; ++i) {
    buf[i] = (seq + i) & 0xff;
  }
  return buf;
}

const RTP_CAPS = {
  codecs: [
    { mimeType: 'audio/opus', kind: 'audio', clockRate: 48000, channels: 2, preferredPayloadType: 100 },
    { mimeType: 'video/VP8', kind: 'video', clockRate: 90000, preferredPayloadType: 101 },
  ],
  headerExtensions: [],
};

async function joinPeer(ws, roomId, peerId) {
  const resp = await ws.request('join', {
    roomId,
    peerId,
    displayName: peerId,
    rtpCapabilities: RTP_CAPS,
  });
  if (!resp.ok) throw new Error(`${peerId} join failed: ${JSON.stringify(resp)}`);
  return resp;
}

function sumRtpStatsFields(target, source) {
  if (!source || typeof source !== 'object') return;
  for (const field of [
    'packetsLost',
    'nackCount',
    'nackPacketCount',
    'packetsRetransmitted',
    'packetsRepaired',
    'rtpPacketLossReceived',
    'rtpPacketLossSent',
  ]) {
    const value = Number(source[field]);
    if (Number.isFinite(value)) {
      target[field] += value;
    }
  }
}

function summarizePeerStats(resp) {
  const summary = {
    packetsLost: 0,
    nackCount: 0,
    nackPacketCount: 0,
    packetsRetransmitted: 0,
    packetsRepaired: 0,
    rtpPacketLossReceived: 0,
    rtpPacketLossSent: 0,
  };
  const data = resp?.data || {};
  sumRtpStatsFields(summary, data.sendTransport);
  sumRtpStatsFields(summary, data.recvTransport);
  for (const producer of Object.values(data.producers || {})) {
    for (const stat of producer.stats || []) {
      sumRtpStatsFields(summary, stat);
    }
  }
  return summary;
}

async function sampleRtpStats(room) {
  const [pubStats, sub1Stats, sub2Stats] = await Promise.all([
    room.pubWs.request('getStats', { peerId: 'pub' }),
    room.sub1Ws.request('getStats', { peerId: 'sub1' }),
    room.sub2Ws.request('getStats', { peerId: 'sub2' }),
  ]);
  return {
    pub: summarizePeerStats(pubStats),
    sub1: summarizePeerStats(sub1Stats),
    sub2: summarizePeerStats(sub2Stats),
  };
}

async function createRoom(roomIndex, opts, wsHost, wsPort) {
  const roomId = `${opts.roomPrefix}_${roomIndex}`;
  const pubWs = new WsJsonClient(wsHost, wsPort);
  const sub1Ws = new WsJsonClient(wsHost, wsPort);
  const sub2Ws = new WsJsonClient(wsHost, wsPort);
  await Promise.all([pubWs.connect(), sub1Ws.connect(), sub2Ws.connect()]);

  await Promise.all([
    joinPeer(pubWs, roomId, 'pub'),
    joinPeer(sub1Ws, roomId, 'sub1'),
    joinPeer(sub2Ws, roomId, 'sub2'),
  ]);

  const sender = await createUdpSender();

  const plainPublishReq = {
    videoSsrc: 90000001 + roomIndex,
    videoSsrcs: [90000001 + roomIndex],
    videoCodec: 'vp8',
    enableAudio: false,
  };
  if (opts.explicitConnect) {
    plainPublishReq.senderIp = '127.0.0.1';
    plainPublishReq.senderPort = sender.port;
  }
  const pubResp = await pubWs.request('plainPublish', plainPublishReq);
  if (!pubResp.ok) throw new Error(`plainPublish failed for ${roomId}: ${JSON.stringify(pubResp)}`);

  const recv1 = await createUdpReceiver();
  const recv2 = await createUdpReceiver();

  const sub1Resp = await sub1Ws.request('plainSubscribe', {
    recvIp: '127.0.0.1',
    recvPort: recv1.port,
  });
  if (!sub1Resp.ok) throw new Error(`plainSubscribe sub1 failed for ${roomId}: ${JSON.stringify(sub1Resp)}`);

  const sub2Resp = await sub2Ws.request('plainSubscribe', {
    recvIp: '127.0.0.1',
    recvPort: recv2.port,
  });
  if (!sub2Resp.ok) throw new Error(`plainSubscribe sub2 failed for ${roomId}: ${JSON.stringify(sub2Resp)}`);

  const serverHost = '127.0.0.1';
  const serverPort = pubResp.data.port;
  const videoPt = pubResp.data.videoPt;
  const ssrc = pubResp.data.videoSsrc;
  let seq = 1;
  let ts = 0;
  const burstPackets = Math.max(1, Math.floor(opts.ppsPerRoom / 30));
  const burstIntervalMs = 33;
  const payloadSize = opts.payloadSize;
  const timer = setInterval(() => {
    try {
      for (let i = 0; i < burstPackets; ++i) {
        const marker = i === burstPackets - 1;
        const packet = buildRtpPacket({
          pt: videoPt,
          seq: seq++,
          ts,
          ssrc,
          payloadSize,
          marker,
        });
        sender.send(packet, serverPort, serverHost);
      }
      ts += 3000;
    } catch (error) {
      console.error(`[${roomId}] sender error: ${error.message}`);
    }
  }, burstIntervalMs);

  return {
    roomId,
    pubWs,
    sub1Ws,
    sub2Ws,
    sender,
    recv1,
    recv2,
    timer,
    getSend() { return seq - 1; },
    getRecv1() { return recv1.packets; },
    getRecv2() { return recv2.packets; },
    close() {
      clearInterval(timer);
      sender.close();
      try { recv1.sock.close(); } catch {}
      try { recv2.sock.close(); } catch {}
      pubWs.close();
      sub1Ws.close();
      sub2Ws.close();
    },
  };
}

async function sampleRooms(rooms, sampleMs, recvRatio) {
  const before = rooms.map(room => ({
    send: room.getSend(),
    recv1: room.getRecv1(),
    recv2: room.getRecv2(),
  }));
  await sleep(sampleMs);
  const after = rooms.map(room => ({
    send: room.getSend(),
    recv1: room.getRecv1(),
    recv2: room.getRecv2(),
  }));

  const details = rooms.map((room, index) => {
    const sendDelta = after[index].send - before[index].send;
    const recv1Delta = after[index].recv1 - before[index].recv1;
    const recv2Delta = after[index].recv2 - before[index].recv2;
    const recvOk1 = sendDelta === 0 || recv1Delta >= sendDelta * recvRatio;
    const recvOk2 = sendDelta === 0 || recv2Delta >= sendDelta * recvRatio;
    return {
      roomId: room.roomId,
      sendDelta,
      recv1Delta,
      recv2Delta,
      healthy: sendDelta > 0 && recvOk1 && recvOk2,
      reasons: [
        ...(sendDelta <= 0 ? ['send=0'] : []),
        ...(!recvOk1 ? [`recv1=${recv1Delta}/${sendDelta}`] : []),
        ...(!recvOk2 ? [`recv2=${recv2Delta}/${sendDelta}`] : []),
      ],
    };
  });

  const healthyRooms = details.filter(d => d.healthy).length;
  const totalSend = details.reduce((sum, d) => sum + d.sendDelta, 0);
  const totalRecv1 = details.reduce((sum, d) => sum + d.recv1Delta, 0);
  const totalRecv2 = details.reduce((sum, d) => sum + d.recv2Delta, 0);
  const totalRecv = totalRecv1 + totalRecv2;

  return {
    healthy: healthyRooms === rooms.length,
    totalRooms: rooms.length,
    healthyRooms,
    sendPps: totalSend / (sampleMs / 1000),
    recv1Pps: totalRecv1 / (sampleMs / 1000),
    recv2Pps: totalRecv2 / (sampleMs / 1000),
    recvRatio: totalSend > 0 ? totalRecv / (totalSend * 2) : 0,
    details,
  };
}

async function main() {
  const opts = parseArgs(process.argv.slice(2));
  const wsUrl = new URL(opts.wsUrl);
  const wsHost = wsUrl.hostname;
  const wsPort = Number.parseInt(wsUrl.port || '80', 10);

  console.log(`ws=${opts.wsUrl}`);
  console.log(`config rooms=${opts.maxRooms} step=${opts.step} churnCycles=${opts.churnCycles} churnBatch=${opts.churnBatch}`);

  const rooms = [];
  let nextRoomIndex = 1;
  let peakHealthyRooms = 0;

  const addRooms = async count => {
    for (let i = 0; i < count; ++i) {
      const room = await createRoom(nextRoomIndex++, opts, wsHost, wsPort);
      rooms.push(room);
    }
  };

  const removeRooms = async count => {
    const removed = rooms.splice(0, Math.min(count, rooms.length));
    for (const room of removed) room.close();
  };

  try {
    while (rooms.length < opts.maxRooms) {
      const addCount = Math.min(opts.step, opts.maxRooms - rooms.length);
      await addRooms(addCount);
      const statsRoom = rooms[0] || null;
      const rtpStatsBefore = statsRoom ? await sampleRtpStats(statsRoom) : null;
      const sample = await sampleRooms(rooms, opts.sampleMs, opts.recvRatio);
      const rtpStatsAfter = statsRoom ? await sampleRtpStats(statsRoom) : null;
      peakHealthyRooms = Math.max(peakHealthyRooms, sample.healthyRooms);
      console.log(`[ramp rooms=${sample.totalRooms}] healthy=${sample.healthyRooms}/${sample.totalRooms} send=${sample.sendPps.toFixed(0)} recv1=${sample.recv1Pps.toFixed(0)} recv2=${sample.recv2Pps.toFixed(0)} ratio=${sample.recvRatio.toFixed(2)} ` +
        (rtpStatsBefore && rtpStatsAfter
          ? `rtp(pub nack=${rtpStatsAfter.pub.nackCount - rtpStatsBefore.pub.nackCount} ` +
            `lost=${rtpStatsAfter.pub.packetsLost - rtpStatsBefore.pub.packetsLost} ` +
            `reTx=${rtpStatsAfter.pub.packetsRetransmitted - rtpStatsBefore.pub.packetsRetransmitted} ` +
            `sub1Loss=${rtpStatsAfter.sub1.rtpPacketLossReceived - rtpStatsBefore.sub1.rtpPacketLossReceived} ` +
            `sub2Loss=${rtpStatsAfter.sub2.rtpPacketLossReceived - rtpStatsBefore.sub2.rtpPacketLossReceived})`
          : ''));
      if (!sample.healthy) {
        const bad = sample.details.filter(d => !d.healthy).slice(0, 5);
        for (const item of bad) console.log(`  fail ${item.roomId}: ${item.reasons.join(', ')}`);
        break;
      }
    }

    for (let cycle = 0; cycle < opts.churnCycles && rooms.length > 0; ++cycle) {
      const churnCount = Math.min(opts.churnBatch, rooms.length);
      await removeRooms(churnCount);
      if (opts.settleMs > 0) await sleep(opts.settleMs);
      await addRooms(churnCount);
      const statsRoom = rooms[0] || null;
      const rtpStatsBefore = statsRoom ? await sampleRtpStats(statsRoom) : null;
      const sample = await sampleRooms(rooms, opts.sampleMs, opts.recvRatio);
      const rtpStatsAfter = statsRoom ? await sampleRtpStats(statsRoom) : null;
      peakHealthyRooms = Math.max(peakHealthyRooms, sample.healthyRooms);
      console.log(`[churn ${cycle + 1}] rooms=${sample.totalRooms} healthy=${sample.healthyRooms}/${sample.totalRooms} send=${sample.sendPps.toFixed(0)} recv1=${sample.recv1Pps.toFixed(0)} recv2=${sample.recv2Pps.toFixed(0)} ratio=${sample.recvRatio.toFixed(2)} ` +
        (rtpStatsBefore && rtpStatsAfter
          ? `rtp(pub nack=${rtpStatsAfter.pub.nackCount - rtpStatsBefore.pub.nackCount} ` +
            `lost=${rtpStatsAfter.pub.packetsLost - rtpStatsBefore.pub.packetsLost} ` +
            `reTx=${rtpStatsAfter.pub.packetsRetransmitted - rtpStatsBefore.pub.packetsRetransmitted} ` +
            `sub1Loss=${rtpStatsAfter.sub1.rtpPacketLossReceived - rtpStatsBefore.sub1.rtpPacketLossReceived} ` +
            `sub2Loss=${rtpStatsAfter.sub2.rtpPacketLossReceived - rtpStatsBefore.sub2.rtpPacketLossReceived})`
          : ''));
      if (!sample.healthy) {
        const bad = sample.details.filter(d => !d.healthy).slice(0, 5);
        for (const item of bad) console.log(`  fail ${item.roomId}: ${item.reasons.join(', ')}`);
        break;
      }
    }

    console.log(`peak healthy rooms: ${peakHealthyRooms}`);
  } finally {
    for (const room of rooms) room.close();
    await sleep(500);
  }
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
