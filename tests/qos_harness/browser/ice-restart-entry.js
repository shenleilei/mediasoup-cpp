import { Device } from '../../../src/client/lib/Device.js';

const DEFAULT_TIMEOUT_MS = 15000;

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
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

function mediaKind(stat) {
  return stat?.kind || stat?.mediaType || null;
}

function sumVideoCounter(report, counter) {
  return statsReportToArray(report)
    .filter(stat => (
      (stat.type === 'outbound-rtp' || stat.type === 'inbound-rtp') &&
      !stat.isRemote &&
      mediaKind(stat) === 'video'
    ))
    .reduce((sum, stat) => sum + (Number(stat[counter]) || 0), 0);
}

function createCanvasTrack() {
  const canvas = document.createElement('canvas');
  canvas.width = 640;
  canvas.height = 360;
  canvas.style.cssText = 'width:320px;height:180px;';
  document.body.appendChild(canvas);

  const ctx = canvas.getContext('2d');
  let frame = 0;
  const draw = () => {
    frame += 1;
    for (let y = 0; y < 12; y += 1) {
      for (let x = 0; x < 20; x += 1) {
        const value = ((frame * 2654435761) ^ (x * 2246822519) ^ (y * 3266489917)) >>> 0;
        ctx.fillStyle = `rgb(${value & 255},${(value >> 8) & 255},${(value >> 16) & 255})`;
        ctx.fillRect(x * 32, y * 30, 32, 30);
      }
    }
    ctx.fillStyle = '#fff';
    ctx.font = '24px sans-serif';
    ctx.fillText(`ice-restart ${frame}`, 16, 34);
    requestAnimationFrame(draw);
  };
  draw();

  const stream = canvas.captureStream(30);
  return stream.getVideoTracks()[0];
}

function createHiddenVideo(track) {
  const video = document.createElement('video');
  video.autoplay = true;
  video.muted = true;
  video.playsInline = true;
  video.style.cssText = 'width:1px;height:1px;position:absolute;opacity:0;';
  video.srcObject = new MediaStream([track]);
  document.body.appendChild(video);
  video.play().catch(() => {});
  return video;
}

async function waitForPredicate(label, predicate, timeoutMs = DEFAULT_TIMEOUT_MS, intervalMs = 100) {
  const deadline = Date.now() + timeoutMs;
  let lastValue;

  while (Date.now() < deadline) {
    lastValue = await predicate();
    if (lastValue) {
      return lastValue;
    }
    await sleep(intervalMs);
  }

  throw new Error(`${label} timed out; last=${JSON.stringify(lastValue)}`);
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
      const timer = setTimeout(() => reject(new Error('websocket connect timeout')), 5000);

      this.ws.addEventListener('open', () => {
        clearTimeout(timer);
        resolve();
      }, { once: true });
      this.ws.addEventListener('error', () => {
        clearTimeout(timer);
        reject(new Error('websocket connect failed'));
      }, { once: true });
      this.ws.addEventListener('message', event => {
        const message = JSON.parse(event.data);
        if (message.response === true) {
          const pending = this.pending.get(message.id);
          if (pending) {
            this.pending.delete(message.id);
            pending(message);
          }
        } else if (message.notification === true) {
          this.notifications.push(message);
        }
      });
    });
  }

  async request(method, data = {}, timeoutMs = 10000) {
    const id = this.nextId++;
    const response = new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        this.pending.delete(id);
        reject(new Error(`request timeout: ${method}`));
      }, timeoutMs);

      this.pending.set(id, message => {
        clearTimeout(timer);
        resolve(message);
      });
    });

    this.ws.send(JSON.stringify({ request: true, id, method, data }));
    return response;
  }

  async waitNotification(method, timeoutMs = 5000) {
    const deadline = Date.now() + timeoutMs;

    while (Date.now() < deadline) {
      const index = this.notifications.findIndex(notification => notification.method === method);
      if (index !== -1) {
        return this.notifications.splice(index, 1)[0];
      }
      await sleep(50);
    }

    return null;
  }

  close() {
    try {
      this.ws?.close?.();
    } catch {}
  }
}

async function joinPeer(wsUrl, roomId, peerId) {
  const ws = new WsClient(wsUrl);
  await ws.connect();

  const device = new Device();
  const nativeHandler = device._handlerFactory();
  const nativeRtpCapabilities = await nativeHandler.getNativeRtpCapabilities();
  nativeHandler.close();

  const join = await ws.request('join', {
    roomId,
    peerId,
    displayName: peerId,
    rtpCapabilities: nativeRtpCapabilities,
  });
  if (!join.ok) {
    throw new Error(`${peerId} join failed: ${JSON.stringify(join)}`);
  }

  await device.load({ routerRtpCapabilities: join.data.routerRtpCapabilities });

  return { ws, device, peerId, joinData: join.data, rtpCapabilities: nativeRtpCapabilities };
}

