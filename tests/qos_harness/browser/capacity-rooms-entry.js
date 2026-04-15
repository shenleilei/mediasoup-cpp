import { Device } from '../../../src/client/lib/Device.js';

const RTP_CAPS = {
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

const DEFAULT_CONFIG = {
  roomPrefix: 'capacity_room',
  width: 1920,
  height: 1080,
  fps: 30,
  targetBitrateBps: 6_000_000,
  minSendBitrateBps: 1_500_000,
  minRecvBitrateBps: 1_500_000,
  minWidth: 1600,
  minHeight: 900,
  createConcurrency: 4,
  connectTimeoutMs: 15_000,
  videoTimeoutMs: 15_000,
};

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function asMediaKind(stat) {
  return stat?.kind || stat?.mediaType || null;
}

function statsReportToArray(report) {
  if (!report) {
    return [];
  }
  if (typeof report.values === 'function') {
    return Array.from(report.values());
  }
  if (Array.isArray(report)) {
    return report;
  }
  return Object.values(report);
}

function findStats(report, predicate) {
  return statsReportToArray(report).find(predicate) || null;
}

function bitrateFromDelta(bytesDelta, sampleMs) {
  if (!Number.isFinite(bytesDelta) || bytesDelta <= 0 || sampleMs <= 0) {
    return 0;
  }
  return Math.round((bytesDelta * 8 * 1000) / sampleMs);
}

function positiveDelta(after, before) {
  if (!Number.isFinite(after) || !Number.isFinite(before)) {
    return 0;
  }
  return Math.max(0, after - before);
}

function average(numbers) {
  if (!numbers.length) {
    return 0;
  }
  return Math.round(numbers.reduce((sum, value) => sum + value, 0) / numbers.length);
}

function minimum(numbers) {
  if (!numbers.length) {
    return 0;
  }
  return Math.min(...numbers);
}

function createHiddenVideo(track) {
  const video = document.createElement('video');
  video.autoplay = true;
  video.muted = true;
  video.playsInline = true;
  video.style.cssText = 'width:1px;height:1px;position:absolute;opacity:0;pointer-events:none;';
  video.srcObject = new MediaStream([track]);
  document.body.appendChild(video);
  video.play().catch(() => {});
  return video;
}

async function waitForTransportConnected(transport, timeoutMs) {
  if (transport.connectionState === 'connected' || transport.connectionState === 'completed') {
    return;
  }

  await new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      cleanup();
      reject(new Error(`transport connect timeout: ${transport.connectionState}`));
    }, timeoutMs);

    const onState = state => {
      if (state === 'connected' || state === 'completed') {
        cleanup();
        resolve();
      } else if (state === 'failed' || state === 'closed' || state === 'disconnected') {
        cleanup();
        reject(new Error(`transport failed: ${state}`));
      }
    };

    const cleanup = () => {
      clearTimeout(timeout);
      transport.off('connectionstatechange', onState);
    };

    transport.on('connectionstatechange', onState);
  });
}

async function waitForVideoReady(video, timeoutMs) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    if (video.readyState >= HTMLMediaElement.HAVE_CURRENT_DATA && video.videoWidth > 0 && video.videoHeight > 0) {
      return;
    }
    await sleep(100);
  }
  throw new Error('remote video did not become ready');
}

