import * as qos from '../../../src/client/lib/qos/index.js';

async function waitForConnection(pc) {
  if (pc.connectionState === 'connected') {
    return;
  }

  await new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      cleanup();
      reject(new Error(`peer connection did not connect, state=${pc.connectionState}`));
    }, 10000);
    const onState = () => {
      if (pc.connectionState === 'connected') {
        cleanup();
        resolve();
      } else if (pc.connectionState === 'failed' || pc.connectionState === 'closed') {
        cleanup();
        reject(new Error(`peer connection failed, state=${pc.connectionState}`));
      }
    };
    const cleanup = () => {
      clearTimeout(timeout);
      pc.removeEventListener('connectionstatechange', onState);
    };
    pc.addEventListener('connectionstatechange', onState);
  });
}

function createCanvasTrack() {
  const canvas = document.createElement('canvas');
  canvas.width = 640;
  canvas.height = 360;
  document.body.appendChild(canvas);

  const ctx = canvas.getContext('2d');
  let tick = 0;
  const draw = () => {
    tick += 1;
    ctx.fillStyle = '#0b1020';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = '#5eead4';
    ctx.fillRect((tick * 7) % (canvas.width - 80), 40, 80, 80);
    ctx.fillStyle = '#f8fafc';
    ctx.font = '32px sans-serif';
    ctx.fillText(`tick ${tick}`, 24, 180);
    requestAnimationFrame(draw);
  };
  draw();

  const stream = canvas.captureStream(24);
  const [track] = stream.getVideoTracks();
  return { canvas, stream, track };
}

async function createLoopbackPair() {
  const { stream, track } = createCanvasTrack();
  const remoteVideo = document.createElement('video');
  remoteVideo.autoplay = true;
  remoteVideo.muted = true;
  remoteVideo.playsInline = true;
  document.body.appendChild(remoteVideo);

  const pc1 = new RTCPeerConnection({ iceServers: [] });
  const pc2 = new RTCPeerConnection({ iceServers: [] });

  pc1.addEventListener('icecandidate', async event => {
    if (event.candidate) {
      await pc2.addIceCandidate(event.candidate);
    }
  });
  pc2.addEventListener('icecandidate', async event => {
    if (event.candidate) {
      await pc1.addIceCandidate(event.candidate);
    }
  });
  pc2.addEventListener('track', event => {
    if (event.streams[0]) {
      remoteVideo.srcObject = event.streams[0];
    }
  });

  const sender = pc1.addTrack(track, stream);
  const senderParams = sender.getParameters();
  senderParams.encodings = senderParams.encodings?.length
    ? senderParams.encodings
    : [{}];
  senderParams.encodings[0].maxBitrate = 120000;
  await sender.setParameters(senderParams);
  const offer = await pc1.createOffer();
  await pc1.setLocalDescription(offer);
  await pc2.setRemoteDescription(offer);
  const answer = await pc2.createAnswer();
  await pc2.setLocalDescription(answer);
  await pc1.setRemoteDescription(answer);

  await Promise.all([waitForConnection(pc1), waitForConnection(pc2)]);
  await new Promise(resolve => setTimeout(resolve, 1000));

  return { pc1, pc2, sender, originalTrack: track, remoteVideo };
}

function createLoopbackAdapter(sender, originalTrack) {
  return {
    getSnapshotBase() {
      return {
        source: 'camera',
        kind: 'video',
        producerId: 'loopback-producer',
        trackId: originalTrack.id,
        configuredBitrateBps: 120000,
      };
    },
    async getStatsReport() {
      return sender.getStats();
    },
    async setEncodingParameters(parameters) {
      const current = sender.getParameters();
      current.encodings = current.encodings?.length
        ? current.encodings
        : [{}];
      const encoding = current.encodings[0];
      if (typeof parameters.maxBitrateBps === 'number') {
        encoding.maxBitrate = parameters.maxBitrateBps;
      }
      if (typeof parameters.maxFramerate === 'number') {
        encoding.maxFramerate = parameters.maxFramerate;
      }
      if (typeof parameters.scaleResolutionDownBy === 'number') {
        encoding.scaleResolutionDownBy = parameters.scaleResolutionDownBy;
      }
      await sender.setParameters(current);
      return { applied: true };
    },
    async setMaxSpatialLayer() {
      return { applied: true };
    },
    async pauseUpstream() {
      await sender.replaceTrack(null);
      return { applied: true };
    },
    async resumeUpstream() {
      await sender.replaceTrack(originalTrack);
      return { applied: true };
    },
  };
}

