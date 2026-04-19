import crypto from 'node:crypto';
import net from 'node:net';

export class WsJsonClient {
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
    if (!this.socket) return;
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
    const header = [0x81];
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
        if (this.pending.length < 4) return;
        payloadLength = this.pending.readUInt16BE(2);
        offset = 4;
      } else if (payloadLength === 127) {
        throw new Error('unsupported long websocket frame');
      }

      if (masked) {
        throw new Error('server frames must not be masked');
      }

      if (this.pending.length < offset + payloadLength) return;

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

export function defaultRtpCapabilities() {
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

export async function joinRoom(ws, roomId, peerId) {
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

  return { resp, qosPolicy };
}
