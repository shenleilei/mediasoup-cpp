import crypto from 'node:crypto';
import dgram from 'node:dgram';
import { createWebSocketConnection, httpGetJson, websocketOriginForUrl } from './node_tls_helpers.mjs';
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function parseArgs(argv) {
  const opts = {
    wsUrl: 'wss://127.0.0.1:1770/ws',
    httpUrl: 'https://127.0.0.1:1770',
    container: 'mediasoup-cpp',
    sampleHost: '',
    maxRooms: Number.POSITIVE_INFINITY,
    step: 10,
    roundMs: 10000,
    steadyRoundMs: null,
    holdAfterMax: false,
    steadyRounds: 0,
    roomPrefix: `pressure_room_${Date.now()}`,
    payloadSize: 1200,
    ppsPerRoom: 300,
    recvRatio: 0.9,
    explicitConnect: true,
    continueOnFailure: false,
    freezeOnFailure: true,
  };

  for (const arg of argv) {
    if (!arg.startsWith('--')) continue;
    const [key, rawValue = ''] = arg.slice(2).split('=');
    const int = value => Number.parseInt(value, 10);
    const float = value => Number.parseFloat(value);
    switch (key) {
      case 'ws-url': opts.wsUrl = rawValue || opts.wsUrl; break;
      case 'http-url': opts.httpUrl = rawValue || opts.httpUrl; break;
      case 'container': opts.container = rawValue || opts.container; break;
      case 'sample-host': opts.sampleHost = rawValue || opts.sampleHost; break;
      case 'max-rooms': opts.maxRooms = Math.max(1, int(rawValue)); break;
      case 'step': opts.step = Math.max(1, int(rawValue)); break;
      case 'round-ms': opts.roundMs = Math.max(1, int(rawValue)); break;
      case 'steady-round-ms': opts.steadyRoundMs = Math.max(1, int(rawValue)); break;
      case 'hold-after-max': opts.holdAfterMax = true; break;
      case 'steady-rounds': opts.steadyRounds = Math.max(0, int(rawValue)); break;
      case 'room-prefix': opts.roomPrefix = rawValue || opts.roomPrefix; break;
      case 'payload-size': opts.payloadSize = Math.max(64, int(rawValue)); break;
      case 'pps': opts.ppsPerRoom = Math.max(1, int(rawValue)); break;
      case 'recv-ratio': opts.recvRatio = Math.min(1, Math.max(0, float(rawValue))); break;
      case 'explicit-connect': opts.explicitConnect = true; break;
      case 'no-explicit-connect': opts.explicitConnect = false; break;
      case 'continue-on-failure': opts.continueOnFailure = true; break;
      case 'stop-on-failure': opts.continueOnFailure = false; break;
      case 'freeze-on-failure': opts.freezeOnFailure = true; break;
      case 'no-freeze-on-failure': opts.freezeOnFailure = false; break;
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
    const protocol = this.path.startsWith('/') ? 'wss:' : 'wss:';
    const url = new URL(`${protocol}//${this.host}:${this.port}${this.path}`);
    this.socket = createWebSocketConnection(url);
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
      `Origin: ${websocketOriginForUrl(url)}\r\n\r\n`;
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
  let packets = 0;
  sock.on('message', () => { packets += 1; });
  return new Promise((resolve, reject) => {
    sock.once('error', reject);
    sock.bind(0, '127.0.0.1', () => {
      const port = sock.address().port;
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

async function joinPeer(ws, roomId, peerId) {
  const resp = await ws.request('join', {
    roomId,
    peerId,
    displayName: peerId,
    rtpCapabilities: {
      codecs: [
        { mimeType: 'audio/opus', kind: 'audio', clockRate: 48000, channels: 2, preferredPayloadType: 100 },
        { mimeType: 'video/VP8', kind: 'video', clockRate: 90000, preferredPayloadType: 101 },
      ],
      headerExtensions: [],
    },
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

async function sampleHttpJson(httpUrl, path) {
  return await httpGetJson(new URL(httpUrl), path);
}

function shellQuote(value) {
  return `'${String(value).replace(/'/g, `'\"'\"'`)}'`;
}

function execRemoteCommand(sampleHost, command) {
  if (sampleHost) {
    return execFileSync('ssh', ['-o', 'BatchMode=yes', sampleHost, command], { encoding: 'utf8' });
  }
  return execFileSync('sh', ['-lc', command], { encoding: 'utf8' });
}

function sampleContainerProcesses(container, sampleHost = '') {
  try {
    const script = `
      read cpu user nice system idle iowait irq softirq steal guest guest_nice < /proc/stat
      total=$((user + nice + system + idle + iowait + irq + softirq + steal + guest + guest_nice))
      sample_pid() {
        pid="$1"
        name="$2"
        if [ -r "/proc/$pid/stat" ] && [ -r "/proc/$pid/status" ]; then
          stat=$(cat "/proc/$pid/stat")
          rss=$(grep '^VmRSS:' "/proc/$pid/status" | tr -s ' ' | cut -d ' ' -f2)
          rss=\${rss:-0}
          echo "$name|$total|$stat|$rss"
        fi
      }
      find_pid_by_name() {
        primary="$1"
        fallback="$2"
        for status in /proc/[0-9]*/status; do
          [ -r "$status" ] || continue
          name=$(awk '/^Name:/ {print $2; exit}' "$status")
          if [ "$name" = "$primary" ] || { [ -n "$fallback" ] && [ "$name" = "$fallback" ]; }; then
            pid="\${status%/status}"
            echo "\${pid##*/}"
            return
          fi
        done
      }
      workerPid=$(find_pid_by_name mediasoup-worke mediasoup-worker)
      sfuPid=$(find_pid_by_name mediasoup-sfu '')
      sample_pid "$workerPid" mediasoup-worker
      sample_pid "$sfuPid" mediasoup-sfu
      if [ -n "$workerPid" ]; then
        ps -L -p "$workerPid" -o tid=,pcpu=,comm= 2>/dev/null | while read tid cpu comm rest; do
          if [ "$comm" = "mediasoup-worke" ] || [ "$comm" = "mediasoup-worker" ]; then
            echo "thread|$tid|$cpu|$comm"
          fi
        done
      fi
    `;
    const output = execRemoteCommand(
      sampleHost,
      `docker exec ${shellQuote(container)} sh -lc ${shellQuote(script)}`
    ).trim();
    const rows = output ? output.split('\n') : [];
    const result = {};
    for (const row of rows) {
      const parts = row.split('|');
      if (parts[0] === 'thread') {
        const tid = Number.parseInt(parts[1], 10);
        const cpu = Number.parseFloat(parts[2] || '0');
        if (Number.isFinite(cpu) && cpu >= (result['mediasoup-worker-thread']?.cpu ?? 0)) {
          result['mediasoup-worker-thread'] = { cpu, tid };
        }
        continue;
      }
      if (parts.length < 4) continue;
      const name = parts[0];
      const total = Number.parseInt(parts[1], 10) || 0;
      const stat = parts[2] || '';
      const rssKb = Number.parseInt(parts[3], 10) || 0;
      if (name === 'mediasoup-sfu' || name === 'mediasoup-worker') {
        const rparen = stat.lastIndexOf(')');
        const rest = rparen !== -1 ? stat.slice(rparen + 2).split(/\s+/) : [];
        const utime = Number.parseInt(rest[11], 10) || 0;
        const stime = Number.parseInt(rest[12], 10) || 0;
        result[name] = { total, proc: utime + stime, rssMb: rssKb / 1024 };
      }
    }
    return result;
  } catch (error) {
    return { error: error.message };
  }
}

function sampleLoadAvg() {
  try {
    const raw = fs.readFileSync('/proc/loadavg', 'utf8').trim();
    const [one, five, fifteen] = raw.split(/\s+/).slice(0, 3).map(Number.parseFloat);
    return { one, five, fifteen };
  } catch (error) {
    return { error: error.message };
  }
}

function sampleSoftnet() {
  try {
    const raw = fs.readFileSync('/proc/net/softnet_stat', 'utf8').trim().split('\n');
    let processed = 0;
    let dropped = 0;
    let timeSqueeze = 0;
    for (const line of raw) {
      const cols = line.trim().split(/\s+/);
      if (cols.length < 3) continue;
      processed += Number.parseInt(cols[0], 16) || 0;
      dropped += Number.parseInt(cols[1], 16) || 0;
      timeSqueeze += Number.parseInt(cols[2], 16) || 0;
    }
    return { processed, dropped, timeSqueeze };
  } catch (error) {
    return { error: error.message };
  }
}

function sampleProcCpu(pid) {
  const stat = fs.readFileSync('/proc/stat', 'utf8').split('\n')[0].trim().split(/\s+/).slice(1).map(v => Number.parseInt(v, 10) || 0);
  const total = stat.reduce((sum, v) => sum + v, 0);
  const procLine = fs.readFileSync(`/proc/${pid}/stat`, 'utf8').trim();
  const rparen = procLine.lastIndexOf(')');
  const rest = procLine.slice(rparen + 2).split(/\s+/);
  const utime = Number.parseInt(rest[11], 10) || 0;
  const stime = Number.parseInt(rest[12], 10) || 0;
  return { total, proc: utime + stime };
}

function cpuPercent(before, after) {
  const dt = after.total - before.total;
  const dp = after.proc - before.proc;
  if (dt <= 0) return 0;
  return (dp / dt) * 100 * (osAvailableCpus() || 1);
}

function sampleProcRssKb(pid) {
  const status = fs.readFileSync(`/proc/${pid}/status`, 'utf8').split('\n');
  for (const line of status) {
    if (line.startsWith('VmRSS:')) {
      const value = Number.parseInt(line.replace(/[^0-9]/g, ''), 10) || 0;
      return value;
    }
  }
  return 0;
}

function osAvailableCpus() {
  try {
    const txt = fs.readFileSync('/proc/cpuinfo', 'utf8');
    return (txt.match(/^processor\s*:/gm) || []).length || 1;
  } catch {
    return 1;
  }
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
  let burstIndex = 0;
  let stopped = false;
  const startAt = Date.now();
  const pump = () => {
    if (stopped) return;
    const elapsed = Date.now() - startAt;
    const shouldHaveSent = Math.floor(elapsed / burstIntervalMs) + 1;
    try {
      while (burstIndex < shouldHaveSent) {
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
        burstIndex += 1;
      }
    } catch (error) {
      console.error(`[${roomId}] sender error: ${error.message}`);
    }
    timer = setTimeout(pump, 10);
  };
  let timer = setTimeout(pump, 0);

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
      stopped = true;
      clearTimeout(timer);
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
  console.log(`http=${opts.httpUrl}`);
  console.log(`container=${opts.container}`);
  console.log(`sampleHost=${opts.sampleHost || 'local'}`);
  console.log(`config rooms=unbounded step=${opts.step} roundMs=${opts.roundMs} steadyRoundMs=${opts.steadyRoundMs ?? opts.roundMs} holdAfterMax=${opts.holdAfterMax} steadyRounds=${opts.steadyRounds}`);

  const rooms = [];
  let nextRoomIndex = 1;
  let peakHealthyRooms = 0;
  const selfPid = process.pid;
  let stopRequested = false;
  process.on('SIGINT', () => { stopRequested = true; });
  process.on('SIGTERM', () => { stopRequested = true; });

  const addRooms = async count => {
    for (let i = 0; i < count; ++i) {
      const room = await createRoom(nextRoomIndex++, opts, wsHost, wsPort);
      rooms.push(room);
    }
  };

  const logSample = async (phaseLabel, allowFailures = false) => {
    const [health, nodeLoad] = await Promise.all([
      sampleHttpJson(opts.httpUrl, '/healthz'),
      sampleHttpJson(opts.httpUrl, '/api/node-load'),
    ]);
    const selfBefore = sampleProcCpu(selfPid);
    const selfRssKb = sampleProcRssKb(selfPid);
    const procBefore = sampleContainerProcesses(opts.container, opts.sampleHost);
    const load = sampleLoadAvg();
    const softnet = sampleSoftnet();
    const statsRoom = rooms[0] || null;
    const rtpStatsBefore = statsRoom ? await sampleRtpStats(statsRoom) : null;
    const sample = await sampleRooms(rooms, opts.roundMs, opts.recvRatio);
    const rtpStatsAfter = statsRoom ? await sampleRtpStats(statsRoom) : null;
    const selfAfter = sampleProcCpu(selfPid);
    const procAfter = sampleContainerProcesses(opts.container, opts.sampleHost);
    peakHealthyRooms = Math.max(peakHealthyRooms, sample.healthyRooms);

    const workerBefore = procBefore['mediasoup-worker'];
    const workerAfter = procAfter['mediasoup-worker'];
    const sfuBefore = procBefore['mediasoup-sfu'];
    const sfuAfter = procAfter['mediasoup-sfu'];
    const workerCpu = workerBefore && workerAfter ? cpuPercent(workerBefore, workerAfter) : null;
    const sfuCpu = sfuBefore && sfuAfter ? cpuPercent(sfuBefore, sfuAfter) : null;
    const selfCpu = cpuPercent(selfBefore, selfAfter);
    console.log(
      `${phaseLabel} ` +
      `[rooms=${sample.totalRooms}] ` +
      `healthy=${sample.healthyRooms}/${sample.totalRooms} ` +
      `send=${sample.sendPps.toFixed(0)} recv1=${sample.recv1Pps.toFixed(0)} recv2=${sample.recv2Pps.toFixed(0)} ` +
      `ratio=${sample.recvRatio.toFixed(2)} ` +
      `sfuCpu=${sfuCpu !== null ? sfuCpu.toFixed(1) : 'n/a'} sfuRss=${sfuAfter ? sfuAfter.rssMb.toFixed(1) : 'n/a'}MB ` +
      `workerCpu=${workerCpu !== null ? workerCpu.toFixed(1) : 'n/a'} workerRss=${workerAfter ? workerAfter.rssMb.toFixed(1) : 'n/a'}MB ` +
      `selfCpu=${selfCpu.toFixed(1)} selfRss=${(selfRssKb / 1024).toFixed(1)}MB ` +
      `load1=${load.one?.toFixed?.(2) ?? 'n/a'} softnetDrop=${softnet.dropped ?? 'n/a'} softnetSqueeze=${softnet.timeSqueeze ?? 'n/a'} ` +
      (rtpStatsBefore && rtpStatsAfter
        ? `rtp(pub nack=${rtpStatsAfter.pub.nackCount - rtpStatsBefore.pub.nackCount} ` +
          `lost=${rtpStatsAfter.pub.packetsLost - rtpStatsBefore.pub.packetsLost} ` +
          `reTx=${rtpStatsAfter.pub.packetsRetransmitted - rtpStatsBefore.pub.packetsRetransmitted} ` +
          `sub1Loss=${rtpStatsAfter.sub1.rtpPacketLossReceived - rtpStatsBefore.sub1.rtpPacketLossReceived} ` +
          `sub2Loss=${rtpStatsAfter.sub2.rtpPacketLossReceived - rtpStatsBefore.sub2.rtpPacketLossReceived}) `
        : '') +
      `health=${health.status} ready=${health.json.ready} ` +
      `nodeRooms=${nodeLoad.json.rooms} availThreads=${nodeLoad.json.availableWorkerThreads} ` +
      `queueDepth=${JSON.stringify(nodeLoad.json.workerQueueStats)}`
    );

    const failures = [];
    if (!health.json.ok || !health.json.ready) {
      failures.push(`service degraded: /healthz=${health.status} /readyz=${health.json.ready}`);
    }
    if (procBefore.error || procAfter.error) {
      failures.push(`process sample failed: ${(procBefore.error || procAfter.error)}`);
    }
    if (!sample.healthy) {
      const bad = sample.details.filter(d => !d.healthy).slice(0, 5);
      for (const item of bad) console.log(`  fail ${item.roomId}: ${item.reasons.join(', ')}`);
      failures.push('traffic health check failed');
    }
    if (failures.length > 0 && !opts.continueOnFailure && !allowFailures) {
      throw new Error(failures[0]);
    }
    return failures;
  };

  try {
    let rampFrozen = false;
    while (rooms.length < opts.maxRooms) {
      const addCount = Math.min(opts.step, opts.maxRooms - rooms.length);
      await addRooms(addCount);
      const failures = await logSample('ramp', opts.freezeOnFailure);
      if (failures.length > 0) {
        console.log(`  continuing after failure: ${failures.join('; ')}`);
        if (opts.freezeOnFailure) {
          rampFrozen = true;
          console.log(`  freezing ramp at ${rooms.length} rooms after first failure`);
          break;
        }
      }
    }

    if (opts.holdAfterMax || rampFrozen) {
      let steadyRound = 0;
      const steadyRoundMs = opts.steadyRoundMs ?? opts.roundMs;
      while (!stopRequested && (opts.steadyRounds === 0 || steadyRound < opts.steadyRounds)) {
        steadyRound += 1;
        const failures = await logSample(`steady#${steadyRound}`, true);
        if (failures.length > 0) {
          console.log(`  continuing after failure: ${failures.join('; ')}`);
        }
        if (stopRequested || (opts.steadyRounds !== 0 && steadyRound >= opts.steadyRounds)) {
          break;
        }
        await sleep(steadyRoundMs);
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