function createCanvasSource(label, { width, height, fps }) {
  const canvas = document.createElement('canvas');
  canvas.width = width;
  canvas.height = height;
  canvas.style.cssText = 'width:1px;height:1px;position:absolute;opacity:0;pointer-events:none;';
  document.body.appendChild(canvas);

  const ctx = canvas.getContext('2d');
  const cols = 32;
  const rows = 18;
  const cellWidth = canvas.width / cols;
  const cellHeight = canvas.height / rows;
  const frameIntervalMs = Math.max(8, Math.round(1000 / Math.max(1, fps)));
  let tick = 0;
  let timer = null;
  let stopped = false;

  const draw = () => {
    if (stopped) {
      return;
    }
    tick += 1;

    for (let row = 0; row < rows; row += 1) {
      for (let col = 0; col < cols; col += 1) {
        const seed =
          ((tick * 1103515245) ^
            (col * 73856093) ^
            (row * 19349663)) >>> 0;
        const red = seed & 0xff;
        const green = (seed >> 8) & 0xff;
        const blue = (seed >> 16) & 0xff;
        ctx.fillStyle = `rgb(${red}, ${green}, ${blue})`;
        ctx.fillRect(
          col * cellWidth,
          row * cellHeight,
          cellWidth + 1,
          cellHeight + 1
        );
      }
    }

    ctx.fillStyle = 'rgba(5, 10, 20, 0.35)';
    ctx.fillRect(0, canvas.height - 220, canvas.width, 220);
    ctx.fillStyle = '#f8fafc';
    ctx.font = 'bold 88px sans-serif';
    ctx.fillText(label, 64, canvas.height - 108);
    ctx.font = '48px sans-serif';
    ctx.fillText(`tick=${tick} ${width}x${height}@${fps}`, 64, canvas.height - 44);

    ctx.strokeStyle = 'rgba(255,255,255,0.6)';
    ctx.lineWidth = 5;
    for (let line = 0; line < 8; line += 1) {
      const offset = (tick * (line + 2) * 17) % canvas.width;
      ctx.beginPath();
      ctx.moveTo(offset, 0);
      ctx.lineTo(canvas.width - offset / 3, canvas.height);
      ctx.stroke();
    }

    ctx.fillStyle = '#22d3ee';
    ctx.fillRect((tick * 31) % (canvas.width - 260), 80, 260, 260);
    ctx.fillStyle = '#f97316';
    ctx.beginPath();
    ctx.arc(
      canvas.width - 220,
      180 + ((tick * 11) % 180),
      112,
      0,
      Math.PI * 2
    );
    ctx.fill();

    timer = setTimeout(draw, frameIntervalMs);
  };

  draw();

  const stream = canvas.captureStream(fps);
  const [track] = stream.getVideoTracks();
  track.contentHint = 'motion';

  return {
    canvas,
    stream,
    track,
    width,
    height,
    fps,
    stop() {
      stopped = true;
      if (timer) {
        clearTimeout(timer);
      }
      for (const item of stream.getTracks()) {
        try {
          item.stop();
        } catch {}
      }
      try {
        canvas.remove();
      } catch {}
    },
  };
}

class WsClient {
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
        const message = JSON.parse(event.data);
        if (message.response === true) {
          const callback = this.pending.get(message.id);
          if (callback) {
            this.pending.delete(message.id);
            callback(message);
          }
        } else if (message.notification === true) {
          this.notifications.push(message);
        }
      });
    });
  }

  async request(method, data = {}) {
    const id = this.nextId++;
    const response = new Promise(resolve => this.pending.set(id, resolve));
    this.ws.send(JSON.stringify({ request: true, id, method, data }));
    return response;
  }

  async waitNotification(method, timeoutMs = 5000) {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      const index = this.notifications.findIndex(item => item.method === method);
      if (index !== -1) {
        return this.notifications.splice(index, 1)[0];
      }
      await sleep(50);
    }
    return null;
  }

  close() {
    try {
      this.ws?.close();
    } catch {}
  }
}

async function joinPeer(wsUrl, roomId, peerId) {
  const ws = new WsClient(wsUrl);
  await ws.connect();
  const response = await ws.request('join', {
    roomId,
    peerId,
    displayName: peerId,
    rtpCapabilities: RTP_CAPS,
  });
  if (!response.ok) {
    throw new Error(`${peerId} join failed: ${JSON.stringify(response)}`);
  }

  const device = new Device();
  await device.load({ routerRtpCapabilities: response.data.routerRtpCapabilities });

  return {
    ws,
    device,
    peerId,
    roomId,
  };
}

