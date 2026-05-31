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
    serviceNetInterface: '',
    plainAutoReturn: false,
    postStopSamples: 0,
    postStopIntervalMs: 10000,
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
      case 'service-net-interface': opts.serviceNetInterface = rawValue || opts.serviceNetInterface; break;
      case 'plain-auto-return': opts.plainAutoReturn = true; break;
      case 'post-stop-samples': opts.postStopSamples = Math.max(0, int(rawValue)); break;
      case 'post-stop-interval-ms': opts.postStopIntervalMs = Math.max(1, int(rawValue)); break;
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

function createUdpReceiver(bindIp = '127.0.0.1') {
  const sock = dgram.createSocket('udp4');
  let packets = 0;
  sock.on('message', () => { packets += 1; });
  return new Promise((resolve, reject) => {
    sock.once('error', reject);
    sock.bind(0, bindIp, () => {
      const port = sock.address().port;
      sock.off('error', reject);
      resolve({ sock, port, get packets() { return packets; } });
    });
  });
}

function createUdpSender(bindIp = '127.0.0.1') {
  const sock = dgram.createSocket('udp4');
  let sendErrors = 0;
  return new Promise((resolve, reject) => {
    sock.once('error', reject);
    sock.bind(0, bindIp, () => {
      const port = sock.address().port;
      sock.off('error', reject);
      resolve({
        sock,
        port,
        get sendErrors() { return sendErrors; },
        close() {
          try { sock.close(); } catch {}
        },
        send(buf, port, host) {
          sock.send(buf, port, host, error => {
            if (error) sendErrors += 1;
          });
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
  if (sampleHost && sampleHost !== 'local') {
    return execFileSync('ssh', ['-o', 'BatchMode=yes', sampleHost, command], { encoding: 'utf8' });
  }
  return execFileSync('sh', ['-lc', command], { encoding: 'utf8' });
}

function sampleContainerProcesses(container, sampleHost = '') {
  try {
    const script = `
      sample_pid() {
        pid="$1"
        name="$2"
        if [ -r "/proc/$pid/stat" ] && [ -r "/proc/$pid/status" ]; then
          stat=$(cat "/proc/$pid/stat")
          rss=$(grep '^VmRSS:' "/proc/$pid/status" | tr -s ' ' | cut -d ' ' -f2)
          rss=\${rss:-0}
          pcpu=$(ps -p "$pid" -o pcpu= 2>/dev/null | tr -d ' ')
          pcpu=\${pcpu:-0}
          echo "$name|$stat|$rss|$pcpu"
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
      const stat = parts[1] || '';
      const rssKb = Number.parseInt(parts[2], 10) || 0;
      const cpu = Number.parseFloat(parts[3] || '0');
      if (name === 'mediasoup-sfu' || name === 'mediasoup-worker') {
        const rparen = stat.lastIndexOf(')');
        const rest = rparen !== -1 ? stat.slice(rparen + 2).split(/\s+/) : [];
        const utime = Number.parseInt(rest[11], 10) || 0;
        const stime = Number.parseInt(rest[12], 10) || 0;
        result[name] = {
          proc: utime + stime,
          rssMb: rssKb / 1024,
          cpuPercent: Number.isFinite(cpu) ? cpu : 0,
        };
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

function sampleRemoteHostStats(sampleHost = '', iface = '') {
  try {
    const script = `
      read l1 l5 l15 _ < /proc/loadavg
      echo "load|$l1|$l5|$l15"
      dropped=$(awk '{sum += strtonum("0x"$2)} END {print sum+0}' /proc/net/softnet_stat 2>/dev/null)
      squeezed=$(awk '{sum += strtonum("0x"$3)} END {print sum+0}' /proc/net/softnet_stat 2>/dev/null)
      echo "softnet|$dropped|$squeezed"
      iface="${iface}"
      if [ -z "$iface" ]; then
        iface=$(ip route get 1.1.1.1 2>/dev/null | awk '/dev/ {for(i=1;i<=NF;i++) if($i=="dev"){print $(i+1); exit}}')
      fi
      if [ -n "$iface" ] && [ -d "/sys/class/net/$iface/statistics" ]; then
        rx_bytes=$(cat /sys/class/net/$iface/statistics/rx_bytes 2>/dev/null || echo 0)
        tx_bytes=$(cat /sys/class/net/$iface/statistics/tx_bytes 2>/dev/null || echo 0)
        rx_packets=$(cat /sys/class/net/$iface/statistics/rx_packets 2>/dev/null || echo 0)
        tx_packets=$(cat /sys/class/net/$iface/statistics/tx_packets 2>/dev/null || echo 0)
        rx_dropped=$(cat /sys/class/net/$iface/statistics/rx_dropped 2>/dev/null || echo 0)
        tx_dropped=$(cat /sys/class/net/$iface/statistics/tx_dropped 2>/dev/null || echo 0)
        echo "iface|$iface|$rx_bytes|$tx_bytes|$rx_packets|$tx_packets|$rx_dropped|$tx_dropped"
      fi
      ss -u -i -n "( dport = :9000 or sport = :9000 )" 2>/dev/null | awk 'NR<=12 {print "ss|" $0}'
    `;
    const output = execRemoteCommand(sampleHost, `sh -lc ${shellQuote(script)}`).trim();
    const result = {
      load: null,
      softnet: null,
      iface: null,
      ss: [],
    };
    for (const row of output ? output.split('\n') : []) {
      const parts = row.split('|');
      switch (parts[0]) {
        case 'load':
          result.load = {
            one: Number.parseFloat(parts[1] || '0'),
            five: Number.parseFloat(parts[2] || '0'),
            fifteen: Number.parseFloat(parts[3] || '0'),
          };
          break;
        case 'softnet':
          result.softnet = {
            dropped: Number.parseInt(parts[1] || '0', 10) || 0,
            timeSqueeze: Number.parseInt(parts[2] || '0', 10) || 0,
          };
          break;
        case 'iface':
          result.iface = {
            name: parts[1] || '',
            rxBytes: Number.parseInt(parts[2] || '0', 10) || 0,
            txBytes: Number.parseInt(parts[3] || '0', 10) || 0,
            rxPackets: Number.parseInt(parts[4] || '0', 10) || 0,
            txPackets: Number.parseInt(parts[5] || '0', 10) || 0,
            rxDropped: Number.parseInt(parts[6] || '0', 10) || 0,
            txDropped: Number.parseInt(parts[7] || '0', 10) || 0,
          };
          break;
        case 'ss':
          result.ss.push(parts.slice(1).join('|'));
          break;
      }
    }
    return result;
  } catch (error) {
    return { error: error.message };
  }
}

function ratePerSec(before, after, sampleMs) {
  if (!Number.isFinite(before) || !Number.isFinite(after) || sampleMs <= 0) return null;
  return (after - before) / (sampleMs / 1000);
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

function sampleLocalProcessCpuPercent(pid) {
  try {
    const output = execFileSync('sh', ['-lc', `ps -p ${pid} -o pcpu= | tr -d ' '`], { encoding: 'utf8' }).trim();
    const value = Number.parseFloat(output);
    return Number.isFinite(value) ? value : 0;
  } catch {
    return 0;
  }
}

function sampleRemoteContainerStats(container, sampleHost = '') {
  try {
    const output = execRemoteCommand(
      sampleHost,
      `docker stats --no-stream --format {{.CPUPerc}}\\|{{.MemUsage}} ${shellQuote(container)}`
    ).trim();
    const [cpuText = '', memText = ''] = output.split('|');
    const cpuPercent = Number.parseFloat(cpuText.replace('%', '').trim());
    const memMatch = memText.match(/([0-9.]+)([KMG]iB)\s*\/\s*([0-9.]+)([KMG]iB)/);
    const toMiB = (value, unit) => {
      const numeric = Number.parseFloat(value);
      if (!Number.isFinite(numeric)) return null;
      if (unit === 'KiB') return numeric / 1024;
      if (unit === 'MiB') return numeric;
      if (unit === 'GiB') return numeric * 1024;
      return null;
    };
    return {
      cpuPercent: Number.isFinite(cpuPercent) ? cpuPercent : null,
      memUsageMiB: memMatch ? toMiB(memMatch[1], memMatch[2]) : null,
      memLimitMiB: memMatch ? toMiB(memMatch[3], memMatch[4]) : null,
    };
  } catch (error) {
    return { error: error.message };
  }
}

function buildAutoReturnConnectPacket() {
  // RTCP APP packet: V=2, subtype=1 ("connect"), PT=204, length=2 (12 bytes total), SSRC=0, name="CNCT".
  const packet = Buffer.alloc(12);
  packet[0] = 0x80 | 0x01;
  packet[1] = 204;
  packet.writeUInt16BE(2, 2);
  packet.writeUInt32BE(0, 4);
  packet.write('CNCT', 8, 'ascii');
  return packet;
}

async function sendAutoReturnConnectProbes(sock, host, port, attempts = 3, intervalMs = 25) {
  const packet = buildAutoReturnConnectPacket();
  for (let i = 0; i < attempts; ++i) {
    await new Promise((resolve, reject) => {
      sock.send(packet, port, host, error => error ? reject(error) : resolve());
    });
    if (i + 1 < attempts) await sleep(intervalMs);
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

  const udpBindIp = opts.plainAutoReturn ? '0.0.0.0' : '127.0.0.1';
  const sender = await createUdpSender(udpBindIp);

  const plainPublishReq = {
    videoSsrc: 90000001 + roomIndex,
    videoSsrcs: [90000001 + roomIndex],
    videoCodec: 'vp8',
    enableAudio: false,
  };
  if (opts.explicitConnect && !opts.plainAutoReturn) {
    plainPublishReq.senderIp = '127.0.0.1';
    plainPublishReq.senderPort = sender.port;
  }
  const pubResp = await pubWs.request('plainPublish', plainPublishReq);
  if (!pubResp.ok) throw new Error(`plainPublish failed for ${roomId}: ${JSON.stringify(pubResp)}`);
  console.log(`[${roomId}] plainPublish senderIp=${plainPublishReq.senderIp || 'comedia'} senderPort=${plainPublishReq.senderPort || 'auto'} serverPort=${pubResp.data.port} localTuple=${pubResp.data.ip}:${pubResp.data.port}`);

  const recv1 = await createUdpReceiver(udpBindIp);
  const recv2 = await createUdpReceiver(udpBindIp);

  const sub1Req = opts.plainAutoReturn
    ? { autoReturn: true }
    : { recvIp: '127.0.0.1', recvPort: recv1.port };
  const sub1Resp = await sub1Ws.request('plainSubscribe', sub1Req);
  if (!sub1Resp.ok) throw new Error(`plainSubscribe sub1 failed for ${roomId}: ${JSON.stringify(sub1Resp)}`);
  console.log(`[${roomId}] plainSubscribe sub1 ${opts.plainAutoReturn ? 'autoReturn=true' : `recvIp=127.0.0.1 recvPort=${recv1.port}`} localTuple=${sub1Resp.data.ip}:${sub1Resp.data.port}`);

  const sub2Req = opts.plainAutoReturn
    ? { autoReturn: true }
    : { recvIp: '127.0.0.1', recvPort: recv2.port };
  const sub2Resp = await sub2Ws.request('plainSubscribe', sub2Req);
  if (!sub2Resp.ok) throw new Error(`plainSubscribe sub2 failed for ${roomId}: ${JSON.stringify(sub2Resp)}`);
  console.log(`[${roomId}] plainSubscribe sub2 ${opts.plainAutoReturn ? 'autoReturn=true' : `recvIp=127.0.0.1 recvPort=${recv2.port}`} localTuple=${sub2Resp.data.ip}:${sub2Resp.data.port}`);

  const serverHost = wsHost;
  const serverPort = pubResp.data.port;
  console.log(`[${roomId}] rtpSend target=${serverHost}:${serverPort}`);
  if (opts.plainAutoReturn) {
    await sendAutoReturnConnectProbes(recv1.sock, wsHost, sub1Resp.data.port);
    await sendAutoReturnConnectProbes(recv2.sock, wsHost, sub2Resp.data.port);
  }
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
    getSendErrors() { return sender.sendErrors; },
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
    sendErrors: room.getSendErrors(),
    recv1: room.getRecv1(),
    recv2: room.getRecv2(),
  }));
  await sleep(sampleMs);
  const after = rooms.map(room => ({
    send: room.getSend(),
    sendErrors: room.getSendErrors(),
    recv1: room.getRecv1(),
    recv2: room.getRecv2(),
  }));

  const details = rooms.map((room, index) => {
    const sendDelta = after[index].send - before[index].send;
    const sendErrorDelta = after[index].sendErrors - before[index].sendErrors;
    const recv1Delta = after[index].recv1 - before[index].recv1;
    const recv2Delta = after[index].recv2 - before[index].recv2;
    const recvOk1 = sendDelta === 0 || recv1Delta >= sendDelta * recvRatio;
    const recvOk2 = sendDelta === 0 || recv2Delta >= sendDelta * recvRatio;
    return {
      roomId: room.roomId,
      sendDelta,
      sendErrorDelta,
      recv1Delta,
      recv2Delta,
      healthy: sendDelta > 0 && sendErrorDelta === 0 && recvOk1 && recvOk2,
      reasons: [
        ...(sendDelta <= 0 ? ['send=0'] : []),
        ...(sendErrorDelta > 0 ? [`sendErrors=${sendErrorDelta}`] : []),
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
  console.log(`serviceNetInterface=${opts.serviceNetInterface || 'auto'}`);
  console.log(`plainAutoReturn=${opts.plainAutoReturn}`);
  console.log(`config rooms=unbounded step=${opts.step} roundMs=${opts.roundMs} steadyRoundMs=${opts.steadyRoundMs ?? opts.roundMs} holdAfterMax=${opts.holdAfterMax} steadyRounds=${opts.steadyRounds}`);

  const rooms = [];
  let nextRoomIndex = 1;
  let peakHealthyRooms = 0;
  const selfPid = process.pid;
  let stopRequested = false;
  const rssSummary = {
    worker: { start: null, min: Number.POSITIVE_INFINITY, max: 0, last: null },
    sfu: { start: null, min: Number.POSITIVE_INFINITY, max: 0, last: null },
    container: { start: null, min: Number.POSITIVE_INFINITY, max: 0, last: null },
    loadGen: { start: null, min: Number.POSITIVE_INFINITY, max: 0, last: null },
  };
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
    const selfRssKb = sampleProcRssKb(selfPid);
    const procBefore = sampleContainerProcesses(opts.container, opts.sampleHost);
    const containerBefore = sampleRemoteContainerStats(opts.container, opts.sampleHost);
    const load = sampleLoadAvg();
    const softnet = sampleSoftnet();
    const serviceHostBefore = sampleRemoteHostStats(opts.sampleHost, opts.serviceNetInterface);
    const statsRoom = rooms[0] || null;
    const rtpStatsBefore = statsRoom ? await sampleRtpStats(statsRoom) : null;
    const sample = await sampleRooms(rooms, opts.roundMs, opts.recvRatio);
    const rtpStatsAfter = statsRoom ? await sampleRtpStats(statsRoom) : null;
    const procAfter = sampleContainerProcesses(opts.container, opts.sampleHost);
    const containerAfter = sampleRemoteContainerStats(opts.container, opts.sampleHost);
    const serviceHostAfter = sampleRemoteHostStats(opts.sampleHost, opts.serviceNetInterface);
    peakHealthyRooms = Math.max(peakHealthyRooms, sample.healthyRooms);

    const workerBefore = procBefore['mediasoup-worker'];
    const workerAfter = procAfter['mediasoup-worker'];
    const sfuBefore = procBefore['mediasoup-sfu'];
    const sfuAfter = procAfter['mediasoup-sfu'];
    const workerCpu = workerAfter?.cpuPercent ?? workerBefore?.cpuPercent ?? null;
    const sfuCpu = sfuAfter?.cpuPercent ?? sfuBefore?.cpuPercent ?? null;
    const selfCpu = sampleLocalProcessCpuPercent(selfPid);
    const containerCpu = containerAfter.cpuPercent ?? containerBefore.cpuPercent ?? null;
    const containerMem = containerAfter.memUsageMiB ?? containerBefore.memUsageMiB ?? null;
    const serviceLoad = serviceHostAfter.load || serviceHostBefore.load;
    const serviceSoftnet = serviceHostAfter.softnet || serviceHostBefore.softnet;
    const serviceIface = serviceHostAfter.iface || serviceHostBefore.iface;
    const serviceIfaceBefore = serviceHostBefore.iface;
    const serviceIfaceAfter = serviceHostAfter.iface;
    const serviceRxMbps = serviceIfaceBefore && serviceIfaceAfter
      ? ratePerSec(serviceIfaceBefore.rxBytes, serviceIfaceAfter.rxBytes, opts.roundMs) * 8 / 1_000_000
      : null;
    const serviceTxMbps = serviceIfaceBefore && serviceIfaceAfter
      ? ratePerSec(serviceIfaceBefore.txBytes, serviceIfaceAfter.txBytes, opts.roundMs) * 8 / 1_000_000
      : null;
    const updateSummary = (bucket, value) => {
      if (!Number.isFinite(value)) return;
      if (!Number.isFinite(bucket.start)) bucket.start = value;
      bucket.min = Math.min(bucket.min, value);
      bucket.max = Math.max(bucket.max, value);
      bucket.last = value;
    };
    updateSummary(rssSummary.worker, workerAfter?.rssMb ?? null);
    updateSummary(rssSummary.sfu, sfuAfter?.rssMb ?? null);
    updateSummary(rssSummary.container, containerMem);
    updateSummary(rssSummary.loadGen, selfRssKb / 1024);

    console.log(
      `${phaseLabel} ` +
      `[rooms=${sample.totalRooms}] ` +
      `healthy=${sample.healthyRooms}/${sample.totalRooms} ` +
      `send=${sample.sendPps.toFixed(0)} recv1=${sample.recv1Pps.toFixed(0)} recv2=${sample.recv2Pps.toFixed(0)} ` +
      `ratio=${sample.recvRatio.toFixed(2)} ` +
      `sfuCpu=${sfuCpu !== null ? sfuCpu.toFixed(1) : 'n/a'} sfuRss=${sfuAfter ? sfuAfter.rssMb.toFixed(1) : 'n/a'}MB ` +
      `workerCpu=${workerCpu !== null ? workerCpu.toFixed(1) : 'n/a'} workerRss=${workerAfter ? workerAfter.rssMb.toFixed(1) : 'n/a'}MB ` +
      `containerCpu=${containerCpu !== null ? containerCpu.toFixed(1) : 'n/a'} containerMem=${containerMem !== null ? containerMem.toFixed(1) : 'n/a'}MB ` +
      `loadGenCpu=${selfCpu.toFixed(1)} loadGenRss=${(selfRssKb / 1024).toFixed(1)}MB ` +
      `loadGenLoad1=${load.one?.toFixed?.(2) ?? 'n/a'} loadGenSoftnetDrop=${softnet.dropped ?? 'n/a'} loadGenSoftnetSqueeze=${softnet.timeSqueeze ?? 'n/a'} ` +
      `serviceLoad1=${serviceLoad?.one?.toFixed?.(2) ?? 'n/a'} serviceSoftnetDrop=${serviceSoftnet?.dropped ?? 'n/a'} serviceSoftnetSqueeze=${serviceSoftnet?.timeSqueeze ?? 'n/a'} ` +
      `serviceIface=${serviceIface?.name || 'n/a'} serviceRxMbps=${serviceRxMbps !== null ? serviceRxMbps.toFixed(1) : 'n/a'} serviceTxMbps=${serviceTxMbps !== null ? serviceTxMbps.toFixed(1) : 'n/a'} serviceRxDrop=${serviceIface?.rxDropped ?? 'n/a'} serviceTxDrop=${serviceIface?.txDropped ?? 'n/a'} ` +
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
    if (serviceHostBefore.error || serviceHostAfter.error) {
      failures.push(`service host sample failed: ${(serviceHostBefore.error || serviceHostAfter.error)}`);
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
    const postProc = sampleContainerProcesses(opts.container, opts.sampleHost);
    const postContainer = sampleRemoteContainerStats(opts.container, opts.sampleHost);
    const postLoadGenRss = sampleProcRssKb(selfPid) / 1024;
    const recovery = [];
    for (let i = 0; i < opts.postStopSamples; ++i) {
      if (i > 0) await sleep(opts.postStopIntervalMs);
      const proc = sampleContainerProcesses(opts.container, opts.sampleHost);
      const container = sampleRemoteContainerStats(opts.container, opts.sampleHost);
      const loadGen = sampleProcRssKb(selfPid) / 1024;
      recovery.push({
        worker: proc['mediasoup-worker']?.rssMb ?? null,
        sfu: proc['mediasoup-sfu']?.rssMb ?? null,
        container: container.memUsageMiB ?? null,
        loadGen,
      });
      console.log(
        `recovery#${i + 1} ` +
        `worker=${recovery[i].worker !== null ? recovery[i].worker.toFixed(1) : 'n/a'}MB ` +
        `sfu=${recovery[i].sfu !== null ? recovery[i].sfu.toFixed(1) : 'n/a'}MB ` +
        `container=${recovery[i].container !== null ? recovery[i].container.toFixed(1) : 'n/a'}MB ` +
        `loadGen=${loadGen.toFixed(1)}MB`
      );
    }
    const latestRecovery = recovery.length > 0 ? recovery[recovery.length - 1] : null;
    console.log(
      `rssSummary ` +
      `workerStart=${Number.isFinite(rssSummary.worker.start) ? rssSummary.worker.start.toFixed(1) : 'n/a'}MB ` +
      `workerMin=${Number.isFinite(rssSummary.worker.min) ? rssSummary.worker.min.toFixed(1) : 'n/a'}MB ` +
      `workerPeak=${Number.isFinite(rssSummary.worker.max) ? rssSummary.worker.max.toFixed(1) : 'n/a'}MB ` +
      `workerFinal=${Number.isFinite(rssSummary.worker.last) ? rssSummary.worker.last.toFixed(1) : 'n/a'}MB ` +
      `postWorker=${postProc['mediasoup-worker'] ? postProc['mediasoup-worker'].rssMb.toFixed(1) : 'n/a'}MB ` +
      `workerRecovery=${latestRecovery?.worker !== null && latestRecovery?.worker !== undefined ? latestRecovery.worker.toFixed(1) : 'n/a'}MB ` +
      `sfuStart=${Number.isFinite(rssSummary.sfu.start) ? rssSummary.sfu.start.toFixed(1) : 'n/a'}MB ` +
      `sfuMin=${Number.isFinite(rssSummary.sfu.min) ? rssSummary.sfu.min.toFixed(1) : 'n/a'}MB ` +
      `sfuPeak=${Number.isFinite(rssSummary.sfu.max) ? rssSummary.sfu.max.toFixed(1) : 'n/a'}MB ` +
      `sfuFinal=${Number.isFinite(rssSummary.sfu.last) ? rssSummary.sfu.last.toFixed(1) : 'n/a'}MB ` +
      `postSfu=${postProc['mediasoup-sfu'] ? postProc['mediasoup-sfu'].rssMb.toFixed(1) : 'n/a'}MB ` +
      `sfuRecovery=${latestRecovery?.sfu !== null && latestRecovery?.sfu !== undefined ? latestRecovery.sfu.toFixed(1) : 'n/a'}MB ` +
      `containerStart=${Number.isFinite(rssSummary.container.start) ? rssSummary.container.start.toFixed(1) : 'n/a'}MB ` +
      `containerMin=${Number.isFinite(rssSummary.container.min) ? rssSummary.container.min.toFixed(1) : 'n/a'}MB ` +
      `containerPeak=${Number.isFinite(rssSummary.container.max) ? rssSummary.container.max.toFixed(1) : 'n/a'}MB ` +
      `containerFinal=${Number.isFinite(rssSummary.container.last) ? rssSummary.container.last.toFixed(1) : 'n/a'}MB ` +
      `postContainer=${postContainer.memUsageMiB !== null && postContainer.memUsageMiB !== undefined ? postContainer.memUsageMiB.toFixed(1) : 'n/a'}MB ` +
      `containerRecovery=${latestRecovery?.container !== null && latestRecovery?.container !== undefined ? latestRecovery.container.toFixed(1) : 'n/a'}MB ` +
      `loadGenStart=${Number.isFinite(rssSummary.loadGen.start) ? rssSummary.loadGen.start.toFixed(1) : 'n/a'}MB ` +
      `loadGenMin=${Number.isFinite(rssSummary.loadGen.min) ? rssSummary.loadGen.min.toFixed(1) : 'n/a'}MB ` +
      `loadGenPeak=${Number.isFinite(rssSummary.loadGen.max) ? rssSummary.loadGen.max.toFixed(1) : 'n/a'}MB ` +
      `loadGenFinal=${Number.isFinite(rssSummary.loadGen.last) ? rssSummary.loadGen.last.toFixed(1) : 'n/a'}MB ` +
      `postLoadGen=${postLoadGenRss.toFixed(1)}MB ` +
      `loadGenRecovery=${latestRecovery?.loadGen !== null && latestRecovery?.loadGen !== undefined ? latestRecovery.loadGen.toFixed(1) : 'n/a'}MB`
    );
  }
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