async function createSendTransport(peer) {
  const response = await peer.ws.request('createWebRtcTransport', {
    producing: true,
    consuming: false,
  });
  if (!response.ok) {
    throw new Error(`send transport failed: ${JSON.stringify(response)}`);
  }

  const transport = peer.device.createSendTransport(response.data);
  transport.__stateLog = ['new'];
  transport.on('connectionstatechange', state => transport.__stateLog.push(state));
  transport.on('connect', async ({ dtlsParameters }, callback, errback) => {
    try {
      const connect = await peer.ws.request('connectWebRtcTransport', {
        transportId: transport.id,
        dtlsParameters,
      });
      if (!connect.ok) {
        throw new Error(connect.error || 'connectWebRtcTransport failed');
      }
      callback();
    } catch (error) {
      errback(error);
    }
  });
  transport.on('produce', async ({ kind, rtpParameters }, callback, errback) => {
    try {
      const produce = await peer.ws.request('produce', {
        transportId: transport.id,
        kind,
        rtpParameters,
      });
      if (!produce.ok) {
        throw new Error(produce.error || 'produce failed');
      }
      callback({ id: produce.data.id });
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
    throw new Error(`recv transport failed: ${JSON.stringify(response)}`);
  }

  const transport = peer.device.createRecvTransport(response.data);
  transport.__stateLog = ['new'];
  transport.on('connectionstatechange', state => transport.__stateLog.push(state));
  transport.on('connect', async ({ dtlsParameters }, callback, errback) => {
    try {
      const connect = await peer.ws.request('connectWebRtcTransport', {
        transportId: transport.id,
        dtlsParameters,
      });
      if (!connect.ok) {
        throw new Error(connect.error || 'connectWebRtcTransport failed');
      }
      callback();
    } catch (error) {
      errback(error);
    }
  });

  return transport;
}

async function waitForTransportState(transport, allowedStates, timeoutMs = DEFAULT_TIMEOUT_MS) {
  const allowed = new Set(allowedStates);

  return waitForPredicate(
    `transport ${transport.id} state ${allowedStates.join('/')}`,
    () => allowed.has(transport.connectionState) ? transport.connectionState : '',
    timeoutMs,
    100
  );
}

async function sampleStats(context) {
  const [producerStats, consumerStats] = await Promise.all([
    context.producer.getStats(),
    context.consumer.getStats(),
  ]);

  return {
    sendState: context.sendTransport.connectionState,
    recvState: context.recvTransport.connectionState,
    sendIceParameters: context.sendTransport._data?.iceParameters || null,
    recvIceParameters: context.recvTransport._data?.iceParameters || null,
    sendStates: context.sendTransport.__stateLog.slice(),
    recvStates: context.recvTransport.__stateLog.slice(),
    packetsSent: sumVideoCounter(producerStats, 'packetsSent'),
    bytesSent: sumVideoCounter(producerStats, 'bytesSent'),
    packetsReceived: sumVideoCounter(consumerStats, 'packetsReceived'),
    bytesReceived: sumVideoCounter(consumerStats, 'bytesReceived'),
    framesDecoded: sumVideoCounter(consumerStats, 'framesDecoded'),
    videoReadyState: context.video.readyState,
    videoWidth: context.video.videoWidth,
    videoHeight: context.video.videoHeight,
  };
}

async function waitForMediaGrowth(context, timeoutMs = DEFAULT_TIMEOUT_MS) {
  const before = await sampleStats(context);

  return waitForPredicate(
    'media stats growth',
    async () => {
      const after = await sampleStats(context);
      if (
        after.packetsSent > before.packetsSent &&
        after.bytesSent > before.bytesSent &&
        after.packetsReceived > before.packetsReceived &&
        after.bytesReceived > before.bytesReceived
      ) {
        return { before, after };
      }
      return '';
    },
    timeoutMs,
    500
  );
}

async function restartTransportIce(peer, transport) {
  const response = await peer.ws.request('restartIce', { transportId: transport.id });
  if (!response.ok) {
    throw new Error(`restartIce failed for ${transport.id}: ${JSON.stringify(response)}`);
  }
  if (!response.data?.iceParameters) {
    throw new Error(`restartIce missing iceParameters for ${transport.id}`);
  }

  await transport.restartIce({ iceParameters: response.data.iceParameters });
  return response.data.iceParameters;
}

window.__iceRestartHarness = {
  context: null,

  async init(wsUrl, roomId) {
    const publisher = await joinPeer(wsUrl, roomId, 'pub');
    const subscriber = await joinPeer(wsUrl, roomId, 'sub');

    const recvTransport = await createRecvTransport(subscriber);
    const sendTransport = await createSendTransport(publisher);
    const track = createCanvasTrack();
    const producer = await sendTransport.produce({
      track,
      encodings: [{ maxBitrate: 800000, maxFramerate: 30 }],
    });

    const notification = await subscriber.ws.waitNotification('newConsumer', DEFAULT_TIMEOUT_MS);
    if (!notification) {
      throw new Error('newConsumer not received');
    }

    const consumer = await recvTransport.consume({
      id: notification.data.id,
      producerId: notification.data.producerId,
      kind: notification.data.kind,
      rtpParameters: notification.data.rtpParameters,
    });
    const video = createHiddenVideo(consumer.track);

    this.context = {
      publisher,
      subscriber,
      sendTransport,
      recvTransport,
      producer,
      consumer,
      track,
      video,
      roomId,
      wsUrl,
    };

    await Promise.all([
      waitForTransportState(sendTransport, ['connected'], DEFAULT_TIMEOUT_MS),
      waitForTransportState(recvTransport, ['connected'], DEFAULT_TIMEOUT_MS),
    ]);
    await waitForMediaGrowth(this.context, DEFAULT_TIMEOUT_MS);

    return this.snapshot();
  },

  async snapshot() {
    return sampleStats(this.context);
  },

  async waitForFailed(timeoutMs = 45000) {
    const { sendTransport, recvTransport } = this.context;

    await waitForPredicate(
      'transport failed/disconnected',
      () => {
        const sendState = sendTransport.connectionState;
        const recvState = recvTransport.connectionState;
        if (
          sendState === 'failed' ||
          recvState === 'failed' ||
          sendState === 'disconnected' ||
          recvState === 'disconnected'
        ) {
          return { sendState, recvState };
        }
        return '';
      },
      timeoutMs,
      250
    );

    return this.snapshot();
  },

  async restartIce() {
    const { publisher, subscriber, sendTransport, recvTransport } = this.context;

    const [sendIce, recvIce] = await Promise.all([
      restartTransportIce(publisher, sendTransport),
      restartTransportIce(subscriber, recvTransport),
    ]);

    await Promise.all([
      waitForTransportState(sendTransport, ['connected'], DEFAULT_TIMEOUT_MS),
      waitForTransportState(recvTransport, ['connected'], DEFAULT_TIMEOUT_MS),
    ]);
    const growth = await waitForMediaGrowth(this.context, DEFAULT_TIMEOUT_MS);

    return {
      sendIce,
      recvIce,
      growth,
      snapshot: await this.snapshot(),
    };
  },

  async restartIceStepwise() {
    const { publisher, subscriber, sendTransport, recvTransport } = this.context;

    const sendIceResponse = await publisher.ws.request('restartIce', { transportId: sendTransport.id });
    const recvIceResponse = await subscriber.ws.request('restartIce', { transportId: recvTransport.id });

    if (!sendIceResponse.ok || !sendIceResponse.data?.iceParameters) {
      throw new Error(`send restartIce failed: ${JSON.stringify(sendIceResponse)}`);
    }
    if (!recvIceResponse.ok || !recvIceResponse.data?.iceParameters) {
      throw new Error(`recv restartIce failed: ${JSON.stringify(recvIceResponse)}`);
    }

    const beforeClientRestart = await this.snapshot();

    await sendTransport.restartIce({ iceParameters: sendIceResponse.data.iceParameters });
    const afterSendClientRestart = await this.snapshot();

    await recvTransport.restartIce({ iceParameters: recvIceResponse.data.iceParameters });
    const afterRecvClientRestart = await this.snapshot();

    return {
      sendIce: sendIceResponse.data.iceParameters,
      recvIce: recvIceResponse.data.iceParameters,
      beforeClientRestart,
      afterSendClientRestart,
      afterRecvClientRestart,
    };
  },

  async reconnectPublisherSignaling() {
    const { publisher, wsUrl, roomId } = this.context;
    const ws = new WsClient(wsUrl);
    await ws.connect();

    const join = await ws.request('join', {
      roomId,
      peerId: publisher.peerId,
      displayName: publisher.peerId,
      rtpCapabilities: publisher.rtpCapabilities,
    });
    if (!join.ok) {
      throw new Error(`publisher rejoin failed: ${JSON.stringify(join)}`);
    }

    publisher.ws = ws;
    publisher.joinData = join.data;

    return {
      joinMode: join.data?.joinMode || null,
      snapshot: await this.snapshot(),
    };
  },

  async waitForReconnected(timeoutMs = DEFAULT_TIMEOUT_MS) {
    const { sendTransport, recvTransport } = this.context;
    await Promise.all([
      waitForTransportState(sendTransport, ['connected', 'completed'], timeoutMs),
      waitForTransportState(recvTransport, ['connected', 'completed'], timeoutMs),
    ]);
    return this.snapshot();
  },

  async waitForGrowth(timeoutMs = DEFAULT_TIMEOUT_MS) {
    return waitForMediaGrowth(this.context, timeoutMs);
  },

  close() {
    const context = this.context;
    this.context = null;
    try { context?.producer?.close?.(); } catch {}
    try { context?.consumer?.close?.(); } catch {}
    try { context?.sendTransport?.close?.(); } catch {}
    try { context?.recvTransport?.close?.(); } catch {}
    try { context?.track?.stop?.(); } catch {}
    try { context?.publisher?.ws?.close?.(); } catch {}
    try { context?.subscriber?.ws?.close?.(); } catch {}
  },
};