async function createSendTransport(peer) {
  const response = await peer.ws.request('createWebRtcTransport', {
    producing: true,
    consuming: false,
  });
  if (!response.ok) {
    throw new Error(`${peer.peerId} send transport failed`);
  }

  const transport = peer.device.createSendTransport(response.data);
  transport.on('connect', async ({ dtlsParameters }, callback, errback) => {
    try {
      const connectResponse = await peer.ws.request('connectWebRtcTransport', {
        transportId: transport.id,
        dtlsParameters,
      });
      if (!connectResponse.ok) {
        throw new Error(connectResponse.error || 'connectWebRtcTransport failed');
      }
      callback();
    } catch (error) {
      errback(error);
    }
  });
  transport.on('produce', async ({ kind, rtpParameters }, callback, errback) => {
    try {
      const produceResponse = await peer.ws.request('produce', {
        transportId: transport.id,
        kind,
        rtpParameters,
      });
      if (!produceResponse.ok) {
        throw new Error(produceResponse.error || 'produce failed');
      }
      callback({ id: produceResponse.data.id });
    } catch (error) {
      errback(error);
    }
  });
  return transport;
}

async function createRecvTransport(peer) {
  const response = await peer.ws.request('createWebRtcTransport', {
    producing: false,
    consuming: true,
  });
  if (!response.ok) {
    throw new Error(`${peer.peerId} recv transport failed`);
  }

  const transport = peer.device.createRecvTransport(response.data);
  transport.on('connect', async ({ dtlsParameters }, callback, errback) => {
    try {
      const connectResponse = await peer.ws.request('connectWebRtcTransport', {
        transportId: transport.id,
        dtlsParameters,
      });
      if (!connectResponse.ok) {
        throw new Error(connectResponse.error || 'connectWebRtcTransport failed');
      }
      callback();
    } catch (error) {
      errback(error);
    }
  });
  return transport;
}

async function consumeSingleVideo(peer, transport, consumerData) {
  return transport.consume({
    id: consumerData.id,
    producerId: consumerData.producerId,
    kind: consumerData.kind,
    rtpParameters: consumerData.rtpParameters,
  });
}

function summarizeProducerStats(report, source) {
  const outbound = findStats(
    report,
    stat => stat.type === 'outbound-rtp' && !stat.isRemote && asMediaKind(stat) === 'video'
  );
  const track = findStats(
    report,
    stat => stat.type === 'track' && asMediaKind(stat) === 'video'
  );

  return {
    bytesSent: outbound?.bytesSent || 0,
    framesSent: outbound?.framesSent ?? track?.framesSent ?? 0,
    frameWidth: track?.frameWidth ?? source.width,
    frameHeight: track?.frameHeight ?? source.height,
    qualityLimitationReason: outbound?.qualityLimitationReason || 'none',
  };
}

function summarizeConsumerStats(report, video) {
  const inbound = findStats(
    report,
    stat => stat.type === 'inbound-rtp' && !stat.isRemote && asMediaKind(stat) === 'video'
  );
  const track = findStats(
    report,
    stat => stat.type === 'track' && asMediaKind(stat) === 'video'
  );

  return {
    bytesReceived: inbound?.bytesReceived || 0,
    framesDecoded: inbound?.framesDecoded ?? track?.framesDecoded ?? 0,
    packetsLost: inbound?.packetsLost || 0,
    jitter: inbound?.jitter || 0,
    frameWidth: track?.frameWidth ?? video?.videoWidth ?? 0,
    frameHeight: track?.frameHeight ?? video?.videoHeight ?? 0,
    framesPerSecond: track?.framesPerSecond ?? 0,
    freezeCount: track?.freezeCount ?? 0,
    totalFreezesDuration: track?.totalFreezesDuration ?? 0,
  };
}

async function captureRoomSnapshot(room) {
  const [producerReport, consumerReport] = await Promise.all([
    room.publisher.producer.getStats(),
    room.subscriber.consumer.getStats(),
  ]);

  return {
    sendTransportState: room.publisher.transport.connectionState,
    recvTransportState: room.subscriber.transport.connectionState,
    producer: summarizeProducerStats(producerReport, room.source),
    consumer: summarizeConsumerStats(consumerReport, room.subscriber.videoElement),
    videoWidth: room.subscriber.videoElement?.videoWidth ?? 0,
    videoHeight: room.subscriber.videoElement?.videoHeight ?? 0,
  };
}

