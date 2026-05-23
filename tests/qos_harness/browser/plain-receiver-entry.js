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
          const resolver = this.pending.get(message.id);
          if (resolver) {
            this.pending.delete(message.id);
            resolver(message);
          }
        } else if (message.notification === true) {
          this.notifications.push(message);
        }
      });
    });
  }

  request(method, data = {}) {
    const id = this.nextId++;
    const response = new Promise(resolve => this.pending.set(id, resolve));
    this.ws.send(JSON.stringify({ request: true, id, method, data }));
    return response;
  }

  async waitNotification(method, timeoutMs = 5000) {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      const index = this.notifications.findIndex(item => item.method === method);
      if (index !== -1) return this.notifications.splice(index, 1)[0];
      await new Promise(resolve => setTimeout(resolve, 50));
    }
    return null;
  }
}

function reportToArray(report) {
  return Array.from(report.values ? report.values() : report);
}

function inboundVideoStats(rows) {
  return rows
    .filter(row => row.type === 'inbound-rtp' && (row.kind === 'video' || row.mediaType === 'video'))
    .map(row => ({
      id: row.id,
      packetsReceived: Number(row.packetsReceived || 0),
      bytesReceived: Number(row.bytesReceived || 0),
      framesDecoded: Number(row.framesDecoded || 0),
      framesReceived: Number(row.framesReceived || 0),
      framesDropped: Number(row.framesDropped || 0),
      frameWidth: Number(row.frameWidth || 0),
      frameHeight: Number(row.frameHeight || 0),
      framesPerSecond: Number(row.framesPerSecond || 0),
      keyFramesDecoded: Number(row.keyFramesDecoded || 0),
    }));
}

function sumField(rows, field) {
  return rows.reduce((total, row) => total + Number(row[field] || 0), 0);
}

function maxField(rows, field) {
  return rows.reduce((value, row) => Math.max(value, Number(row[field] || 0)), 0);
}

function makeStatsSnapshot(consumers) {
  const inbound = consumers.flatMap(consumer => consumer.inbound);
  return {
    timestampMs: Date.now(),
    consumerCount: consumers.length,
    packetsReceived: sumField(inbound, 'packetsReceived'),
    bytesReceived: sumField(inbound, 'bytesReceived'),
    framesDecoded: sumField(inbound, 'framesDecoded'),
    framesReceived: sumField(inbound, 'framesReceived'),
    framesDropped: sumField(inbound, 'framesDropped'),
    frameWidth: maxField(inbound, 'frameWidth'),
    frameHeight: maxField(inbound, 'frameHeight'),
    framesPerSecond: maxField(inbound, 'framesPerSecond'),
    keyFramesDecoded: sumField(inbound, 'keyFramesDecoded'),
    videos: consumers.map(consumer => ({
      consumerId: consumer.consumerId,
      producerId: consumer.producerId,
      readyState: consumer.readyState,
      paused: consumer.paused,
      currentTime: consumer.currentTime,
      videoWidth: consumer.videoWidth,
      videoHeight: consumer.videoHeight,
      inbound: consumer.inbound,
    })),
  };
}

