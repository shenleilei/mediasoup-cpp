(function () {
  const qosMode = new URLSearchParams(location.search).get('qos');
  const useLegacyQos = qosMode === 'legacy';
  const useFullQos = qosMode === 'full';

  const els = {
    status: document.getElementById('status'),
    roomInput: document.getElementById('roomInput'),
    joinBtn: document.getElementById('joinBtn'),
    publishBtn: document.getElementById('publishBtn'),
    stopBtn: document.getElementById('stopBtn'),
    log: document.getElementById('log'),
    videos: document.getElementById('videos'),
    localVideo: document.getElementById('localVideo'),
    localVideoBadge: document.getElementById('localVideoBadge'),
    qosContent: document.getElementById('qos-content'),
    metaRoom: document.getElementById('metaRoom'),
    metaPeer: document.getElementById('metaPeer'),
    metaQosMode: document.getElementById('metaQosMode'),
  };

  const state = {
    ws: null,
    device: null,
    sendTransport: null,
    recvTransport: null,
    roomId: '',
    peerId: '',
    reqId: 0,
    pending: new Map(),
    pendingConsumers: [],
    localStream: null,
    publishedProducers: new Map(),
    qosBundle: null,
    latestQosPolicy: null,
    latestQosOverride: null,
    latestConnectionQuality: null,
    latestRoomState: null,
    latestStatsReport: null,
    localDebugStats: null,
    debugStatsTimer: null,
    legacyStatsTimer: null,
    usingFallbackStream: false,
  };

  const qosLib = window.mediasoupClient && window.mediasoupClient.qos;

  function log(message) {
    const line = `[${new Date().toLocaleTimeString()}] ${message}`;
    els.log.textContent += `${line}\n`;
    els.log.scrollTop = els.log.scrollHeight;
    console.log(message);
  }

  function setStatus(text, level = 'warn') {
    els.status.className = `status ${level}`;
    els.status.innerHTML = `<strong>Status:</strong> ${text}`;
  }

  function escapeHtml(value) {
    return String(value ?? '')
      .replaceAll('&', '&amp;')
      .replaceAll('<', '&lt;')
      .replaceAll('>', '&gt;')
      .replaceAll('"', '&quot;')
      .replaceAll("'", '&#39;');
  }

  function fmtValue(value, fallback = '-') {
    if (value === undefined || value === null || value === '') {
      return fallback;
    }
    return String(value);
  }

  function fmtBitrate(bps) {
    if (bps === undefined || bps === null) {
      return '-';
    }
    if (bps >= 1000000) {
      return `${(bps / 1000000).toFixed(2)} Mbps`;
    }
    if (bps >= 1000) {
      return `${(bps / 1000).toFixed(0)} kbps`;
    }
    return `${bps} bps`;
  }

  function fmtLoss(lossRate) {
    if (lossRate === undefined || lossRate === null) {
      return '-';
    }
    return `${(lossRate * 100).toFixed(1)}%`;
  }

  function fmtMs(value, scale = 1) {
    if (value === undefined || value === null) {
      return '-';
    }
    return `${(value * scale).toFixed(0)} ms`;
  }

  function pillClass(stateName) {
    if (!stateName) {
      return 'stable';
    }
    return String(stateName).replaceAll(' ', '_');
  }

  function qosItem(label, value) {
    return (
      '<div class="qos-item">' +
      `<span class="label">${escapeHtml(label)}</span>` +
      `<span class="value">${escapeHtml(value)}</span>` +
      '</div>'
    );
  }

  function updateMeta() {
    els.metaRoom.textContent = state.roomId || '-';
    els.metaPeer.textContent = state.peerId || '-';
    els.metaQosMode.textContent = useLegacyQos
      ? 'legacy fallback'
      : useFullQos
        ? 'new library (full override)'
        : 'new library (no video pause)';
  }

  function resetQosState() {
    state.latestQosPolicy = null;
    state.latestQosOverride = null;
    state.latestConnectionQuality = null;
    state.latestRoomState = null;
    state.latestStatsReport = null;
    state.localDebugStats = null;
  }

  function buildDemoQosPolicy(basePolicy) {
    const policy = basePolicy || {};
    const profiles = policy.profiles || {};
    return {
      schema: 'mediasoup.qos.policy.v1',
      sampleIntervalMs: policy.sampleIntervalMs || 1000,
      snapshotIntervalMs: policy.snapshotIntervalMs || 2000,
      allowAudioOnly: false,
      allowVideoPause: false,
      profiles: {
        camera: profiles.camera || 'default',
        screenShare: profiles.screenShare || 'clarity-first',
        audio: profiles.audio || 'speech-first',
      },
    };
  }

  function shouldUseLocalNoPausePolicy() {
    return !useLegacyQos && !useFullQos;
  }

  function normalizeIncomingQosMessage(message) {
    if (message.method !== 'qosPolicy' || !shouldUseLocalNoPausePolicy()) {
      return message;
    }

    return {
      ...message,
      data: buildDemoQosPolicy(message.data),
    };
  }

  function buildFallbackCameraProfile() {
    return {
      name: 'demo-fallback-camera',
      source: 'camera',
      thresholds: {
        warnLossRate: 0.04,
        congestedLossRate: 0.08,
        warnRttMs: 220,
        congestedRttMs: 320,
        warnJitterMs: 30,
        congestedJitterMs: 60,
        warnBitrateUtilization: 0.45,
        congestedBitrateUtilization: 0.25,
        stableLossRate: 0.03,
        stableRttMs: 180,
        stableJitterMs: 20,
        stableBitrateUtilization: 0.55,
      },
      ladder: [
        {
          level: 0,
          description: 'Fallback canvas baseline.',
          encodingParameters: {
            maxBitrateBps: 450000,
            maxFramerate: 15,
            scaleResolutionDownBy: 1,
          },
        },
        {
          level: 1,
          description: 'Fallback canvas mild downgrade.',
          encodingParameters: {
            maxBitrateBps: 350000,
            maxFramerate: 12,
            scaleResolutionDownBy: 1,
          },
        },
        {
          level: 2,
          description: 'Fallback canvas moderate downgrade.',
          encodingParameters: {
            maxBitrateBps: 250000,
            maxFramerate: 10,
            scaleResolutionDownBy: 1.25,
          },
        },
        {
          level: 3,
          description: 'Fallback canvas aggressive downgrade.',
          encodingParameters: {
            maxBitrateBps: 180000,
            maxFramerate: 8,
            scaleResolutionDownBy: 1.5,
          },
        },
        {
          level: 4,
          description: 'Reserved audio-only fallback step.',
          enterAudioOnly: true,
        },
      ],
    };
  }

  async function wsRequest(method, data = {}) {
    if (!state.ws || state.ws.readyState !== WebSocket.OPEN) {
      throw new Error('websocket is not connected');
    }

    return new Promise((resolve, reject) => {
      const id = ++state.reqId;
      const timer = setTimeout(() => {
        state.pending.delete(id);
        reject(new Error(`request timeout: ${method}`));
      }, 10000);

      state.pending.set(id, {
        resolve: value => {
          clearTimeout(timer);
          resolve(value);
        },
        reject: error => {
          clearTimeout(timer);
          reject(error);
        },
      });

      state.ws.send(JSON.stringify({
        request: true,
        id,
        method,
        data,
      }));
    });
  }

  function resolveWsUrl(roomId) {
    const proto = location.protocol === 'https:' ? 'wss' : 'ws';
    const fallback = `${proto}://${location.hostname}:${location.port || 3000}/ws`;
    return fetch(`/api/resolve?roomId=${encodeURIComponent(roomId)}`)
      .then(resp => (resp.ok ? resp.json() : null))
      .then(data => {
        if (!data || !data.wsUrl) {
          return fallback;
        }
        const target = new URL(data.wsUrl);
        return `${proto}://${target.hostname}:${target.port || location.port || 3000}/ws`;
      })
      .catch(() => fallback);
  }

  async function connectWs(url) {
    if (state.ws && state.ws.readyState <= WebSocket.OPEN) {
      state.ws.close();
    }

    return new Promise((resolve, reject) => {
      const ws = new WebSocket(url);
      let settled = false;

      ws.onopen = () => {
        settled = true;
        state.ws = ws;
        ws.onmessage = handleMessage;
        ws.onclose = () => {
          setStatus('Disconnected', 'err');
          log('WebSocket closed');
        };
        resolve();
      };

      ws.onerror = () => {
        if (settled) {
          return;
        }
        reject(new Error('websocket connect failed'));
      };
    });
  }

  function dispatchQosNotification(message) {
    const normalizedMessage = normalizeIncomingQosMessage(message);

    if (state.qosBundle) {
      try {
        state.qosBundle.handleNotification(normalizedMessage);
      } catch (error) {
        log(`QoS notification dispatch failed: ${error.message}`);
      }
    }

    if (normalizedMessage.method === 'qosPolicy') {
      state.latestQosPolicy = normalizedMessage.data;
    } else if (normalizedMessage.method === 'qosOverride') {
      state.latestQosOverride = normalizedMessage.data;
    } else if (normalizedMessage.method === 'qosConnectionQuality') {
      state.latestConnectionQuality = normalizedMessage.data;
    } else if (normalizedMessage.method === 'qosRoomState') {
      state.latestRoomState = normalizedMessage.data;
    }

    renderQosPanel();
  }

  function handleMessage(event) {
    const message = JSON.parse(event.data);

    if (message.response === true) {
      const pending = state.pending.get(message.id);
      if (!pending) {
        return;
      }
      state.pending.delete(message.id);
      if (message.ok) {
        pending.resolve(message.data);
      } else {
        pending.reject(new Error(message.error || 'request failed'));
      }
      return;
    }

    if (message.notification !== true) {
      return;
    }

    if (
      message.method === 'qosPolicy' ||
      message.method === 'qosOverride' ||
      message.method === 'qosConnectionQuality' ||
      message.method === 'qosRoomState'
    ) {
      dispatchQosNotification(message);
      return;
    }

    if (message.method === 'statsReport') {
      state.latestStatsReport = message.data;
      renderQosPanel();
      return;
    }

    if (message.method === 'newConsumer') {
      void handleNewConsumer(message.data);
      return;
    }

    if (message.method === 'newProducer') {
      void consumeProducer(message.data.producerId, message.data.kind);
      return;
    }

    if (message.method === 'peerJoined') {
      log(`Peer joined: ${message.data.peerId}`);
      return;
    }

    if (message.method === 'peerLeft') {
      log(`Peer left: ${message.data.peerId}`);
      return;
    }

    log(`Notification: ${message.method}`);
  }

  async function handleNewConsumer(data) {
    if (!state.recvTransport) {
      state.pendingConsumers.push(data);
      log(`Queued consumer ${data.kind} from ${data.peerId || data.producerId} until recv transport is ready`);
      return;
    }

    try {
      const consumer = await state.recvTransport.consume({
        id: data.id,
        producerId: data.producerId,
        kind: data.kind,
        rtpParameters: data.rtpParameters,
        appData: {},
      });

      const element = document.createElement(consumer.kind === 'video' ? 'video' : 'audio');
      element.autoplay = true;
      element.playsInline = true;
      element.srcObject = new MediaStream([consumer.track]);
      element.play().catch(() => {});

      if (consumer.kind === 'video') {
        const card = document.createElement('div');
        card.className = 'video-card';
        card.dataset.remoteProducerId = data.producerId;

        const frame = document.createElement('div');
        frame.className = 'video-frame';
        frame.appendChild(element);

        const meta = document.createElement('div');
        meta.className = 'video-meta';
        meta.innerHTML =
          '<div>' +
          `<div class="video-title">Remote Video</div>` +
          `<div class="video-subtitle">${escapeHtml(data.peerId || data.producerId)}</div>` +
          '</div>' +
          '<div class="badge">subscribed</div>';

        card.appendChild(frame);
        card.appendChild(meta);
        els.videos.appendChild(card);
      }

      log(`Subscribed ${data.kind} from ${data.peerId || data.producerId}`);
    } catch (error) {
      log(`newConsumer error: ${error.message}`);
    }
  }

  async function consumeProducer(producerId, kind) {
    if (!state.recvTransport || !state.device) {
      return;
    }

    try {
      const response = await wsRequest('consume', {
        transportId: state.recvTransport.id,
        producerId,
        rtpCapabilities: state.device.rtpCapabilities,
      });

      await handleNewConsumer({
        id: response.id,
        producerId: response.producerId,
        kind: response.kind || kind,
        rtpParameters: response.rtpParameters,
      });
    } catch (error) {
      log(`Consume error: ${error.message}`);
    }
  }

  function buildFallbackStream() {
    const canvas = document.createElement('canvas');
    canvas.width = 960;
    canvas.height = 540;
    const ctx = canvas.getContext('2d');
    let frame = 0;
    setInterval(() => {
      frame += 1;
      ctx.fillStyle = `hsl(${(frame * 4) % 360}, 70%, 50%)`;
      ctx.fillRect(0, 0, canvas.width, canvas.height);
      ctx.fillStyle = '#ffffff';
      ctx.font = 'bold 40px sans-serif';
      ctx.fillText('mediasoup-cpp QoS Demo', 180, 220);
      ctx.font = '28px sans-serif';
      ctx.fillText(`Frame ${frame}`, 360, 290);
      ctx.fillText(new Date().toLocaleTimeString(), 305, 340);
    }, 66);

    const stream = canvas.captureStream(15);
    const audioContext = new AudioContext();
    const oscillator = audioContext.createOscillator();
    const destination = audioContext.createMediaStreamDestination();
    oscillator.connect(destination);
    oscillator.start();
    stream.addTrack(destination.stream.getAudioTracks()[0]);
    return stream;
  }

  function stopLegacyClientStatsReporting() {
    if (state.legacyStatsTimer) {
      clearInterval(state.legacyStatsTimer);
      state.legacyStatsTimer = null;
    }
  }

  function stopDebugStatsTimer() {
    if (state.debugStatsTimer) {
      clearInterval(state.debugStatsTimer);
      state.debugStatsTimer = null;
    }
  }

  async function collectLocalDebugStats() {
    const debug = {};

    if (state.sendTransport && state.sendTransport._handler && state.sendTransport._handler._pc) {
      const stats = await state.sendTransport._handler._pc.getStats();
      stats.forEach(report => {
        if (report.type === 'candidate-pair' && report.nominated) {
          debug.send = {
            availableOutgoingBitrate: report.availableOutgoingBitrate || 0,
            currentRoundTripTime: report.currentRoundTripTime || 0,
            totalRoundTripTime: report.totalRoundTripTime || 0,
            bytesSent: report.bytesSent || 0,
          };
        }
      });
    }

    if (state.recvTransport && state.recvTransport._handler && state.recvTransport._handler._pc) {
      const stats = await state.recvTransport._handler._pc.getStats();
      const inboundVideo = {};
      const inboundAudio = {};

      stats.forEach(report => {
        if (report.type === 'candidate-pair' && report.nominated) {
          debug.recv = {
            availableIncomingBitrate: report.availableIncomingBitrate || 0,
            currentRoundTripTime: report.currentRoundTripTime || 0,
            bytesReceived: report.bytesReceived || 0,
          };
        }
        if (report.type === 'inbound-rtp' && report.kind === 'video') {
          inboundVideo.jitter = report.jitter || 0;
          inboundVideo.packetsLost = report.packetsLost || 0;
          inboundVideo.framesPerSecond = report.framesPerSecond || 0;
          inboundVideo.frameWidth = report.frameWidth || 0;
          inboundVideo.frameHeight = report.frameHeight || 0;
        }
        if (report.type === 'inbound-rtp' && report.kind === 'audio') {
          inboundAudio.jitter = report.jitter || 0;
          inboundAudio.packetsLost = report.packetsLost || 0;
        }
      });

      if (Object.keys(inboundVideo).length) {
        debug.inboundVideo = inboundVideo;
      }
      if (Object.keys(inboundAudio).length) {
        debug.inboundAudio = inboundAudio;
      }
    }

    state.localDebugStats = debug;
  }

  function startDebugStatsTimer() {
    if (state.debugStatsTimer) {
      return;
    }

    state.debugStatsTimer = setInterval(async () => {
      try {
        await collectLocalDebugStats();
        renderQosPanel();
      } catch {
        // Ignore debug polling errors.
      }
    }, 2000);
  }

  function startLegacyClientStatsReporting() {
    if (state.legacyStatsTimer) {
      return;
    }

    state.legacyStatsTimer = setInterval(async () => {
      if (!state.ws || state.ws.readyState !== WebSocket.OPEN) {
        return;
      }

      try {
        await collectLocalDebugStats();
        const report = {};

        if (state.localDebugStats.send) {
          report.send = state.localDebugStats.send;
        }
        if (state.localDebugStats.recv) {
          report.recv = state.localDebugStats.recv;
        }
        if (state.localDebugStats.inboundVideo) {
          report.inboundVideo = state.localDebugStats.inboundVideo;
        }
        if (state.localDebugStats.inboundAudio) {
          report.inboundAudio = state.localDebugStats.inboundAudio;
        }

        if (Object.keys(report).length > 0) {
          await wsRequest('clientStats', report);
        }
      } catch {
        // Ignore legacy stats upload errors in demo mode.
      }
    }, 2000);
  }

  function stopQosController() {
    if (!state.qosBundle) {
      return;
    }

    try {
      state.qosBundle.controller.stop();
    } catch {
      // Ignore stop errors.
    }

    state.qosBundle = null;
    renderQosPanel();
  }

  function setupQosController(producer) {
    if (useLegacyQos) {
      log('Legacy QoS mode enabled via query parameter');
      startLegacyClientStatsReporting();
      return;
    }

    if (!qosLib || typeof qosLib.createMediasoupProducerQosController !== 'function') {
      log('QoS library is not available in browser bundle, falling back to legacy stats uploader');
      startLegacyClientStatsReporting();
      return;
    }

    stopLegacyClientStatsReporting();
    stopQosController();

    state.qosBundle = qosLib.createMediasoupProducerQosController({
      producer,
      source: 'camera',
      profile: state.usingFallbackStream ? buildFallbackCameraProfile() : undefined,
      allowAudioOnly: shouldUseLocalNoPausePolicy() ? false : undefined,
      allowVideoPause: shouldUseLocalNoPausePolicy() ? false : undefined,
      sendRequest: async (method, data) => {
        await wsRequest(method, data);
      },
    });

    if (state.latestQosPolicy) {
      state.qosBundle.handleNotification({
        method: 'qosPolicy',
        data: state.latestQosPolicy,
      });
    }
    if (state.latestQosOverride) {
      state.qosBundle.handleNotification({
        method: 'qosOverride',
        data: state.latestQosOverride,
      });
    }
    if (state.latestConnectionQuality) {
      state.qosBundle.handleNotification({
        method: 'qosConnectionQuality',
        data: state.latestConnectionQuality,
      });
    }
    if (state.latestRoomState) {
      state.qosBundle.handleNotification({
        method: 'qosRoomState',
        data: state.latestRoomState,
      });
    }

    state.qosBundle.controller.start();
    log(`QoS controller started for producer ${producer.id}`);
    renderQosPanel();
  }

  async function joinRoom() {
    const roomId = els.roomInput.value.trim() || 'test-room';
    const peerId = `peer-${Math.random().toString(36).slice(2, 8)}`;
    state.roomId = roomId;
    state.peerId = peerId;
    updateMeta();
    resetQosState();
    renderQosPanel();

    setStatus('Connecting...', 'warn');
    log(`Resolving room ${roomId}`);

    try {
      const wsUrl = await resolveWsUrl(roomId);
      await connectWs(wsUrl);
      setStatus('Connected', 'ok');
      log(`Connected to ${wsUrl}`);

      const joinResponse = await wsRequest('join', {
        roomId,
        peerId,
      });

      state.latestQosPolicy = joinResponse.qosPolicy || null;
      if (!useLegacyQos && !useFullQos) {
        const demoPolicy = buildDemoQosPolicy(joinResponse.qosPolicy);
        try {
          await wsRequest('setQosPolicy', {
            peerId,
            policy: demoPolicy,
          });
          state.latestQosPolicy = demoPolicy;
          log('Applied demo QoS policy: keep video visible, disable upstream video pause');
        } catch (error) {
          state.latestQosPolicy = demoPolicy;
          log(`Demo QoS policy update failed (${error.message}), enforcing no-pause policy locally`);
        }
      }
      state.device = new window.mediasoupClient.Device();
      await state.device.load({
        routerRtpCapabilities: joinResponse.routerRtpCapabilities,
      });

      const recvData = await wsRequest('createWebRtcTransport', {
        producing: false,
        consuming: true,
      });

      state.recvTransport = state.device.createRecvTransport(recvData);
      state.recvTransport.on('connect', async ({ dtlsParameters }, callback, errback) => {
        try {
          await wsRequest('connectWebRtcTransport', {
            transportId: state.recvTransport.id,
            dtlsParameters,
          });
          callback();
        } catch (error) {
          errback(error);
        }
      });

      const precreatedConsumers = recvData.consumers || [];
      for (const consumerData of precreatedConsumers) {
        await handleNewConsumer(consumerData);
      }

      if (state.pendingConsumers.length > 0) {
        const queued = state.pendingConsumers.slice();
        state.pendingConsumers = [];
        for (const consumerData of queued) {
          await handleNewConsumer(consumerData);
        }
      }

      const precreatedProducerIds = new Set(precreatedConsumers.map(item => item.producerId));
      for (const producerInfo of joinResponse.existingProducers || []) {
        if (!precreatedProducerIds.has(producerInfo.producerId)) {
          await consumeProducer(producerInfo.producerId, producerInfo.kind);
        }
      }

      els.publishBtn.disabled = false;
      els.stopBtn.disabled = true;
      setStatus(`Joined room ${roomId}`, 'ok');
      log(`Joined room ${roomId} as ${peerId}`);
      startDebugStatsTimer();
      renderQosPanel();
    } catch (error) {
      setStatus('Join failed', 'err');
      log(`Join failed: ${error.message}`);
    }
  }

  async function publishMedia() {
    if (!state.device) {
      return;
    }

    try {
      let stream;
      try {
        stream = await navigator.mediaDevices.getUserMedia({ audio: true, video: true });
        state.usingFallbackStream = false;
        log('Using camera + microphone');
      } catch (error) {
        state.usingFallbackStream = true;
        log(`getUserMedia failed (${error.message}), using fallback canvas stream`);
        stream = buildFallbackStream();
      }

      state.localStream = stream;
      els.localVideo.srcObject = stream;
      els.localVideoBadge.textContent = 'publishing';

      const sendData = await wsRequest('createWebRtcTransport', {
        producing: true,
        consuming: false,
      });

      state.sendTransport = state.device.createSendTransport(sendData);
      state.sendTransport.on('connect', async ({ dtlsParameters }, callback, errback) => {
        try {
          await wsRequest('connectWebRtcTransport', {
            transportId: state.sendTransport.id,
            dtlsParameters,
          });
          callback();
        } catch (error) {
          errback(error);
        }
      });
      state.sendTransport.on('produce', async ({ kind, rtpParameters }, callback, errback) => {
        try {
          const response = await wsRequest('produce', {
            transportId: state.sendTransport.id,
            kind,
            rtpParameters,
          });
          callback({ id: response.id });
        } catch (error) {
          errback(error);
        }
      });

      for (const track of stream.getTracks()) {
        const producer = await state.sendTransport.produce({ track });
        state.publishedProducers.set(producer.id, producer);
        log(`Producing ${track.kind} [${producer.id}]`);
        if (track.kind === 'video') {
          setupQosController(producer);
        }
      }

      els.publishBtn.disabled = true;
      els.stopBtn.disabled = false;
      setStatus(`Publishing in ${state.roomId}`, 'ok');
      startDebugStatsTimer();
      renderQosPanel();
    } catch (error) {
      setStatus('Publish failed', 'err');
      log(`Publish failed: ${error.message}`);
    }
  }

  function stopQos() {
    stopQosController();
    stopLegacyClientStatsReporting();
    els.stopBtn.disabled = true;
    els.publishBtn.disabled = !state.device;
    log('QoS control stopped');
    renderQosPanel();
  }

  function renderLocalQosSection() {
    const parts = ['<div class="qos-section"><h3>Local QoS Controller</h3>'];

    if (!state.qosBundle) {
      parts.push('<div class="hint">No active video QoS controller. Publish a video track to enable uplink QoS.</div></div>');
      return parts.join('');
    }

    const controller = state.qosBundle.controller;
    const trackState = controller.getTrackState();
    const runtime = controller.getRuntimeSettings();
    const trace = controller.getTraceEntries();
    const latestTrace = trace[trace.length - 1];
    const lastError = controller.getLastSampleError();

    parts.push(
      '<div class="track-card">' +
      '<div class="track-header">' +
      `<div class="track-title">${escapeHtml(trackState.source)} / ${escapeHtml(trackState.kind)}</div>` +
      `<div class="pill ${pillClass(trackState.state)}">${escapeHtml(trackState.state)} L${trackState.level}</div>` +
      '</div>' +
      '<div class="qos-grid">' +
      qosItem('Quality', trackState.quality) +
      qosItem('Audio-only', trackState.inAudioOnlyMode ? 'yes' : 'no') +
      qosItem('Sample Interval', `${runtime.sampleIntervalMs} ms`) +
      qosItem('Snapshot Interval', `${runtime.snapshotIntervalMs} ms`) +
      qosItem('Recovery Probe', runtime.probeActive ? 'active' : 'idle') +
      qosItem('Last Action', latestTrace ? latestTrace.plannedAction.type : '-') +
      qosItem('Send Bitrate', latestTrace ? fmtBitrate(latestTrace.signals.sendBitrateBps) : '-') +
      qosItem('Target Bitrate', latestTrace ? fmtBitrate(latestTrace.signals.targetBitrateBps) : '-') +
      qosItem('Loss', latestTrace ? fmtLoss(latestTrace.signals.lossRate) : '-') +
      qosItem('RTT', latestTrace ? fmtMs(latestTrace.signals.rttMs) : '-') +
      qosItem('Jitter', latestTrace ? fmtMs(latestTrace.signals.jitterMs) : '-') +
      qosItem('Trace Entries', String(trace.length)) +
      '</div>' +
      (lastError ? `<div class="hint" style="margin-top:8px;color:#fca5a5;">Last error: ${escapeHtml(lastError.message)}</div>` : '') +
      '</div>'
    );

    parts.push('<div class="qos-grid">');
    parts.push(qosItem('Latest Policy', state.latestQosPolicy ? state.latestQosPolicy.profiles?.camera || 'received' : 'none'));
    parts.push(qosItem('Policy Allow Audio-only', state.latestQosPolicy ? fmtBool(state.latestQosPolicy.allowAudioOnly) : '-'));
    parts.push(qosItem('Policy Allow Video Pause', state.latestQosPolicy ? fmtBool(state.latestQosPolicy.allowVideoPause) : '-'));
    parts.push(qosItem('Latest Override', state.latestQosOverride ? state.latestQosOverride.reason || 'received' : 'none'));
    parts.push(qosItem('Override Force Audio-only', state.latestQosOverride ? fmtBool(state.latestQosOverride.forceAudioOnly) : '-'));
    parts.push(qosItem('Override Max Level Clamp', state.latestQosOverride ? fmtValue(state.latestQosOverride.maxLevelClamp) : '-'));
    parts.push(qosItem('Override Disable Recovery', state.latestQosOverride ? fmtBool(state.latestQosOverride.disableRecovery) : '-'));
    parts.push(qosItem('Override TTL', state.latestQosOverride ? fmtValue(state.latestQosOverride.ttlMs) : '-'));
    parts.push(qosItem('Connection Quality', state.latestConnectionQuality ? state.latestConnectionQuality.quality : 'none'));
    parts.push(qosItem('Connection Stale', state.latestConnectionQuality ? fmtBool(state.latestConnectionQuality.stale) : '-'));
    parts.push(qosItem('Connection Lost', state.latestConnectionQuality ? fmtBool(state.latestConnectionQuality.lost) : '-'));
    parts.push(qosItem('Room State', state.latestRoomState ? state.latestRoomState.quality : 'none'));
    parts.push('</div></div>');
    return parts.join('');
  }

  function renderLocalDebugSection() {
    if (!state.localDebugStats || Object.keys(state.localDebugStats).length === 0) {
      return '<div class="qos-section"><h3>Local Debug Stats</h3><div class="hint">No local browser stats collected yet.</div></div>';
    }

    const send = state.localDebugStats.send || {};
    const recv = state.localDebugStats.recv || {};
    const inboundVideo = state.localDebugStats.inboundVideo || {};
    const inboundAudio = state.localDebugStats.inboundAudio || {};

    return (
      '<div class="qos-section">' +
      '<h3>Local Debug Stats</h3>' +
      '<div class="qos-grid">' +
      qosItem('Avail Outgoing', fmtBitrate(send.availableOutgoingBitrate)) +
      qosItem('Send RTT', fmtMs(send.currentRoundTripTime, 1000)) +
      qosItem('Avail Incoming', fmtBitrate(recv.availableIncomingBitrate)) +
      qosItem('Recv RTT', fmtMs(recv.currentRoundTripTime, 1000)) +
      qosItem('Inbound Video Jitter', fmtMs(inboundVideo.jitter, 1000)) +
      qosItem('Inbound Video Lost', fmtValue(inboundVideo.packetsLost)) +
      qosItem('Inbound Audio Jitter', fmtMs(inboundAudio.jitter, 1000)) +
      qosItem('Inbound Audio Lost', fmtValue(inboundAudio.packetsLost)) +
      qosItem(
        'Inbound Resolution',
        inboundVideo.frameWidth && inboundVideo.frameHeight
          ? `${inboundVideo.frameWidth}x${inboundVideo.frameHeight}`
          : '-'
      ) +
      qosItem('Inbound FPS', fmtValue(inboundVideo.framesPerSecond)) +
      '</div>' +
      '</div>'
    );
  }

  function renderServerPeer(peer) {
    const qos = peer.qos || {};
    const snapshot = peer.clientStats || {};
    const downlinkSnapshot = peer.downlinkClientStats || {};
    const downlinkQos = peer.downlinkQos || {};
    const tracks = snapshot.tracks || [];
    const stateName = qos.quality || 'unknown';
    let html =
      '<div class="peer-card">' +
      '<div class="peer-header">' +
      `<div class="peer-title">${escapeHtml(peer.peerId || 'peer')}</div>` +
      `<div class="pill quality-${pillClass(stateName)}">${escapeHtml(stateName)}</div>` +
      '</div>' +
      '<div class="qos-grid">' +
      qosItem('Snapshot Seq', fmtValue(snapshot.seq)) +
      qosItem('Peer Mode', fmtValue(snapshot.peerState ? snapshot.peerState.mode : snapshot.peerMode)) +
      qosItem('Peer Quality', fmtValue(snapshot.peerState ? snapshot.peerState.quality : snapshot.peerQuality)) +
      qosItem('Stale', qos.stale ? 'yes' : 'no') +
      qosItem('Last Updated', fmtValue(qos.lastUpdatedMs || peer.qosLastUpdatedMs)) +
      qosItem('DL Snapshot Seq', fmtValue(downlinkSnapshot.seq)) +
      qosItem('DL Health', fmtValue(downlinkQos.health)) +
      qosItem('DL Degrade', fmtValue(downlinkQos.degradeLevel)) +
      qosItem('DL Last Updated', fmtValue(peer.downlinkLastUpdatedMs)) +
      qosItem('Send Transport', peer.sendTransport ? 'present' : 'none') +
      qosItem('Recv Transport', peer.recvTransport ? 'present' : 'none') +
      '</div>';

    for (const track of tracks) {
      const signals = track.signals || {};
      html +=
        '<div class="track-card">' +
        '<div class="track-header">' +
        `<div class="track-title">${escapeHtml(track.source || track.kind || 'track')}</div>` +
        `<div class="pill ${pillClass(track.state)}">${escapeHtml(track.state)} L${fmtValue(track.ladderLevel, 0)}</div>` +
        '</div>' +
        '<div class="qos-grid">' +
        qosItem('Reason', fmtValue(track.reason)) +
        qosItem('Quality', fmtValue(track.quality)) +
        qosItem('Send Bitrate', fmtBitrate(signals.sendBitrateBps)) +
        qosItem('Target Bitrate', fmtBitrate(signals.targetBitrateBps)) +
        qosItem('Loss', fmtLoss(signals.lossRate)) +
        qosItem('RTT', fmtMs(signals.rttMs)) +
        qosItem('Jitter', fmtMs(signals.jitterMs)) +
        qosItem('Last Action', track.lastAction ? track.lastAction.type : '-') +
        '</div>' +
        '</div>';
    }

    html += '</div>';
    return html;
  }

  function renderServerRoomSection() {
    if (!state.latestStatsReport || !Array.isArray(state.latestStatsReport.peers)) {
      return '<div class="qos-section"><h3>Server Aggregate View</h3><div class="hint">Waiting for statsReport notification.</div></div>';
    }

    return (
      '<div class="qos-section">' +
      '<h3>Server Aggregate View</h3>' +
      state.latestStatsReport.peers.map(renderServerPeer).join('') +
      '</div>'
    );
  }

  function renderQosPanel() {
    const sections = [
      renderLocalQosSection(),
      renderLocalDebugSection(),
      renderServerRoomSection(),
    ];
    els.qosContent.innerHTML = `<div class="qos-layout">${sections.join('')}</div>`;
  }

  function init() {
    els.joinBtn.addEventListener('click', () => {
      void joinRoom();
    });
    els.publishBtn.addEventListener('click', () => {
      void publishMedia();
    });
    els.stopBtn.addEventListener('click', stopQos);
    updateMeta();
    renderQosPanel();

    if (!window.mediasoupClient) {
      setStatus('mediasoup client bundle missing', 'err');
      log('mediasoup client bundle did not load');
      return;
    }

    log(useLegacyQos ? 'Demo started in legacy QoS mode' : 'Demo started with new QoS library');
  }

  init();
})();