async function closeRoomPair(room) {
  try {
    room.subscriber.videoElement?.pause?.();
  } catch {}
  try {
    room.subscriber.videoElement?.remove?.();
  } catch {}
  try {
    room.subscriber.consumer?.close?.();
  } catch {}
  try {
    room.publisher.producer?.close?.();
  } catch {}
  try {
    room.subscriber.transport?.close?.();
  } catch {}
  try {
    room.publisher.transport?.close?.();
  } catch {}
  room.subscriber.ws?.close?.();
  room.publisher.ws?.close?.();
  try {
    room.source?.stop?.();
  } catch {}
}

async function createRoomPair(wsUrl, index, config) {
  const roomId = `${config.roomPrefix}_${index}`;
  const room = {
    index,
    roomId,
    publisher: {},
    subscriber: {},
    source: null,
  };

  try {
    room.publisher = await joinPeer(wsUrl, roomId, 'pub');
    room.subscriber = await joinPeer(wsUrl, roomId, 'sub');

    room.subscriber.transport = await createRecvTransport(room.subscriber);
    room.publisher.transport = await createSendTransport(room.publisher);

    room.source = createCanvasSource(`room-${index}`, config);
    room.publisher.producer = await room.publisher.transport.produce({
      track: room.source.track,
      encodings: [
        {
          maxBitrate: config.targetBitrateBps,
          maxFramerate: config.fps,
          priority: 'high',
          networkPriority: 'high',
        },
      ],
      codecOptions: {
        videoGoogleStartBitrate: Math.max(300, Math.round(config.targetBitrateBps / 1000)),
      },
    });

    const notification = await room.subscriber.ws.waitNotification('newConsumer', config.connectTimeoutMs);
    if (!notification) {
      throw new Error(`[${roomId}] newConsumer timeout`);
    }

    room.subscriber.consumer = await consumeSingleVideo(
      room.subscriber,
      room.subscriber.transport,
      notification.data
    );
    room.subscriber.videoElement = createHiddenVideo(room.subscriber.consumer.track);

    await Promise.all([
      waitForTransportConnected(room.publisher.transport, config.connectTimeoutMs),
      waitForTransportConnected(room.subscriber.transport, config.connectTimeoutMs),
      waitForVideoReady(room.subscriber.videoElement, config.videoTimeoutMs),
    ]);

    return room;
  } catch (error) {
    await closeRoomPair(room);
    throw error;
  }
}

async function runWithConcurrency(total, concurrency, iterator) {
  const workers = Math.max(1, Math.min(concurrency, total));
  const results = new Array(total);
  let cursor = 0;

  async function worker() {
    while (cursor < total) {
      const current = cursor;
      cursor += 1;
      results[current] = await iterator(current);
    }
  }

  await Promise.all(Array.from({ length: workers }, () => worker()));
  return results;
}