async function createHarness() {
  const loopback = await createLoopbackPair();
  const adapter = createLoopbackAdapter(loopback.sender, loopback.originalTrack);
  const statsProvider = new qos.ProducerSenderStatsProvider(adapter);
  const profile = qos.resolveProfile('camera', {
    name: 'browser-loopback-camera',
    thresholds: {
      warnBitrateUtilization: 0.6,
      congestedBitrateUtilization: 0.4,
      stableBitrateUtilization: 0.7,
      warnLossRate: 0.04,
      congestedLossRate: 0.08,
      warnRttMs: 220,
      congestedRttMs: 320,
      stableLossRate: 0.03,
      stableRttMs: 180,
    },
    ladder: [
      {
        level: 0,
        description: 'loopback baseline',
        encodingParameters: {
          maxBitrateBps: 120000,
          maxFramerate: 24,
          scaleResolutionDownBy: 1,
        },
      },
      {
        level: 1,
        description: 'loopback mild degrade',
        encodingParameters: {
          maxBitrateBps: 100000,
          maxFramerate: 20,
          scaleResolutionDownBy: 1,
        },
      },
      {
        level: 2,
        description: 'loopback medium degrade',
        encodingParameters: {
          maxBitrateBps: 80000,
          maxFramerate: 16,
          scaleResolutionDownBy: 1.25,
        },
      },
      {
        level: 3,
        description: 'loopback heavy degrade',
        encodingParameters: {
          maxBitrateBps: 60000,
          maxFramerate: 12,
          scaleResolutionDownBy: 1.5,
        },
      },
      {
        level: 4,
        description: 'loopback audio-only',
        enterAudioOnly: true,
      },
    ],
  });
  const actionExecutor = new qos.QosActionExecutor({
    async execute(action) {
      switch (action.type) {
        case 'setEncodingParameters':
          return adapter.setEncodingParameters(action.encodingParameters);
        case 'setMaxSpatialLayer':
          return adapter.setMaxSpatialLayer(action.spatialLayer);
        case 'enterAudioOnly':
        case 'pauseUpstream':
          return adapter.pauseUpstream();
        case 'exitAudioOnly':
        case 'resumeUpstream':
          return adapter.resumeUpstream();
        case 'noop':
          return;
      }
    },
  });
  const controller = new qos.PublisherQosController({
    clock: new qos.SystemQosClock(),
    profile,
    statsProvider,
    actionExecutor,
    trackId: loopback.originalTrack.id,
    kind: 'video',
    producerId: 'loopback-producer',
    sampleIntervalMs: 500,
    snapshotIntervalMs: 5000,
    allowAudioOnly: true,
    allowVideoPause: true,
  });

  controller.start();

  return {
    controller,
    loopback,
    stop() {
      controller.stop();
      loopback.pc1.close();
      loopback.pc2.close();
      loopback.originalTrack.stop();
      if (loopback.remoteVideo.srcObject) {
        loopback.remoteVideo.srcObject.getTracks().forEach(track => track.stop());
      }
    },
    getState() {
      const trace = controller.getTraceEntries();
      return {
        state: controller.state,
        level: controller.level,
        trace,
        lastError: controller.getLastSampleError()?.message,
      };
    },
  };
}

window.__qosLoopbackHarness = {
  async init() {
    if (this.instance) {
      return true;
    }
    this.instance = await createHarness();
    return true;
  },
  async getState() {
    return this.instance.getState();
  },
  async stop() {
    this.instance?.stop();
    this.instance = undefined;
  },
};