window.__plainReceiverHarness = {
  ws: null,
  device: null,
  recvTransport: null,
  consumers: [],
  keyframeRequests: 0,
  precreatedConsumers: 0,
  notificationConsumers: 0,
  diagnostics: {},

  async init(wsUrl, roomId, peerId) {
    this.consumers = [];
    this.keyframeRequests = 0;
    this.precreatedConsumers = 0;
    this.notificationConsumers = 0;
    this.diagnostics = {};

    this.ws = new WsClient(wsUrl);
    await this.ws.connect();

    const join = await this.ws.request('join', {
      roomId,
      peerId,
      displayName: peerId,
    });
    if (!join.ok) throw new Error(`join failed: ${JSON.stringify(join)}`);
    this.diagnostics.join = {
      existingProducers: join.data?.existingProducers || [],
      routerVideoCodecs: (join.data?.routerRtpCapabilities?.codecs || [])
        .filter(codec => String(codec.mimeType || '').toLowerCase().startsWith('video/'))
        .map(codec => ({
          mimeType: codec.mimeType,
          payloadType: codec.preferredPayloadType,
          parameters: codec.parameters || {},
        })),
    };

    if (!window.mediasoupClient?.Device) {
      throw new Error('mediasoupClient.Device is not available');
    }
    this.device = new window.mediasoupClient.Device();
    await this.device.load({ routerRtpCapabilities: join.data.routerRtpCapabilities });
    const deviceVideoCodecs = (this.device.rtpCapabilities?.codecs || [])
      .filter(codec => String(codec.mimeType || '').toLowerCase().startsWith('video/'))
      .map(codec => ({
        mimeType: codec.mimeType,
        payloadType: codec.preferredPayloadType ?? codec.payloadType,
        parameters: codec.parameters || {},
      }));
    const supportsH264Packetization1 = deviceVideoCodecs.some(codec => {
      const mimeType = String(codec.mimeType || '').toLowerCase();
      const packetizationMode = Number(codec.parameters?.['packetization-mode'] ?? 0);
      return mimeType === 'video/h264' && packetizationMode === 1;
    });
    this.diagnostics.device = {
      handlerName: this.device.handlerName,
      videoCodecs: deviceVideoCodecs,
      supportsH264Packetization1,
    };
    if (!supportsH264Packetization1) {
      const error = new Error('browser does not expose H264 packetization-mode=1 receive capability');
      error.code = 'BROWSER_H264_UNSUPPORTED';
      throw error;
    }

    const transportResponse = await this.ws.request('createWebRtcTransport', {
      producing: false,
      consuming: true,
      rtpCapabilities: this.device.rtpCapabilities,
    });
    this.diagnostics.createWebRtcTransport = {
      ok: transportResponse.ok === true,
      error: transportResponse.error || null,
      precreatedConsumers: transportResponse.data?.consumers || [],
    };
    if (!transportResponse.ok) {
      throw new Error(`createWebRtcTransport failed: ${JSON.stringify(transportResponse)}`);
    }

    this.recvTransport = this.device.createRecvTransport(transportResponse.data);
    this.recvTransport.on('connect', async ({ dtlsParameters }, callback, errback) => {
      try {
        const response = await this.ws.request('connectWebRtcTransport', {
          transportId: this.recvTransport.id,
          dtlsParameters,
        });
        if (!response.ok) throw new Error(response.error || 'connectWebRtcTransport failed');
        callback();
      } catch (error) {
        errback(error);
      }
    });

    for (const consumerData of transportResponse.data.consumers || []) {
      this.precreatedConsumers += 1;
      await this.consumeOne(consumerData);
    }

    while (this.consumers.length === 0) {
      const notification = await this.ws.waitNotification('newConsumer', 1000);
      if (!notification) break;
      this.notificationConsumers += 1;
      await this.consumeOne(notification.data);
    }

    if (this.consumers.length === 0 && Array.isArray(join.data.existingProducers)) {
      for (const producer of join.data.existingProducers) {
        if (producer.kind !== 'video') continue;
        const response = await this.ws.request('consume', {
          transportId: this.recvTransport.id,
          producerId: producer.producerId,
          rtpCapabilities: this.device.rtpCapabilities,
        });
        if (response.ok) await this.consumeOne(response.data);
      }
    }

    if (this.consumers.length === 0) throw new Error('no video consumer created');

    return {
      consumerCount: this.consumers.length,
      precreatedConsumers: this.precreatedConsumers,
      notificationConsumers: this.notificationConsumers,
      consumers: this.consumers.map(item => ({
        consumerId: item.consumer.id,
        producerId: item.consumer.producerId,
        kind: item.consumer.kind,
      })),
    };
  },

  async consumeOne(data) {
    if (data.kind !== 'video') return;
    const consumer = await this.recvTransport.consume({
      id: data.id,
      producerId: data.producerId,
      kind: data.kind,
      rtpParameters: data.rtpParameters,
      appData: {},
    });

    const element = document.createElement('video');
    element.autoplay = true;
    element.muted = true;
    element.playsInline = true;
    element.width = 320;
    element.height = 180;
    element.srcObject = new MediaStream([consumer.track]);
    document.body.appendChild(element);
    await element.play().catch(() => {});

    const response = await this.ws.request('requestConsumerKeyFrame', {
      consumerId: consumer.id,
    });
    if (response.ok) this.keyframeRequests += 1;

    this.consumers.push({ consumer, element });
  },

  async sampleStats() {
    const consumers = [];
    for (const item of this.consumers) {
      const stats = typeof item.consumer.getStats === 'function'
        ? await item.consumer.getStats()
        : await this.recvTransport.getStats();
      consumers.push({
        consumerId: item.consumer.id,
        producerId: item.consumer.producerId,
        readyState: item.consumer.track.readyState,
        paused: item.consumer.paused,
        currentTime: item.element.currentTime,
        videoWidth: item.element.videoWidth,
        videoHeight: item.element.videoHeight,
        inbound: inboundVideoStats(reportToArray(stats)),
      });
    }
    return makeStatsSnapshot(consumers);
  },

  async waitForMedia(durationMs = 8000, intervalMs = 500) {
    const first = await this.sampleStats();
    let last = first;
    const deadline = Date.now() + durationMs;
    while (Date.now() < deadline) {
      await new Promise(resolve => setTimeout(resolve, intervalMs));
      last = await this.sampleStats();
      const currentTimeMs = Math.round(
        Math.max(0, ...last.videos.map((video, index) =>
          (video.currentTime - (first.videos[index]?.currentTime || 0)) * 1000
        ))
      );
      if (last.packetsReceived > first.packetsReceived &&
        (last.framesDecoded > first.framesDecoded || currentTimeMs > 0 || last.frameWidth > 0)) {
        break;
      }
      if ((Date.now() - first.timestampMs) > 1000 && this.consumers[0]) {
        await this.ws.request('requestConsumerKeyFrame', {
          consumerId: this.consumers[0].consumer.id,
        });
      }
    }
    return {
      first,
      last,
      delta: {
        packetsReceived: last.packetsReceived - first.packetsReceived,
        bytesReceived: last.bytesReceived - first.bytesReceived,
        framesDecoded: last.framesDecoded - first.framesDecoded,
        framesReceived: last.framesReceived - first.framesReceived,
        currentTimeMs: Math.round(
          Math.max(0, ...last.videos.map((video, index) =>
            (video.currentTime - (first.videos[index]?.currentTime || 0)) * 1000
          ))
        ),
      },
      keyframeRequests: this.keyframeRequests,
      transportConnectionState: this.recvTransport.connectionState,
    };
  },
};
