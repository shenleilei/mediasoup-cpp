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
    localVideos: document.getElementById('localVideos'),
    remoteVideos: document.getElementById('remoteVideos'),
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
    localCaptureStreams: [],
    localVideoEntries: [],
    localAudioActive: false,
    publishedProducers: new Map(),
    remoteVideoConsumers: new Map(),
    qosSession: null,
    downlinkBundle: null,
    latestQosPolicy: null,
    latestQosOverride: null,
    latestConnectionQuality: null,
    latestRoomState: null,
    latestStatsReport: null,
    localDebugStats: null,
    debugInboundCounters: createEmptyDebugCounterState(),
    debugStatsTimer: null,
    legacyStatsTimer: null,
    legacyDebugModeActive: false,
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
    const label = document.createElement('strong');
    label.textContent = '状态：';
    els.status.replaceChildren(label, document.createTextNode(` ${String(text ?? '')}`));
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

  function asFiniteNumber(value) {
    return Number.isFinite(Number(value)) ? Number(value) : undefined;
  }

  function asFiniteOrZero(value) {
    return asFiniteNumber(value) ?? 0;
  }

  function createCounterMap() {
    return Object.create(null);
  }

  function createEmptyDebugCounterState() {
    return {
      video: createCounterMap(),
      audio: createCounterMap(),
    };
  }

  function makeInboundCounterKey(report, fallbackIndex) {
    if (report && typeof report.id === 'string' && report.id) {
      return report.id;
    }
    if (report && typeof report.mid === 'string' && report.mid) {
      return `mid:${report.mid}`;
    }
    if (Number.isFinite(asFiniteNumber(report?.ssrc))) {
      return `ssrc:${Number(report.ssrc)}`;
    }
    return `fallback:${fallbackIndex}`;
  }

  function computeCounterDelta(current, previous) {
    if (!Number.isFinite(current) || !Number.isFinite(previous)) {
      return 0;
    }
    return current >= previous ? current - previous : 0;
  }

  function fmtConcealRate(concealedSamples, totalSamplesReceived) {
    if (!Number.isFinite(concealedSamples) || !Number.isFinite(totalSamplesReceived) || totalSamplesReceived <= 0) {
      return '-';
    }
    return `${((concealedSamples / totalSamplesReceived) * 100).toFixed(1)}%`;
  }

  function fmtPercentValue(value, digits = 1) {
    const n = asFiniteNumber(value);
    return n === undefined ? '-' : `${n.toFixed(digits)}%`;
  }

  function fmtBool(value) {
    if (value === undefined || value === null) {
      return '-';
    }
    return value ? 'yes' : 'no';
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

  function summaryItem(label, value) {
    return (
      '<div class="mini-item">' +
      `<span class="label">${escapeHtml(label)}</span>` +
      `<span class="value">${escapeHtml(value)}</span>` +
      '</div>'
    );
  }

  function hasActiveLocalPublish() {
    return Boolean(
      state.sendTransport ||
      state.publishedProducers.size > 0 ||
      state.localCaptureStreams.length > 0
    );
  }

  function updateControls() {
    els.publishBtn.disabled = !state.device || hasActiveLocalPublish();
    els.stopBtn.disabled = !state.qosSession && !state.legacyDebugModeActive;
  }

  function createVideoCard({ title, subtitle, badgeText, stream, muted = false }) {
    const card = document.createElement('div');
    card.className = 'video-card';

    const frame = document.createElement('div');
    frame.className = 'video-frame';

    if (stream) {
      const element = document.createElement('video');
      element.autoplay = true;
      element.playsInline = true;
      element.muted = Boolean(muted);
      element.srcObject = stream;
      element.play().catch(() => {});
      frame.appendChild(element);
    } else {
      const empty = document.createElement('div');
      empty.className = 'video-empty';
      empty.textContent = subtitle || 'No media';
      frame.appendChild(empty);
    }

    const meta = document.createElement('div');
    meta.className = 'video-meta';
    meta.innerHTML =
      '<div>' +
      `<div class="video-title">${escapeHtml(title)}</div>` +
      `<div class="video-subtitle">${escapeHtml(subtitle)}</div>` +
      '</div>' +
      `<div class="badge">${escapeHtml(badgeText)}</div>`;

    card.appendChild(frame);
    card.appendChild(meta);
    return card;
  }

  function prettyTrackSource(source, kind = 'video') {
    if (source === 'screenShare') {
      return '屏幕共享';
    }
    if (source === 'audio' || kind === 'audio') {
      return '音频';
    }
    return '摄像头';
  }

  function remoteTrackTitle(entry) {
    return entry.remoteTrack?.localTrackId
      ? `Track ${entry.remoteTrack.localTrackId}`
      : `Track #${entry.fallbackTrackOrdinal || 1}`;
  }

  function remoteTrackSubtitle(entry) {
    const parts = [entry.peerId || 'remote-peer'];
    parts.push(prettyTrackSource(entry.remoteTrack?.source, entry.consumer?.kind));
    parts.push(shortId(entry.producerId));
    return parts.filter(Boolean).join(' · ');
  }

  function remoteTrackBadge(entry) {
    if (entry.remoteTrack?.quality || entry.remoteTrack?.state) {
      return `${entry.remoteTrack.quality || '-'} / ${entry.remoteTrack.state || '-'}`;
    }
    return '已订阅';
  }

  function formatRemoteServerSummary(serverState) {
    if (!serverState) {
      return '等待 statsReport';
    }

    const mode = serverState.paused ? '暂停' : '活动';
    return `${mode} · S${fmtValue(serverState.preferredSpatialLayer, 0)} / T${fmtValue(serverState.preferredTemporalLayer, 0)} / P${fmtValue(serverState.priority, 0)}`;
  }

  function getCurrentPeerStats() {
    if (!Array.isArray(state.latestStatsReport?.peers)) {
      return null;
    }
    return state.latestStatsReport.peers.find(peer => peer.peerId === state.peerId) || null;
  }

  function getCurrentQosDecision() {
    if (!state.qosSession || typeof state.qosSession.getDecision !== 'function') {
      return null;
    }
    return state.qosSession.getDecision();
  }

  function buildLocalBundleMap() {
    const map = new Map();
    const bundles = Array.isArray(state.qosSession?.bundles) ? state.qosSession.bundles : [];

    for (const bundle of bundles) {
      const snapshotBase = bundle?.producerAdapter?.getSnapshotBase?.();
      if (snapshotBase?.producerId) {
        map.set(snapshotBase.producerId, bundle);
      }
    }

    return map;
  }

  function latestProducerScore(producerStats) {
    const scores = Array.isArray(producerStats?.scores) ? producerStats.scores : [];
    if (scores.length === 0) {
      return '-';
    }
    return fmtValue(scores[scores.length - 1]?.score, '-');
  }

  function createLocalVideoCard(entry, index) {
    const card = createVideoCard({
      title: `Local Camera ${index + 1}`,
      subtitle: entry.label,
      badgeText: 'publishing',
      stream: entry.previewStream,
      muted: true,
    });

    const inspector = document.createElement('div');
    inspector.className = 'video-controls';

    const clientSummary = document.createElement('div');
    clientSummary.className = 'video-summary';

    const serverSummary = document.createElement('div');
    serverSummary.className = 'video-summary';

    const metricsGrid = document.createElement('div');
    metricsGrid.className = 'qos-grid';

    const errorSummary = document.createElement('div');
    errorSummary.className = 'video-summary is-alert';

    inspector.appendChild(clientSummary);
    inspector.appendChild(serverSummary);
    inspector.appendChild(metricsGrid);
    inspector.appendChild(errorSummary);
    card.appendChild(inspector);

    entry.card = card;
    entry.titleEl = card.querySelector('.video-title');
    entry.subtitleEl = card.querySelector('.video-subtitle');
    entry.badgeEl = card.querySelector('.badge');
    entry.clientSummaryEl = clientSummary;
    entry.serverSummaryEl = serverSummary;
    entry.metricsGridEl = metricsGrid;
    entry.errorSummaryEl = errorSummary;

    return card;
  }

  function updateLocalVideoCardUI(entry, index) {
    if (!entry || !entry.card) {
      return;
    }

    const bundle = entry.qosBundle;
    const controller = bundle?.controller;
    const trackState = controller?.getTrackState?.();
    const runtime = controller?.getRuntimeSettings?.();
    const trace = controller?.getTraceEntries?.() || [];
    const latestTrace = trace[trace.length - 1];
    const lastError = controller?.getLastSampleError?.();
    const serverTrack = entry.serverTrack || null;
    const serverProducer = entry.serverProducer || null;

    if (entry.titleEl) {
      entry.titleEl.textContent = `Local Camera ${index + 1}`;
    }
    if (entry.subtitleEl) {
      entry.subtitleEl.textContent = state.localAudioActive && index === 0
        ? `${entry.label} · microphone active`
        : entry.label;
    }
    if (entry.badgeEl) {
      entry.badgeEl.textContent = trackState
        ? `${trackState.quality} / ${trackState.state}`
        : 'publishing';
    }
    if (entry.clientSummaryEl) {
      entry.clientSummaryEl.innerHTML = trackState
        ? `<strong>Client</strong> ${escapeHtml(`${trackState.quality} / ${trackState.state} / L${trackState.level} · ${latestTrace?.plannedAction?.type || 'idle'}`)}`
        : '<strong>Client</strong> waiting for local QoS session';
    }
    if (entry.serverSummaryEl) {
      entry.serverSummaryEl.innerHTML = serverTrack
        ? `<strong>Server</strong> ${escapeHtml(`${serverTrack.quality || '-'} / ${serverTrack.state || '-'} / L${fmtValue(serverTrack.ladderLevel, 0)} · score ${latestProducerScore(serverProducer)}`)}`
        : `<strong>Server</strong> waiting for statsReport for producer ${escapeHtml(shortId(entry.producer?.id))}`;
    }
    if (entry.metricsGridEl) {
      entry.metricsGridEl.innerHTML = [
        qosItem('Producer', shortId(entry.producer?.id)),
        qosItem('Track', shortId(entry.track?.id)),
        qosItem('Sample Interval', runtime ? `${runtime.sampleIntervalMs} ms` : '-'),
        qosItem('Snapshot Interval', runtime ? `${runtime.snapshotIntervalMs} ms` : '-'),
        qosItem('Send Bitrate', latestTrace ? fmtBitrate(latestTrace.signals.sendBitrateBps) : '-'),
        qosItem('Target Bitrate', latestTrace ? fmtBitrate(latestTrace.signals.targetBitrateBps) : '-'),
        qosItem('Loss', latestTrace ? fmtLoss(latestTrace.signals.lossRate) : '-'),
        qosItem('RTT', latestTrace ? fmtMs(latestTrace.signals.rttMs) : '-'),
        qosItem('Jitter', latestTrace ? fmtMs(latestTrace.signals.jitterMs) : '-'),
        qosItem('Last Action', latestTrace ? latestTrace.plannedAction.type : '-'),
        qosItem('Server Reason', serverTrack ? fmtValue(serverTrack.reason) : '-'),
        qosItem('Producer Score', latestProducerScore(serverProducer)),
      ].join('');
    }
    if (entry.errorSummaryEl) {
      if (lastError) {
        entry.errorSummaryEl.style.display = '';
        entry.errorSummaryEl.innerHTML = `<strong>Error</strong> ${escapeHtml(lastError.message)}`;
      } else {
        entry.errorSummaryEl.style.display = 'none';
        entry.errorSummaryEl.textContent = '';
      }
    }
  }

  function syncLocalVideoEntries() {
    const currentPeerStats = getCurrentPeerStats();
    const bundleMap = buildLocalBundleMap();
    const serverTracks = Array.isArray(currentPeerStats?.clientStats?.tracks)
      ? currentPeerStats.clientStats.tracks
      : [];
    const serverProducers = currentPeerStats?.producers || {};

    state.localVideoEntries.forEach((entry, index) => {
      const producerId = entry.producer?.id;
      const trackId = entry.track?.id;
      entry.qosBundle = producerId ? bundleMap.get(producerId) || null : null;
      entry.serverTrack = serverTracks.find(track =>
        (producerId && track.producerId === producerId) ||
        (trackId && track.localTrackId === trackId)
      ) || null;
      entry.serverProducer = producerId ? serverProducers[producerId] || null : null;
      if (!entry.card) {
        createLocalVideoCard(entry, index);
      }
      updateLocalVideoCardUI(entry, index);
    });
  }

  function renderLocalPreviewCards(cameraEntries = state.localVideoEntries, hasAudioTrack = state.localAudioActive) {
    els.localVideos.innerHTML = '';

    if (!cameraEntries || cameraEntries.length === 0) {
      els.localVideos.appendChild(createVideoCard({
        title: '本地发布',
        subtitle: '发布麦克风和所有检测到的摄像头。',
        badgeText: '空闲',
      }));
      return;
    }

    cameraEntries.forEach((entry, index) => {
      if (!entry.card) {
        createLocalVideoCard(entry, index);
      }
      updateLocalVideoCardUI(entry, index, hasAudioTrack);
      els.localVideos.appendChild(entry.card);
    });
  }

  function clearRemoteVideoCards() {
    els.remoteVideos.innerHTML = '';
    state.remoteVideoConsumers.clear();
    renderRemoteVideoGroups();
  }

  function createRemotePeerGroup(peerId, entries) {
    const group = document.createElement('section');
    group.className = 'remote-peer-group';
    group.dataset.peerId = peerId;

    const head = document.createElement('div');
    head.className = 'remote-peer-head';

    const copy = document.createElement('div');
    const title = document.createElement('div');
    title.className = 'remote-peer-title';
    title.textContent = peerId || 'remote-peer';

    const statsPeer = Array.isArray(state.latestStatsReport?.peers)
      ? state.latestStatsReport.peers.find(peer => peer.peerId === peerId) || null
      : null;
    const producerCount = statsPeer?.producers ? Object.keys(statsPeer.producers).length : entries.length;
    const subtitle = document.createElement('div');
    subtitle.className = 'remote-peer-subtitle';
    subtitle.textContent = `tracks ${entries.length} · producers ${producerCount}`;

    copy.appendChild(title);
    copy.appendChild(subtitle);

    const badge = document.createElement('div');
    badge.className = 'badge';
    badge.textContent = statsPeer?.qos?.quality || 'remote';

    head.appendChild(copy);
    head.appendChild(badge);

    const grid = document.createElement('div');
    grid.className = 'remote-peer-grid';
    for (const entry of entries) {
      grid.appendChild(entry.card);
    }

    group.appendChild(head);
    group.appendChild(grid);
    return group;
  }

  function renderRemoteVideoGroups() {
    els.remoteVideos.innerHTML = '';

    if (state.remoteVideoConsumers.size === 0) {
      els.remoteVideos.appendChild(createVideoCard({
        title: '远端订阅',
        subtitle: '加入房间并等待远端发布后，这里会按 peer 分组显示多路 video track。',
        badgeText: '空闲',
      }));
      return;
    }

    const groups = new Map();
    for (const entry of state.remoteVideoConsumers.values()) {
      const key = entry.peerId || 'remote-peer';
      const list = groups.get(key) || [];
      list.push(entry);
      groups.set(key, list);
    }

    const sortedGroups = Array.from(groups.entries()).sort((left, right) => left[0].localeCompare(right[0]));
    for (const [peerId, entries] of sortedGroups) {
      entries.sort((left, right) => {
        const leftKey = left.remoteTrack?.localTrackId || left.producerId || left.consumer?.id || '';
        const rightKey = right.remoteTrack?.localTrackId || right.producerId || right.consumer?.id || '';
        return leftKey.localeCompare(rightKey);
      });
      entries.forEach((entry, index) => {
        entry.fallbackTrackOrdinal = index + 1;
      });
      els.remoteVideos.appendChild(createRemotePeerGroup(peerId, entries));
    }
  }

  function updateRemoteCardUI(entry) {
    if (!entry || !entry.card || !entry.serverSummaryEl || !entry.metricsGridEl) {
      return;
    }

    const allPeerStats = Array.isArray(state.latestStatsReport?.peers) ? state.latestStatsReport.peers : [];
    entry.remotePeerStats = allPeerStats.find(peer => peer.peerId === entry.peerId) || null;
    const remoteTracks = Array.isArray(entry.remotePeerStats?.clientStats?.tracks)
      ? entry.remotePeerStats.clientStats.tracks
      : [];
    entry.remoteTrack = remoteTracks.find(track => track.producerId === entry.producerId) || null;
    entry.remoteProducer = entry.remotePeerStats?.producers?.[entry.producerId] || null;

    const serverState = entry.serverState || {};
    const serverSub = entry.serverSubscription || {};
    const recvRttMs = state.localDebugStats?.recv?.currentRoundTripTime;

    if (entry.titleEl) {
      entry.titleEl.textContent = remoteTrackTitle(entry);
    }
    if (entry.subtitleEl) {
      entry.subtitleEl.textContent = remoteTrackSubtitle(entry);
    }
    if (entry.badgeEl) {
      entry.badgeEl.textContent = remoteTrackBadge(entry);
    }
    entry.serverSummaryEl.innerHTML = `<strong>远端 QoS</strong> ${escapeHtml(formatRemoteServerSummary(entry.serverState))}`;
    entry.metricsGridEl.innerHTML = [
      qosItem('Consumer', shortId(entry.consumer?.id)),
      qosItem('Producer', shortId(entry.producerId)),
      qosItem('Track', fmtValue(entry.remoteTrack?.localTrackId, '-')),
      qosItem('Source', prettyTrackSource(entry.remoteTrack?.source, entry.consumer?.kind)),
      qosItem('状态', serverState.paused ? '暂停' : '活动'),
      qosItem('Producer 暂停', fmtBool(serverState.producerPaused)),
      qosItem('空间层', fmtValue(serverState.preferredSpatialLayer, 0)),
      qosItem('时间层', fmtValue(serverState.preferredTemporalLayer, 0)),
      qosItem('优先级', fmtValue(serverState.priority, 0)),
      qosItem('Consumer 分数', fmtValue(serverState.score, 0)),
      qosItem('Producer 分数', fmtValue(serverState.producerScore, 0)),
      qosItem('Publisher QoS', fmtValue(entry.remotePeerStats?.qos?.quality, '-')),
      qosItem('可见', fmtBool(serverSub.visible)),
      qosItem('置顶', fmtBool(serverSub.pinned)),
      qosItem('主讲', fmtBool(serverSub.activeSpeaker)),
      qosItem('共享屏幕', fmtBool(serverSub.isScreenShare)),
      qosItem('目标尺寸', fmtDimensions(serverSub.targetWidth, serverSub.targetHeight)),
      qosItem('渲染尺寸', fmtDimensions(serverSub.frameWidth, serverSub.frameHeight)),
      qosItem('RTT', fmtMs(recvRttMs, 1000)),
      qosItem('帧率', fmtValue(serverSub.framesPerSecond)),
      qosItem('丢包', fmtPercentValue(serverSub.packetsLost)),
      qosItem('抖动', fmtMs(serverSub.jitter, 1000)),
      qosItem('卡顿率', fmtPercentValue(asFiniteNumber(serverSub.freezeRate) * 100)),
      qosItem('Freeze Count', fmtValue(serverSub.freezeCount)),
      qosItem('Frames Dropped', fmtValue(serverSub.framesDropped)),
      qosItem('JB Delay', fmtMs(serverSub.jitterBufferDelayMs)),
      qosItem('Audio Conceal', fmtConcealRate(serverSub.concealedSamples, serverSub.totalSamplesReceived)),
    ].join('');
  }

  function createRemoteVideoControls(entry) {
    const controls = document.createElement('div');
    controls.className = 'video-controls';

    const serverSummary = document.createElement('div');
    serverSummary.className = 'video-summary';

    const metricsGrid = document.createElement('div');
    metricsGrid.className = 'qos-grid';
    controls.appendChild(serverSummary);
    controls.appendChild(metricsGrid);
    entry.serverSummaryEl = serverSummary;
    entry.metricsGridEl = metricsGrid;

    updateRemoteCardUI(entry);
    return controls;
  }

  function syncRemoteConsumerServerState() {
    if (!Array.isArray(state.latestStatsReport?.peers)) {
      return;
    }

    const localPeerStats = state.latestStatsReport.peers.find(peer => peer.peerId === state.peerId);
    if (!localPeerStats) {
      return;
    }

    const consumers = localPeerStats.consumers || {};
    const subscriptions = Array.isArray(localPeerStats.downlinkClientStats?.subscriptions)
      ? localPeerStats.downlinkClientStats.subscriptions
      : [];
    const subscriptionByConsumerId = new Map(
      subscriptions
        .filter(item => item && item.consumerId)
        .map(item => [item.consumerId, item])
    );

    for (const entry of state.remoteVideoConsumers.values()) {
      entry.serverState = consumers[entry.consumer.id] || null;
      entry.serverSubscription = subscriptionByConsumerId.get(entry.consumer.id) || null;
      updateRemoteCardUI(entry);
    }

    renderRemoteVideoGroups();
  }

  function stopMediaStream(stream) {
    if (!stream) {
      return;
    }

    for (const track of stream.getTracks()) {
      try {
        track.stop();
      } catch {
        // Ignore stop errors.
      }
    }

    if (typeof stream.__demoCleanup === 'function') {
      try {
        stream.__demoCleanup();
      } catch {
        // Ignore stream cleanup errors.
      }
    }
  }

  function getTrackCameraKey(track) {
    const settings = typeof track.getSettings === 'function' ? track.getSettings() : {};
    if (settings.groupId && track.label) {
      return `${settings.groupId}::${track.label}`;
    }
    return settings.deviceId || track.label || '';
  }

  function getDeviceCameraKey(device) {
    if (device.groupId && device.label) {
      return `${device.groupId}::${device.label}`;
    }
    return device.deviceId || device.label || '';
  }

  async function captureAllLocalMedia() {
    const primaryStream = await navigator.mediaDevices.getUserMedia({ audio: true, video: true });
    const captureStreams = [primaryStream];
    const cameraEntries = [];
    const claimedCameraKeys = new Set();
    const audioTrack = primaryStream.getAudioTracks()[0] || null;
    const primaryVideoTrack = primaryStream.getVideoTracks()[0] || null;

    if (primaryVideoTrack) {
      const primaryKey = getTrackCameraKey(primaryVideoTrack) || `camera-${cameraEntries.length + 1}`;
      claimedCameraKeys.add(primaryKey);
      cameraEntries.push({
        track: primaryVideoTrack,
        previewStream: new MediaStream([primaryVideoTrack]),
        label: primaryVideoTrack.label || 'Default camera',
      });
    }

    if (navigator.mediaDevices && typeof navigator.mediaDevices.enumerateDevices === 'function') {
      const devices = await navigator.mediaDevices.enumerateDevices();
      for (const device of devices) {
        if (device.kind !== 'videoinput') {
          continue;
        }

        const deviceKey = getDeviceCameraKey(device);
        if (deviceKey && claimedCameraKeys.has(deviceKey)) {
          continue;
        }

        try {
          const stream = await navigator.mediaDevices.getUserMedia({
            audio: false,
            video: device.deviceId ? { deviceId: { exact: device.deviceId } } : true,
          });
          const track = stream.getVideoTracks()[0];
          if (!track) {
            stopMediaStream(stream);
            continue;
          }

          const trackKey = getTrackCameraKey(track) || deviceKey || `camera-${cameraEntries.length + 1}`;
          if (claimedCameraKeys.has(trackKey)) {
            stopMediaStream(stream);
            continue;
          }

          claimedCameraKeys.add(trackKey);
          captureStreams.push(stream);
          cameraEntries.push({
            track,
            previewStream: new MediaStream([track]),
            label: track.label || device.label || `Camera ${cameraEntries.length + 1}`,
          });
        } catch (error) {
          log(`Skipping camera ${device.label || device.deviceId || 'unknown'}: ${error.message}`);
        }
      }
    }

    return {
      audioTrack,
      cameraEntries,
      captureStreams,
    };
  }

  function resetLocalPublishState() {
    stopLegacyClientStatsReporting();
    stopLocalQosSession();

    if (state.sendTransport) {
      try {
        state.sendTransport.close();
      } catch {
        // Ignore transport close errors.
      }
      state.sendTransport = null;
    }

    state.publishedProducers.clear();

    for (const stream of state.localCaptureStreams) {
      stopMediaStream(stream);
    }
    state.localCaptureStreams = [];
    state.localVideoEntries = [];
    state.localAudioActive = false;
    state.debugInboundCounters.audio = createCounterMap();
    renderLocalPreviewCards([]);
    updateControls();
  }

  function resetRemoteMediaState() {
    stopDownlinkReporting();

    if (state.recvTransport) {
      try {
        state.recvTransport.close();
      } catch {
        // Ignore transport close errors.
      }
      state.recvTransport = null;
    }

    state.pendingConsumers = [];
    state.debugInboundCounters.video = createCounterMap();
    clearRemoteVideoCards();
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
    state.debugInboundCounters = createEmptyDebugCounterState();
  }

  function supportsDownlinkQos() {
    return (
      qosLib &&
      typeof qosLib.DownlinkHints === 'function' &&
      typeof qosLib.DownlinkSampler === 'function' &&
      typeof qosLib.DownlinkReporter === 'function'
    );
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
          if (state.ws !== ws) {
            return;
          }
          state.ws = null;
          stopDownlinkReporting();
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

    if (state.qosSession) {
      try {
        state.qosSession.handleNotification(normalizedMessage);
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
      syncRemoteConsumerServerState();
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
        const card = createVideoCard({
          title: '远端视频',
          subtitle: data.peerId || data.producerId,
          badgeText: '已订阅',
          stream: new MediaStream([consumer.track]),
        });
        const renderedElement = card.querySelector('video');
        card.dataset.remoteProducerId = data.producerId;
        card.dataset.consumerId = consumer.id;

        const entry = {
          consumer,
          producerId: data.producerId,
          peerId: data.peerId || data.producerId,
          element: renderedElement,
          hintTargetEl: card.querySelector('.video-frame'),
          card,
          serverState: null,
          serverSubscription: null,
          remotePeerStats: null,
          remoteTrack: null,
          remoteProducer: null,
          titleEl: card.querySelector('.video-title'),
          subtitleEl: card.querySelector('.video-subtitle'),
          badgeEl: card.querySelector('.badge'),
          serverSummaryEl: null,
          metricsGridEl: null,
        };
        const controls = createRemoteVideoControls(entry);

        card.appendChild(controls);

        registerRemoteVideoConsumer(entry);
        renderRemoteVideoGroups();
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
    const frameTimer = setInterval(() => {
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
    stream.__demoCleanup = () => {
      clearInterval(frameTimer);
      try {
        oscillator.stop();
      } catch {
        // Ignore oscillator stop errors.
      }
      return audioContext.close();
    };
    return stream;
  }

  function stopLegacyClientStatsReporting() {
    state.legacyDebugModeActive = false;
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

  function removeRemoteVideoConsumer(consumerId) {
    const entry = state.remoteVideoConsumers.get(consumerId);
    if (entry && entry.card && typeof entry.card.remove === 'function') {
      entry.card.remove();
    }
    state.remoteVideoConsumers.delete(consumerId);
    if (state.downlinkBundle) {
      state.downlinkBundle.hints.remove(consumerId);
      if (state.downlinkBundle.sampler && typeof state.downlinkBundle.sampler.removeHints === 'function') {
        state.downlinkBundle.sampler.removeHints(consumerId);
      }
    }
    renderRemoteVideoGroups();
  }

  function computeConsumerHint(entry) {
    const target = entry.hintTargetEl || entry.card || entry.element;
    const rect = target && typeof target.getBoundingClientRect === 'function'
      ? target.getBoundingClientRect()
      : { width: 0, height: 0 };
    const width = Math.round(rect.width || entry.element.clientWidth || entry.element.videoWidth || 0);
    const height = Math.round(rect.height || entry.element.clientHeight || entry.element.videoHeight || 0);
    const styles = target ? getComputedStyle(target) : null;
    const visible = Boolean(
      entry.element &&
      entry.element.isConnected &&
      target &&
      target.isConnected &&
      width > 8 &&
      height > 8 &&
      styles &&
      styles.display !== 'none' &&
      styles.visibility !== 'hidden'
    );
    let hintVisible = visible;
    let targetWidth = width;
    let targetHeight = height;
    if (!hintVisible) {
      targetWidth = 0;
      targetHeight = 0;
    }

    return {
      producerId: entry.producerId,
      kind: entry.consumer?.kind,
      mid: entry.consumer?.rtpParameters?.mid,
      visible: hintVisible,
      pinned: false,
      activeSpeaker: false,
      isScreenShare: false,
      targetWidth,
      targetHeight,
    };
  }

  function refreshDownlinkHints() {
    if (!state.downlinkBundle) {
      return;
    }

    for (const [consumerId, entry] of state.remoteVideoConsumers) {
      if (!entry.element || !entry.element.isConnected || entry.consumer.track.readyState === 'ended') {
        removeRemoteVideoConsumer(consumerId);
        continue;
      }

      state.downlinkBundle.hints.set(consumerId, computeConsumerHint(entry));
      updateRemoteCardUI(entry);
    }
  }

  function stopDownlinkReporting() {
    if (!state.downlinkBundle) {
      return;
    }

    if (state.downlinkBundle.hintTimer) {
      clearInterval(state.downlinkBundle.hintTimer);
    }
    try {
      state.downlinkBundle.reporter.stop();
    } catch {
      // Ignore stop errors.
    }

    state.downlinkBundle = null;
  }

  function ensureDownlinkReporting() {
    if (state.downlinkBundle || !state.recvTransport || !state.peerId) {
      return;
    }

    if (!supportsDownlinkQos()) {
      log('Downlink QoS reporter is not available in browser bundle');
      return;
    }

    const hints = new qosLib.DownlinkHints();
    const sampler = new qosLib.DownlinkSampler(state.recvTransport);
    const reporter = new qosLib.DownlinkReporter({
      sampler,
      hints,
      send: async (method, data) => {
        await wsRequest(method, data);
      },
      peerId: state.peerId,
      intervalMs: 2000,
    });

    state.downlinkBundle = {
      hints,
      sampler,
      reporter,
      hintTimer: setInterval(refreshDownlinkHints, 500),
    };

    refreshDownlinkHints();
    reporter.start();
    void reporter.reportNow();
    log(`Downlink QoS reporter started for subscriber ${state.peerId}`);
  }

  function registerRemoteVideoConsumer(entry) {
    state.remoteVideoConsumers.set(entry.consumer.id, entry);
    try {
      entry.consumer.on('transportclose', () => removeRemoteVideoConsumer(entry.consumer.id));
      entry.consumer.on('trackended', () => removeRemoteVideoConsumer(entry.consumer.id));
    } catch {
      // Ignore consumer event binding failures.
    }

    if (state.downlinkBundle) {
      state.downlinkBundle.hints.set(entry.consumer.id, computeConsumerHint(entry));
      void state.downlinkBundle.reporter.reportNow();
    }

    syncRemoteConsumerServerState();
    updateRemoteCardUI(entry);
  }

  async function collectLocalDebugStats() {
    const debug = {};

    if (state.sendTransport && typeof state.sendTransport.getStats === 'function') {
      const stats = await state.sendTransport.getStats();
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

    if (state.recvTransport && typeof state.recvTransport.getStats === 'function') {
      const stats = await state.recvTransport.getStats();
      const inboundVideo = {
        jitter: 0,
        packetsLost: 0,
        packetsReceived: 0,
        packetsLostDelta: 0,
        framesPerSecond: 0,
        frameWidth: 0,
        frameHeight: 0,
      };
      const inboundAudio = {
        jitter: 0,
        packetsLost: 0,
        packetsReceived: 0,
        packetsLostDelta: 0,
        concealedSamples: 0,
        totalSamplesReceived: 0,
      };
      const videoFreeze = {
        freezeCount: 0,
        totalFreezesDuration: 0,
        framesDropped: 0,
        jitterBufferDelayMs: undefined,
      };
      const nextVideoCounters = createCounterMap();
      const nextAudioCounters = createCounterMap();
      let sawVideo = false;
      let sawAudio = false;
      let totalVideoJitterBufferDelay = 0;
      let totalVideoJitterBufferCount = 0;
      let inboundReportIndex = 0;

      stats.forEach(report => {
        if (report.type === 'candidate-pair' && report.nominated) {
          debug.recv = {
            availableIncomingBitrate: report.availableIncomingBitrate || 0,
            currentRoundTripTime: report.currentRoundTripTime || 0,
            bytesReceived: report.bytesReceived || 0,
          };
        }
        if (report.type === 'inbound-rtp' && report.kind === 'video') {
          sawVideo = true;
          const currentLost = asFiniteOrZero(report.packetsLost);
          const currentReceived = asFiniteOrZero(report.packetsReceived);
          const counterKey = makeInboundCounterKey(report, inboundReportIndex++);
          const previous = state.debugInboundCounters.video?.[counterKey];
          nextVideoCounters[counterKey] = {
            packetsLost: currentLost,
            packetsReceived: currentReceived,
          };
          inboundVideo.jitter = Math.max(inboundVideo.jitter, asFiniteOrZero(report.jitter));
          inboundVideo.packetsLost += currentLost;
          inboundVideo.packetsReceived += currentReceived;
          inboundVideo.packetsLostDelta += computeCounterDelta(currentLost, previous?.packetsLost);
          inboundVideo.framesPerSecond = Math.max(inboundVideo.framesPerSecond, asFiniteOrZero(report.framesPerSecond));
          inboundVideo.frameWidth = Math.max(inboundVideo.frameWidth, asFiniteOrZero(report.frameWidth));
          inboundVideo.frameHeight = Math.max(inboundVideo.frameHeight, asFiniteOrZero(report.frameHeight));
          videoFreeze.freezeCount += asFiniteOrZero(report.freezeCount);
          videoFreeze.totalFreezesDuration += asFiniteOrZero(report.totalFreezesDuration);
          videoFreeze.framesDropped += asFiniteOrZero(report.framesDropped);
          totalVideoJitterBufferDelay += asFiniteOrZero(report.jitterBufferDelay);
          totalVideoJitterBufferCount += asFiniteOrZero(report.jitterBufferEmittedCount);
        }
        if (report.type === 'inbound-rtp' && report.kind === 'audio') {
          sawAudio = true;
          const currentLost = asFiniteOrZero(report.packetsLost);
          const currentReceived = asFiniteOrZero(report.packetsReceived);
          const counterKey = makeInboundCounterKey(report, inboundReportIndex++);
          const previous = state.debugInboundCounters.audio?.[counterKey];
          nextAudioCounters[counterKey] = {
            packetsLost: currentLost,
            packetsReceived: currentReceived,
          };
          inboundAudio.jitter = Math.max(inboundAudio.jitter, asFiniteOrZero(report.jitter));
          inboundAudio.packetsLost += currentLost;
          inboundAudio.packetsReceived += currentReceived;
          inboundAudio.packetsLostDelta += computeCounterDelta(currentLost, previous?.packetsLost);
          inboundAudio.concealedSamples += asFiniteOrZero(report.concealedSamples);
          inboundAudio.totalSamplesReceived += asFiniteOrZero(report.totalSamplesReceived);
        }
      });

      if (sawVideo) {
        debug.inboundVideo = inboundVideo;
        state.debugInboundCounters.video = nextVideoCounters;
        if (totalVideoJitterBufferCount > 0) {
          videoFreeze.jitterBufferDelayMs =
            (totalVideoJitterBufferDelay / totalVideoJitterBufferCount) * 1000;
        } else if (totalVideoJitterBufferDelay > 0) {
          videoFreeze.jitterBufferDelayMs = totalVideoJitterBufferDelay * 1000;
        }
        if (
          videoFreeze.freezeCount > 0 ||
          videoFreeze.totalFreezesDuration > 0 ||
          videoFreeze.framesDropped > 0 ||
          videoFreeze.jitterBufferDelayMs !== undefined
        ) {
          debug.videoFreeze = videoFreeze;
        }
      } else {
        state.debugInboundCounters.video = createCounterMap();
      }
      if (sawAudio) {
        debug.inboundAudio = inboundAudio;
        state.debugInboundCounters.audio = nextAudioCounters;
      } else {
        state.debugInboundCounters.audio = createCounterMap();
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
    if (state.legacyDebugModeActive) {
      return;
    }
    state.legacyDebugModeActive = true;
    state.legacyStatsTimer = null;
    log('Legacy QoS mode keeps local browser debug only; server-side clientStats upload is disabled because mediasoup.qos.client.v1 is required.');
    updateControls();
  }

  function createSingleProducerQosSession(producer) {
    const bundle = qosLib.createMediasoupProducerQosController({
      producer,
      source: 'camera',
      profile: state.usingFallbackStream ? buildFallbackCameraProfile() : undefined,
      allowAudioOnly: shouldUseLocalNoPausePolicy() ? false : undefined,
      allowVideoPause: shouldUseLocalNoPausePolicy() ? false : undefined,
      sendRequest: async (method, data) => {
        await wsRequest(method, data);
      },
    });

    return {
      bundles: [bundle],
      startAll() {
        bundle.controller.start();
      },
      stopAll() {
        bundle.controller.stop();
      },
      async sampleAllNow() {
        await bundle.controller.sampleNow();
      },
      handleNotification(message) {
        bundle.handleNotification(message);
      },
      getDecision() {
        const track = bundle.controller.getTrackState();
        return {
          peerQuality: track.quality,
          prioritizedTrackIds: [track.trackId],
          sacrificialTrackIds: [],
          keepAudioAlive: true,
          preferScreenShare: false,
          allowVideoRecovery: true,
        };
      },
    };
  }

  function stopLocalQosSession() {
    if (!state.qosSession) {
      return;
    }

    try {
      state.qosSession.stopAll();
    } catch {
      // Ignore stop errors.
    }

    state.qosSession = null;
    state.localVideoEntries.forEach(entry => {
      entry.qosBundle = null;
    });
    renderLocalPreviewCards();
    renderQosPanel();
    updateControls();
  }

  function createLocalQosSession(videoProducers) {
    if (!qosLib || videoProducers.length === 0) {
      return null;
    }

    if (typeof qosLib.createMediasoupPeerQosSession === 'function') {
      return qosLib.createMediasoupPeerQosSession({
        producers: videoProducers.map(producer => ({
          producer,
          source: 'camera',
          profile: state.usingFallbackStream ? buildFallbackCameraProfile() : undefined,
        })),
        sendRequest: async (method, data) => {
          await wsRequest(method, data);
        },
        allowAudioOnly: shouldUseLocalNoPausePolicy() ? false : undefined,
        allowVideoPause: shouldUseLocalNoPausePolicy() ? false : undefined,
      });
    }

    if (typeof qosLib.createMediasoupProducerQosController === 'function') {
      if (videoProducers.length > 1) {
        log('Peer QoS session API missing, falling back to the first video producer only');
      }
      return createSingleProducerQosSession(videoProducers[0]);
    }

    return null;
  }

  function setupLocalQosSession(videoProducers) {
    if (useLegacyQos) {
      log('Legacy QoS mode enabled via query parameter');
      startLegacyClientStatsReporting();
      updateControls();
      return;
    }

    const session = createLocalQosSession(videoProducers);
    if (!session) {
      log('QoS library is not available in browser bundle, falling back to legacy local debug mode');
      startLegacyClientStatsReporting();
      updateControls();
      return;
    }

    stopLegacyClientStatsReporting();
    stopLocalQosSession();
    state.qosSession = session;

    if (state.latestQosPolicy) {
      state.qosSession.handleNotification({
        method: 'qosPolicy',
        data: state.latestQosPolicy,
      });
    }
    if (state.latestQosOverride) {
      state.qosSession.handleNotification({
        method: 'qosOverride',
        data: state.latestQosOverride,
      });
    }
    if (state.latestConnectionQuality) {
      state.qosSession.handleNotification({
        method: 'qosConnectionQuality',
        data: state.latestConnectionQuality,
      });
    }
    if (state.latestRoomState) {
      state.qosSession.handleNotification({
        method: 'qosRoomState',
        data: state.latestRoomState,
      });
    }

    state.qosSession.startAll();
    void state.qosSession.sampleAllNow?.();
    syncLocalVideoEntries();
    renderLocalPreviewCards();
    log(`QoS session started for ${videoProducers.length} video producer(s)`);
    renderQosPanel();
    updateControls();
  }

  async function joinRoom() {
    const roomId = els.roomInput.value.trim() || 'test-room';
    const peerId = `peer-${Math.random().toString(36).slice(2, 8)}`;

    state.device = null;
    resetLocalPublishState();
    resetRemoteMediaState();
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

      ensureDownlinkReporting();

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

      setStatus(`Joined room ${roomId}`, 'ok');
      log(`Joined room ${roomId} as ${peerId}`);
      startDebugStatsTimer();
      renderQosPanel();
      updateControls();
    } catch (error) {
      setStatus('Join failed', 'err');
      log(`Join failed: ${error.message}`);
      updateControls();
    }
  }

  async function publishMedia() {
    if (!state.device || hasActiveLocalPublish()) {
      return;
    }

    try {
      let audioTrack = null;
      let cameraEntries = [];
      let captureStreams = [];

      try {
        const capture = await captureAllLocalMedia();
        audioTrack = capture.audioTrack;
        cameraEntries = capture.cameraEntries;
        captureStreams = capture.captureStreams;
        state.usingFallbackStream = false;
        log(`Using ${cameraEntries.length} camera(s)${audioTrack ? ' + microphone' : ''}`);
      } catch (error) {
        state.usingFallbackStream = true;
        log(`getUserMedia failed (${error.message}), using fallback canvas stream`);
        const fallbackStream = buildFallbackStream();
        audioTrack = fallbackStream.getAudioTracks()[0] || null;
        const fallbackVideoTrack = fallbackStream.getVideoTracks()[0] || null;
        captureStreams = [fallbackStream];
        cameraEntries = fallbackVideoTrack
          ? [{
              track: fallbackVideoTrack,
              previewStream: new MediaStream([fallbackVideoTrack]),
              label: 'Fallback canvas stream',
            }]
          : [];
      }

      if (cameraEntries.length === 0) {
        throw new Error('no camera tracks available');
      }

      state.localCaptureStreams = captureStreams;
      renderLocalPreviewCards(cameraEntries, Boolean(audioTrack));

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

      if (audioTrack) {
        const audioProducer = await state.sendTransport.produce({ track: audioTrack });
        state.publishedProducers.set(audioProducer.id, audioProducer);
        log(`Producing audio [${audioProducer.id}]`);
      }

      const videoProducers = [];
      for (const [index, entry] of cameraEntries.entries()) {
        const producer = await state.sendTransport.produce({ track: entry.track });
        state.publishedProducers.set(producer.id, producer);
        videoProducers.push(producer);
        log(`Producing video #${index + 1} (${entry.label}) [${producer.id}]`);
      }

      state.localAudioActive = Boolean(audioTrack);
      state.localVideoEntries = cameraEntries.map((entry, index) => ({
        ...entry,
        producer: videoProducers[index] || null,
        qosBundle: null,
        serverTrack: null,
        serverProducer: null,
        card: null,
        titleEl: null,
        subtitleEl: null,
        badgeEl: null,
        clientSummaryEl: null,
        serverSummaryEl: null,
        metricsGridEl: null,
        errorSummaryEl: null,
      }));
      setupLocalQosSession(videoProducers);
      syncLocalVideoEntries();
      renderLocalPreviewCards();
      setStatus(`Publishing ${cameraEntries.length} camera(s) in ${state.roomId}`, 'ok');
      startDebugStatsTimer();
      renderQosPanel();
      updateControls();
    } catch (error) {
      resetLocalPublishState();
      setStatus('Publish failed', 'err');
      log(`Publish failed: ${error.message}`);
      renderQosPanel();
      updateControls();
    }
  }

  function stopQos() {
    stopLocalQosSession();
    stopLegacyClientStatsReporting();
    log('Local producer QoS control stopped');
    renderQosPanel();
    updateControls();
  }

  function renderOverviewSection() {
    const currentPeerStats = getCurrentPeerStats();
    const decision = getCurrentQosDecision();
    const downlinkQos = currentPeerStats?.downlinkQos || {};
    const peerQos = currentPeerStats?.qos || {};

    return (
      '<div class="qos-section">' +
      '<h3>Current Peer</h3>' +
      '<div class="mini-grid">' +
      summaryItem('Peer Quality', fmtValue(peerQos.quality)) +
      summaryItem('Downlink Health', fmtValue(downlinkQos.health)) +
      summaryItem('Connection', state.latestConnectionQuality ? state.latestConnectionQuality.quality : 'none') +
      summaryItem('Room State', state.latestRoomState ? state.latestRoomState.quality : 'none') +
      summaryItem('Local Cameras', String(state.localVideoEntries.length)) +
      summaryItem('Remote Videos', String(state.remoteVideoConsumers.size)) +
      summaryItem('Policy Camera', state.latestQosPolicy ? state.latestQosPolicy.profiles?.camera || 'received' : 'none') +
      summaryItem('Override', state.latestQosOverride ? state.latestQosOverride.reason || 'received' : 'none') +
      summaryItem('Priority Tracks', decision ? String(decision.prioritizedTrackIds.length) : '-') +
      summaryItem('Sacrificial', decision ? String(decision.sacrificialTrackIds.length) : '-') +
      summaryItem('Recovery', decision ? fmtBool(decision.allowVideoRecovery) : '-') +
      summaryItem('Keep Audio', decision ? fmtBool(decision.keepAudioAlive) : '-') +
      '</div>' +
      '</div>'
    );
  }

  function renderTransportDebugSection() {
    if (!state.localDebugStats || Object.keys(state.localDebugStats).length === 0) {
      return '<div class="qos-section"><h3>Transport Debug</h3><div class="hint">No local browser transport stats collected yet.</div></div>';
    }

    const send = state.localDebugStats.send || {};
    const recv = state.localDebugStats.recv || {};
    const inboundVideo = state.localDebugStats.inboundVideo || {};
    const inboundAudio = state.localDebugStats.inboundAudio || {};
    const videoFreeze = state.localDebugStats.videoFreeze || {};

    return (
      '<div class="qos-section">' +
      '<h3>Transport Debug</h3>' +
      '<div class="mini-grid">' +
      summaryItem('Downlink Reporter', state.downlinkBundle ? 'running' : 'off') +
      summaryItem('Legacy Upload', state.legacyDebugModeActive ? 'disabled' : 'n/a') +
      summaryItem('Remote Hints', String(state.remoteVideoConsumers.size)) +
      summaryItem('Avail Out', fmtBitrate(send.availableOutgoingBitrate)) +
      summaryItem('Send RTT', fmtMs(send.currentRoundTripTime, 1000)) +
      summaryItem('Avail In', fmtBitrate(recv.availableIncomingBitrate)) +
      summaryItem('Recv RTT', fmtMs(recv.currentRoundTripTime, 1000)) +
      summaryItem(
        'Inbound Size',
        inboundVideo.frameWidth && inboundVideo.frameHeight
          ? `${inboundVideo.frameWidth}x${inboundVideo.frameHeight}`
          : '-'
      ) +
      summaryItem('Inbound FPS', fmtValue(inboundVideo.framesPerSecond)) +
      summaryItem('Video Loss Δ', fmtValue(inboundVideo.packetsLostDelta)) +
      summaryItem('Video Jitter', fmtMs(inboundVideo.jitter, 1000)) +
      summaryItem('Audio Loss Δ', fmtValue(inboundAudio.packetsLostDelta)) +
      summaryItem('Audio Jitter', fmtMs(inboundAudio.jitter, 1000)) +
      summaryItem(
        'Audio Conceal',
        fmtConcealRate(inboundAudio.concealedSamples, inboundAudio.totalSamplesReceived)
      ) +
      summaryItem('Freeze Count', fmtValue(videoFreeze.freezeCount)) +
      summaryItem('Frames Dropped', fmtValue(videoFreeze.framesDropped)) +
      summaryItem('JB Delay', fmtMs(videoFreeze.jitterBufferDelayMs)) +
      '</div>' +
      '</div>'
    );
  }

  function shortId(value, size = 8) {
    if (value === undefined || value === null || value === '') {
      return '-';
    }

    const text = String(value);
    if (text.length <= size) {
      return text;
    }
    return `${text.slice(0, size)}...`;
  }

  function fmtDimensions(width, height) {
    if (!width || !height) {
      return '-';
    }
    return `${width}x${height}`;
  }

  function renderPeerOverviewCard(peer) {
    const qos = peer.qos || {};
    const snapshot = peer.clientStats || {};
    const downlinkSnapshot = peer.downlinkClientStats || {};
    const downlinkQos = peer.downlinkQos || {};
    const tracks = snapshot.tracks || [];
    const consumers = peer.consumers || {};
    const producers = peer.producers || {};
    const subscriptionByConsumerId = new Map(
      (Array.isArray(downlinkSnapshot.subscriptions) ? downlinkSnapshot.subscriptions : [])
        .filter(item => item && item.consumerId)
        .map(item => [item.consumerId, item])
    );
    const stateName = qos.quality || 'unknown';
    const visibleCount = Array.from(subscriptionByConsumerId.values()).filter(item => item.visible).length;
    const pinnedCount = Array.from(subscriptionByConsumerId.values()).filter(item => item.pinned).length;
    return (
      '<div class="peer-row">' +
      '<div class="peer-row-head">' +
      '<div>' +
      `<div class="peer-row-title">${escapeHtml(peer.peerId || 'peer')}</div>` +
      `<div class="peer-row-subtitle">tracks ${tracks.length} · producers ${Object.keys(producers).length} · consumers ${Object.keys(consumers).length} · visible ${visibleCount} · pinned ${pinnedCount}</div>` +
      '</div>' +
      `<div class="pill quality-${pillClass(stateName)}">${escapeHtml(peer.peerId === state.peerId ? `${stateName} · you` : stateName)}</div>` +
      '</div>' +
      '<div class="mini-grid">' +
      summaryItem('Mode', fmtValue(snapshot.peerState ? snapshot.peerState.mode : snapshot.peerMode)) +
      summaryItem('Stale', fmtBool(qos.stale)) +
      summaryItem('DL Health', fmtValue(downlinkQos.health)) +
      summaryItem('DL Degrade', fmtValue(downlinkQos.degradeLevel)) +
      summaryItem('QoS Updated', fmtValue(qos.lastUpdatedMs || peer.qosLastUpdatedMs)) +
      summaryItem('DL Updated', fmtValue(peer.downlinkLastUpdatedMs)) +
      summaryItem('DL Age', fmtMs(peer.downlinkAgeMs)) +
      summaryItem('Recv Transport', peer.recvTransport ? 'present' : 'none') +
      '</div>' +
      '</div>'
    );
  }

  function renderPeerOverviewSection() {
    if (!state.latestStatsReport || !Array.isArray(state.latestStatsReport.peers)) {
      return '<div class="qos-section"><h3>Room Peers</h3><div class="hint">Waiting for statsReport notification.</div></div>';
    }

    return (
      '<div class="qos-section">' +
      '<h3>Room Peers</h3>' +
      state.latestStatsReport.peers.map(renderPeerOverviewCard).join('') +
      '</div>'
    );
  }

  function renderQosPanel() {
    syncLocalVideoEntries();
    renderLocalPreviewCards();
    const sections = [
      renderOverviewSection(),
      renderTransportDebugSection(),
      renderPeerOverviewSection(),
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
    renderLocalPreviewCards([]);
    renderQosPanel();
    updateControls();

    if (!window.mediasoupClient) {
      setStatus('mediasoup client bundle missing', 'err');
      log('mediasoup client bundle did not load');
      return;
    }

    log(useLegacyQos ? 'Demo started in legacy QoS mode' : 'Demo started with new QoS library');
  }

  init();
})();