window.__capacityRoomsHarness = {
  wsUrl: '',
  config: { ...DEFAULT_CONFIG },
  rooms: [],
  nextRoomIndex: 1,

  async init(wsUrl, config = {}) {
    this.wsUrl = wsUrl;
    this.config = {
      ...DEFAULT_CONFIG,
      ...config,
    };
    this.rooms = [];
    this.nextRoomIndex = 1;
    return {
      wsUrl: this.wsUrl,
      config: this.config,
      totalRooms: this.rooms.length,
    };
  },

  async addRooms(count) {
    const startIndex = this.nextRoomIndex;
    const created = await runWithConcurrency(
      count,
      this.config.createConcurrency,
      async offset => createRoomPair(this.wsUrl, startIndex + offset, this.config)
    );

    this.rooms.push(...created);
    this.nextRoomIndex += count;

    return {
      added: created.map(room => room.roomId),
      totalRooms: this.rooms.length,
    };
  },

  async sample(sampleMs = 8000) {
    const before = await Promise.all(this.rooms.map(room => captureRoomSnapshot(room)));
    await sleep(sampleMs);
    const after = await Promise.all(this.rooms.map(room => captureRoomSnapshot(room)));

    const details = this.rooms.map((room, index) => {
      const sendBytesDelta = positiveDelta(
        after[index].producer.bytesSent,
        before[index].producer.bytesSent
      );
      const recvBytesDelta = positiveDelta(
        after[index].consumer.bytesReceived,
        before[index].consumer.bytesReceived
      );
      const framesSentDelta = positiveDelta(
        after[index].producer.framesSent,
        before[index].producer.framesSent
      );
      const framesDecodedDelta = positiveDelta(
        after[index].consumer.framesDecoded,
        before[index].consumer.framesDecoded
      );

      const sendBitrateBps = bitrateFromDelta(sendBytesDelta, sampleMs);
      const recvBitrateBps = bitrateFromDelta(recvBytesDelta, sampleMs);
      const recvWidth = Math.max(
        after[index].consumer.frameWidth || 0,
        after[index].videoWidth || 0
      );
      const recvHeight = Math.max(
        after[index].consumer.frameHeight || 0,
        after[index].videoHeight || 0
      );

      const reasons = [];
      if (after[index].sendTransportState !== 'connected' && after[index].sendTransportState !== 'completed') {
        reasons.push(`sendTransport=${after[index].sendTransportState}`);
      }
      if (after[index].recvTransportState !== 'connected' && after[index].recvTransportState !== 'completed') {
        reasons.push(`recvTransport=${after[index].recvTransportState}`);
      }
      if (sendBitrateBps < this.config.minSendBitrateBps) {
        reasons.push(`sendBitrate=${sendBitrateBps}`);
      }
      if (recvBitrateBps < this.config.minRecvBitrateBps) {
        reasons.push(`recvBitrate=${recvBitrateBps}`);
      }
      if (recvWidth < this.config.minWidth || recvHeight < this.config.minHeight) {
        reasons.push(`recvResolution=${recvWidth}x${recvHeight}`);
      }
      if (framesSentDelta <= 0) {
        reasons.push('framesSent=0');
      }
      if (framesDecodedDelta <= 0) {
        reasons.push('framesDecoded=0');
      }

      return {
        roomId: room.roomId,
        sendTransportState: after[index].sendTransportState,
        recvTransportState: after[index].recvTransportState,
        sendBitrateBps,
        recvBitrateBps,
        framesSentDelta,
        framesDecodedDelta,
        recvWidth,
        recvHeight,
        packetsLost: after[index].consumer.packetsLost,
        jitter: after[index].consumer.jitter,
        qualityLimitationReason: after[index].producer.qualityLimitationReason,
        healthy: reasons.length === 0,
        reasons,
      };
    });

    const healthyRooms = details.filter(item => item.healthy).length;
    const unhealthyRooms = details.filter(item => !item.healthy);

    return {
      healthy: unhealthyRooms.length === 0,
      totalRooms: details.length,
      healthyRooms,
      unhealthyRooms: unhealthyRooms.length,
      sendBitrateAvgBps: average(details.map(item => item.sendBitrateBps)),
      recvBitrateAvgBps: average(details.map(item => item.recvBitrateBps)),
      sendBitrateMinBps: minimum(details.map(item => item.sendBitrateBps)),
      recvBitrateMinBps: minimum(details.map(item => item.recvBitrateBps)),
      details,
    };
  },

  async stop() {
    for (const room of [...this.rooms].reverse()) {
      await closeRoomPair(room);
    }
    this.rooms = [];
    return true;
  },

  getState() {
    return {
      wsUrl: this.wsUrl,
      config: this.config,
      totalRooms: this.rooms.length,
      roomIds: this.rooms.map(room => room.roomId),
    };
  },
};
