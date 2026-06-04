(function () {
  'use strict';

  const REQUEST_TIMEOUT_MS = 10000;
  const ICE_GRACE_MS = 10000;

  function randomPeerId() {
    return `web-${Date.now().toString(36)}-${Math.random().toString(36).slice(2, 8)}`;
  }

  function noop() {}

  function isNotFoundError(error) {
    const message = String(error && error.message ? error.message : error || '');
    return /room not found|peer not found|transport not found/i.test(message);
  }

  function normalizeMediaConfig(config, defaults) {
    if (!config) return null;
    if (config === true) return { ...defaults };
    if (config instanceof MediaStreamTrack) return { ...defaults, track: config };
    if (config.track instanceof MediaStreamTrack) {
      return { ...defaults, ...config };
    }
    return { ...defaults, ...config };
  }

  function stopStream(stream) {
    if (!stream) return;
    for (const track of stream.getTracks()) {
      try { track.stop(); } catch {}
    }
  }

  class MediasoupRoomClient {
    constructor(options) {
      if (!options || !options.roomId || !(options.wsUrl || options.wssUrl)) {
        throw new Error('roomId and wsUrl/wssUrl are required');
      }
      if (!window.mediasoupClient || !window.mediasoupClient.Device) {
        throw new Error('mediasoup-client bundle is required');
      }

      this.roomId = options.roomId;
      this.wsUrl = options.wsUrl || options.wssUrl;
      this.peerId = options.peerId || randomPeerId();
      this.displayName = options.displayName || this.peerId;
      this.audioRole = options.audioRole || 'normal';
      this.initialRtpCapabilities = options.rtpCapabilities || null;

      this.onTrack = options.onTrack || noop;
      this.onTrackClosed = options.onTrackClosed || noop;
      this.onPeerJoined = options.onPeerJoined || noop;
      this.onPeerLeft = options.onPeerLeft || noop;
      this.onPeersChanged = options.onPeersChanged || noop;
      this.onStateChange = options.onStateChange || noop;
      this.onLog = options.onLog || noop;

      this.ws = null;
      this.reqId = 0;
      this.pending = new Map();
      this.device = null;
      this.recvTransport = null;
      this.sendTransport = null;
      this.consumers = new Map();
      this.producers = new Map();
      this.peers = new Map();
      this.pendingConsumers = [];
      this.closed = false;
      this.reconnectTimer = null;
      this.reconnectInFlight = null;
      this.signalingRecoveryInFlight = false;
      this.signalingSocketInvalid = false;
      this.iceState = new Map();
      this.listeners = new Map();
    }

    async start() {
      this.closed = false;
      await this.connectAndJoin({ rebuildMedia: true });
      this.emitState('connected');
      return this;
    }

    async join() {
      return this.start();
    }

    async close() {
      this.closed = true;
      if (this.reconnectTimer) clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
      this.failPendingRequests('client closed');
      this.closeTransportsAndMedia();
      if (this.ws) {
        try { this.ws.close(); } catch {}
      }
      this.ws = null;
      this.signalingSocketInvalid = false;
      this.emitState('closed');
    }

    async leave() {
      return this.close();
    }

    on(event, listener) {
      if (typeof listener !== 'function') return () => {};
      const listeners = this.listeners.get(event) || new Set();
      listeners.add(listener);
      this.listeners.set(event, listeners);
      return () => this.off(event, listener);
    }

    off(event, listener) {
      const listeners = this.listeners.get(event);
      if (!listeners) return;
      listeners.delete(listener);
      if (listeners.size === 0) this.listeners.delete(event);
    }

    emit(event, payload) {
      const listeners = this.listeners.get(event);
      if (!listeners) return;
      for (const listener of Array.from(listeners)) {
        try {
          listener(payload);
        } catch (error) {
          this.log('listener error', { event, error: error.message });
        }
      }
    }

    emitError(error, context) {
      const payload = {
        error,
        message: error?.message || String(error || 'unknown error'),
        context: context || null,
        ts: Date.now(),
      };
      this.emit('error', payload);
      this.log('sdk error', { message: payload.message, context: payload.context });
    }

    log(message, data) {
      const event = { message, data: data || null, ts: Date.now() };
      this.onLog(event);
      this.emit('log', event);
    }

    emitState(state, data) {
      const event = { state, data: data || null, ts: Date.now() };
      this.onStateChange(event);
      this.emit('networkState', event);
      this.emit('stateChange', event);
    }

    async connectAndJoin({ rebuildMedia, forceNewSocket = false } = {}) {
      await this.connectWs({ forceNew: forceNewSocket });
      const join = await this.request('join', {
        roomId: this.roomId,
        peerId: this.peerId,
        displayName: this.displayName,
        audioRole: this.audioRole,
        rtpCapabilities: this.device ? this.device.rtpCapabilities : this.initialRtpCapabilities,
      });

      this.audioRole = join.audioRole || this.audioRole;
      this.rebuildPeers(join.participants || []);
      this.emitState('joined', {
        joinMode: join.joinMode || 'new-peer',
        audioRole: this.audioRole,
        participantCount: Array.isArray(join.participants) ? join.participants.length : 0,
      });

      if (!this.device) {
        this.device = new window.mediasoupClient.Device();
        await this.device.load({ routerRtpCapabilities: join.routerRtpCapabilities });
      }

      const joinMode = join.joinMode || 'new-peer';
      if (rebuildMedia || joinMode === 'new-peer' || !this.recvTransport) {
        this.closeRecvSide();
        const precreatedIds = await this.createRecvTransport();
        await this.consumeInitial(join, precreatedIds);
        return join;
      }

      if (joinMode === 'replaced-session') {
        this.emitState('session-restored', {
          joinMode,
          reusedRecvTransport: Boolean(this.recvTransport),
        });
        if (this.transportConnected(this.recvTransport)) {
          await this.consumeInitial(join, new Set());
        } else {
          await this.restartIceForTransport(this.recvTransport);
        }
      }

      return join;
    }

    connectWs({ forceNew = false } = {}) {
      if (!forceNew && this.ws && this.ws.readyState === WebSocket.OPEN && !this.signalingSocketInvalid) {
        return Promise.resolve();
      }

      return new Promise((resolve, reject) => {
        const ws = new WebSocket(this.wsUrl);
        let settled = false;
        const timer = setTimeout(() => {
          if (!settled) {
            settled = true;
            try { ws.close(); } catch {}
            reject(new Error('websocket connect timeout'));
          }
        }, REQUEST_TIMEOUT_MS);

        ws.onopen = () => {
          if (settled) return;
          settled = true;
          clearTimeout(timer);
          this.ws = ws;
          this.signalingSocketInvalid = false;
          ws.onmessage = event => this.handleMessage(event);
          ws.onclose = () => {
            if (this.ws !== ws) return;
            this.ws = null;
            this.signalingSocketInvalid = false;
            this.failPendingRequests('websocket closed');
            if (!this.closed) this.scheduleReconnect();
          };
          ws.onerror = () => {
            if (!settled) {
              settled = true;
              clearTimeout(timer);
              reject(new Error('websocket connect failed'));
            }
          };
          resolve();
        };

        ws.onerror = () => {
          if (settled) return;
          settled = true;
          clearTimeout(timer);
          reject(new Error('websocket connect failed'));
        };
      });
    }

    scheduleReconnect(reason = 'websocket-close') {
      if (this.reconnectTimer || this.reconnectInFlight || this.closed) return;
      this.emitState('reconnecting');
      this.reconnectTimer = setTimeout(() => {
        this.reconnectTimer = null;
        this.reconnectInFlight = this.connectAndJoin({ rebuildMedia: false })
          .then(() => this.emitState('connected'))
          .catch(error => {
            this.log('reconnect failed', { error: error.message });
            this.emitError(error, { phase: 'reconnect' });
            this.emitState('reconnecting', { error: error.message, reason });
            this.reconnectInFlight = null;
            this.scheduleReconnect(reason);
          })
          .finally(() => {
            this.reconnectInFlight = null;
          });
      }, 1000);
    }

    triggerSignalingRecovery(reason, data) {
      if (this.closed || this.signalingRecoveryInFlight || this.reconnectInFlight) return;
      this.signalingRecoveryInFlight = true;
      const oldWs = this.ws;
      this.signalingSocketInvalid = true;
      this.ws = null;
      try { oldWs?.close?.(); } catch {}
      this.failPendingRequests(`signaling recovery: ${reason}`);
      this.emitState('reconnecting', { reason, ...(data || {}) });
      void this.connectAndJoin({ rebuildMedia: false, forceNewSocket: true })
        .then(() => {
          this.emitState('connected', { reason });
        })
        .catch(error => {
          this.log('signaling recovery failed', { reason, error: error.message });
          this.emitError(error, { phase: 'signaling-recovery', reason, ...(data || {}) });
          this.scheduleReconnect(reason);
        })
        .finally(() => {
          this.signalingRecoveryInFlight = false;
        });
    }

    request(method, data) {
      if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
        return Promise.reject(new Error('websocket is not connected'));
      }
      const id = ++this.reqId;
      return new Promise((resolve, reject) => {
        const timer = setTimeout(() => {
          this.pending.delete(id);
          if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.triggerSignalingRecovery('request-timeout', { method });
          }
          reject(new Error(`request timeout: ${method}`));
        }, REQUEST_TIMEOUT_MS);

        this.pending.set(id, {
          resolve: value => {
            clearTimeout(timer);
            resolve(value);
          },
          reject: error => {
            clearTimeout(timer);
            reject(error);
          },
        });

        this.ws.send(JSON.stringify({ request: true, id, method, data: data || {} }));
      });
    }

    failPendingRequests(reason) {
      for (const [id, pending] of this.pending) {
        pending.reject(new Error(reason));
        this.pending.delete(id);
      }
    }

    handleMessage(event) {
      const message = JSON.parse(event.data);
      if (message.response === true) {
        const pending = this.pending.get(message.id);
        if (!pending) return;
        this.pending.delete(message.id);
        if (message.ok) {
          pending.resolve(message.data || {});
        } else {
          const error = new Error(message.error || 'request failed');
          error.data = message.data || {};
          pending.reject(error);
        }
        return;
      }

      if (message.notification !== true) return;
      this.handleNotification(message.method, message.data || {});
    }

    handleNotification(method, data) {
      if (method === 'newConsumer') {
        void this.handleNewConsumer(data);
        return;
      }
      if (method === 'consumerClosed') {
        this.closeConsumer(data.consumerId || data.id || '');
        return;
      }
      if (method === 'producerLeft') {
        const ids = Array.isArray(data.consumerIds) ? data.consumerIds : [];
        ids.forEach(id => this.closeConsumer(id));
        if (data.producerId) {
          this.closeConsumersWhere(info => info.producerId === data.producerId);
        } else if (data.peerId) {
          this.closeConsumersWhere(info => info.peerId === data.peerId);
        }
        return;
      }
      if (method === 'peerJoined') {
        this.upsertPeer(data);
        this.onPeerJoined(data);
        this.emit('peerJoined', data);
        return;
      }
      if (method === 'peerLeft') {
        this.peers.delete(data.peerId);
        this.closeConsumersWhere(info => info.peerId === data.peerId);
        this.onPeerLeft(data);
        this.emit('peerLeft', data);
        this.onPeersChanged(this.getPeers());
      }
    }

    rebuildPeers(participants) {
      this.peers.clear();
      participants.forEach(peer => this.upsertPeer(peer));
      this.onPeersChanged(this.getPeers());
      this.emit('peersChanged', this.getPeers());
    }

    upsertPeer(peer) {
      if (!peer || !peer.peerId) return;
      const previous = this.peers.get(peer.peerId) || {};
      this.peers.set(peer.peerId, { ...previous, ...peer });
      this.onPeersChanged(this.getPeers());
      this.emit('peersChanged', this.getPeers());
    }

    getPeers() {
      return Array.from(this.peers.values());
    }

    async createRecvTransport() {
      const data = await this.request('createWebRtcTransport', {
        producing: false,
        consuming: true,
        rtpCapabilities: this.device.rtpCapabilities,
      });
      this.recvTransport = this.device.createRecvTransport(data);
      this.bindTransportRecovery(this.recvTransport);
      this.recvTransport.on('connect', async ({ dtlsParameters }, callback, errback) => {
        try {
          await this.request('connectWebRtcTransport', {
            transportId: this.recvTransport.id,
            dtlsParameters,
          });
          callback();
        } catch (error) {
          errback(error);
        }
      });

      const consumers = Array.isArray(data.consumers) ? data.consumers : [];
      const precreatedIds = new Set();
      for (const consumer of consumers) {
        if (consumer.producerId) precreatedIds.add(consumer.producerId);
        await this.handleNewConsumer(consumer);
      }
      return precreatedIds;
    }

    async ensureSendTransport() {
      if (this.sendTransport) return this.sendTransport;
      const data = await this.request('createWebRtcTransport', {
        producing: true,
        consuming: false,
      });
      this.sendTransport = this.device.createSendTransport(data);
      this.bindTransportRecovery(this.sendTransport);
      this.sendTransport.on('connect', async ({ dtlsParameters }, callback, errback) => {
        try {
          await this.request('connectWebRtcTransport', {
            transportId: this.sendTransport.id,
            dtlsParameters,
          });
          callback();
        } catch (error) {
          errback(error);
        }
      });
      this.sendTransport.on('produce', async ({ kind, rtpParameters, appData }, callback, errback) => {
        try {
          const response = await this.request('produce', {
            transportId: this.sendTransport.id,
            kind,
            rtpParameters,
            appData: appData || {},
          });
          callback({ id: response.id });
        } catch (error) {
          errback(error);
        }
      });
      return this.sendTransport;
    }

    async consumeInitial(join, precreatedIds) {
      for (const item of this.pendingConsumers.splice(0)) {
        await this.handleNewConsumer(item);
        if (item.producerId) precreatedIds.add(item.producerId);
      }
      for (const item of Array.isArray(join.existingProducers) ? join.existingProducers : []) {
        if (precreatedIds.has(item.producerId)) continue;
        await this.consumeProducer(item.producerId, item.kind);
      }
    }

    async consumeProducer(producerId, kind) {
      if (!producerId || !this.recvTransport) return;
      const response = await this.request('consume', {
        transportId: this.recvTransport.id,
        producerId,
        rtpCapabilities: this.device.rtpCapabilities,
      });
      await this.handleNewConsumer({
        id: response.id,
        producerId: response.producerId || producerId,
        kind: response.kind || kind,
        rtpParameters: response.rtpParameters,
        peerId: response.peerId,
        appData: response.appData || {},
        producerPaused: response.producerPaused,
      });
    }

    async handleNewConsumer(data) {
      if (!this.recvTransport) {
        this.pendingConsumers.push(data);
        return;
      }
      if (this.consumers.has(data.id)) return;

      const consumer = await this.recvTransport.consume({
        id: data.id,
        producerId: data.producerId,
        kind: data.kind,
        rtpParameters: data.rtpParameters,
        appData: data.appData || {},
      });
      const stream = new MediaStream([consumer.track]);
      const info = {
        peerId: data.peerId || data.producerPeerId || '',
        displayName: this.peers.get(data.peerId || data.producerPeerId || '')?.displayName || '',
        producerId: data.producerId,
        consumerId: consumer.id,
        kind: consumer.kind,
        appData: data.appData || {},
        source: data.appData?.source || '',
        track: consumer.track,
        stream,
        consumer,
      };
      this.consumers.set(consumer.id, info);
      consumer.on('transportclose', () => this.closeConsumer(consumer.id));
      consumer.on('producerclose', () => this.closeConsumer(consumer.id));
      if (consumer.track) {
        consumer.track.addEventListener('ended', () => this.closeConsumer(consumer.id));
      }
      this.onTrack(info);
      this.emit('track', info);
    }

    closeConsumer(consumerId) {
      const info = this.consumers.get(consumerId);
      if (!info) return;
      this.consumers.delete(consumerId);
      try { info.consumer.close(); } catch {}
      this.onTrackClosed(info);
      this.emit('trackClosed', info);
    }

    closeConsumersWhere(predicate) {
      for (const [consumerId, info] of Array.from(this.consumers.entries())) {
        if (predicate(info)) this.closeConsumer(consumerId);
      }
    }

    async produceTrack(track, appData, options = {}) {
      const transport = await this.ensureSendTransport();
      const producer = await transport.produce({
        track,
        codec: options.codec,
        appData: appData || {},
      });
      const info = {
        producer,
        producerId: producer.id,
        kind: producer.kind,
        source: producer.appData?.source || appData?.source || '',
        appData: producer.appData || appData || {},
        stream: options.stream || null,
        ownedStream: Boolean(options.ownedStream),
        track,
      };
      this.producers.set(producer.id, info);
      producer.on('transportclose', () => this.forgetProducer(producer.id, { stopTrack: false, closeProducer: false }));
      producer.on('trackended', () => this.forgetProducer(producer.id, { stopTrack: false, closeProducer: false }));
      return producer;
    }

    async produceAudio(track, appData) {
      return this.produceTrack(track, appData || { source: 'audio' });
    }

    async publish(config = {}) {
      const published = [];
      const ownedStreams = [];
      try {
        const audioConfig = normalizeMediaConfig(config.audio, { appData: { source: 'audio' } });
        const videoConfig = normalizeMediaConfig(config.video, { appData: { source: 'camera' } });
        const screenConfig = normalizeMediaConfig(config.screen, { appData: { source: 'screenShare' } });

        if (audioConfig) {
          let track = audioConfig.track;
          let stream = audioConfig.stream || null;
          let ownedStream = false;
          if (!track) {
            stream = await navigator.mediaDevices.getUserMedia({ audio: audioConfig.constraints || true, video: false });
            ownedStream = true;
            ownedStreams.push(stream);
            track = stream.getAudioTracks()[0];
          }
          if (!track) throw new Error('no audio track available');
          const producer = await this.produceTrack(track, audioConfig.appData || { source: 'audio' }, {
            stream,
            ownedStream,
            codec: audioConfig.codec,
          });
          published.push(producer);
        }

        const publishVideoTrack = async (mediaConfig, capture) => {
          let track = mediaConfig.track;
          let stream = mediaConfig.stream || null;
          let ownedStream = false;
          if (!track) {
            stream = await capture();
            ownedStream = true;
            ownedStreams.push(stream);
            track = stream.getVideoTracks()[0];
          }
          if (!track) throw new Error('no video track available');
          const producer = await this.produceTrack(track, mediaConfig.appData, {
            stream,
            ownedStream,
            codec: mediaConfig.codec,
          });
          published.push(producer);
        };

        if (videoConfig) {
          await publishVideoTrack(videoConfig, () => navigator.mediaDevices.getUserMedia({
            audio: false,
            video: videoConfig.constraints || true,
          }));
        }

        if (screenConfig) {
          if (!navigator.mediaDevices.getDisplayMedia && !screenConfig.track) {
            throw new Error('getDisplayMedia is not available');
          }
          await publishVideoTrack(screenConfig, () => navigator.mediaDevices.getDisplayMedia({
            audio: false,
            video: screenConfig.constraints || true,
          }));
        }

        return published;
      } catch (error) {
        for (const producer of published) {
          await this.closeProducer(producer.id).catch(() => {});
        }
        ownedStreams.forEach(stopStream);
        throw error;
      }
    }

    forgetProducer(producerId, options = {}) {
      const info = this.producers.get(producerId);
      if (!info) return;
      this.producers.delete(producerId);
      if (options.closeProducer !== false) {
        try { info.producer.close(); } catch {}
      }
      if (options.stopTrack !== false) {
        try { info.track?.stop?.(); } catch {}
      }
      if (info.ownedStream) stopStream(info.stream);
    }

    async closeProducer(producerId) {
      if (!producerId) return;
      await this.request('closeProducer', { producerId }).catch(() => {});
      this.forgetProducer(producerId);
    }

    async unpublish({ producerId } = {}) {
      if (producerId) {
        await this.closeProducer(producerId);
        return;
      }
      for (const id of Array.from(this.producers.keys())) {
        await this.closeProducer(id);
      }
    }

    bindTransportRecovery(transport) {
      const state = { timer: null, inFlight: false };
      this.iceState.set(transport.id, state);
      transport.on('connectionstatechange', connectionState => {
        this.emitState('transport-state', { transportId: transport.id, connectionState });
        if (connectionState === 'connected') {
          if (state.timer) clearTimeout(state.timer);
          state.timer = null;
          state.inFlight = false;
          return;
        }
        if (connectionState === 'disconnected') {
          if (state.timer || state.inFlight) return;
          state.timer = setTimeout(() => {
            state.timer = null;
            if (transport.connectionState === 'connected' || state.inFlight) return;
            void this.restartIceForTransport(transport);
          }, ICE_GRACE_MS);
          return;
        }
        if (connectionState === 'failed') {
          if (state.timer) clearTimeout(state.timer);
          state.timer = null;
          if (!state.inFlight) void this.restartIceForTransport(transport);
        }
      });
    }

    async restartIceForTransport(transport) {
      const state = this.iceState.get(transport.id) || { inFlight: false };
      if (state.inFlight) return;
      state.inFlight = true;
      this.iceState.set(transport.id, state);
      try {
        const response = await this.request('restartIce', { transportId: transport.id });
        if (!response.iceParameters) throw new Error('restartIce missing iceParameters');
        await transport.restartIce({ iceParameters: response.iceParameters });
      } catch (error) {
        if (isNotFoundError(error)) {
          await this.rebuildAfterNewPeer();
          return;
        }
        this.emitError(error, { phase: 'restartIce', transportId: transport.id });
        throw error;
      } finally {
        state.inFlight = false;
      }
    }

    transportConnected(transport) {
      return transport && transport.connectionState === 'connected';
    }

    async rebuildAfterNewPeer() {
      this.closeTransportsAndMedia();
      await this.connectAndJoin({ rebuildMedia: true });
    }

    closeRecvSide() {
      for (const id of Array.from(this.consumers.keys())) this.closeConsumer(id);
      if (this.recvTransport) {
        try { this.recvTransport.close(); } catch {}
      }
      this.recvTransport = null;
    }

    closeTransportsAndMedia() {
      this.closeRecvSide();
      for (const id of Array.from(this.producers.keys())) this.forgetProducer(id);
      if (this.sendTransport) {
        try { this.sendTransport.close(); } catch {}
      }
      this.sendTransport = null;
    }
  }

  class TalkbackClient {
    constructor(roomClient, options) {
      this.room = roomClient;
      this.onTargetsChanged = options?.onTargetsChanged || noop;
      this.onStateChange = options?.onStateChange || noop;
      this.targetPeerId = '';
      this.audioProducer = null;
      this.audioStream = null;

      const oldPeerJoined = this.room.onPeerJoined;
      const oldPeerLeft = this.room.onPeerLeft;
      const oldPeersChanged = this.room.onPeersChanged;
      this.room.onPeerJoined = peer => {
        oldPeerJoined(peer);
        this.emitTargets();
        if (this.targetPeerId && peer.peerId === this.targetPeerId && this.audioProducer) {
          void this.claim(this.targetPeerId).catch(error => this.emitState('error', error));
        }
      };
      this.room.onPeerLeft = peer => {
        oldPeerLeft(peer);
        this.emitTargets();
      };
      this.room.onPeersChanged = peers => {
        oldPeersChanged(peers);
        this.emitTargets();
      };
      this.emitTargets();
    }

    getTargets() {
      return this.room.getPeers()
        .filter(peer => peer.peerId !== this.room.peerId && peer.audioRole === 'audio-restricted')
        .map(peer => ({
          peerId: peer.peerId,
          displayName: peer.displayName || peer.peerId,
          audioRole: peer.audioRole,
        }));
    }

    emitTargets() {
      this.onTargetsChanged(this.getTargets());
    }

    emitState(state, data) {
      this.onStateChange({ state, data: data || null, targetPeerId: this.targetPeerId, ts: Date.now() });
    }

    async claim(targetPeerId) {
      const response = await this.room.request('claimAudioRestrictedSlot', { targetPeerId });
      if (response.required === true && response.claimed !== true && response.alreadyOwned !== true) {
        throw new Error(response.reason || 'claim failed');
      }
      this.targetPeerId = targetPeerId;
      return response;
    }

    async openTalkTo(targetPeerId) {
      if (!targetPeerId) throw new Error('targetPeerId is required');
      this.emitState('opening', { targetPeerId });
      let claimed = false;
      try {
        await this.claim(targetPeerId);
        claimed = true;
        const producers = await this.room.publish({
          audio: {
            appData: { source: 'talkback', targetPeerId },
          },
        });
        this.audioProducer = producers.find(producer => producer.kind === 'audio') || null;
        const producerInfo = this.audioProducer ? this.room.producers.get(this.audioProducer.id) : null;
        this.audioStream = producerInfo?.stream || null;
        if (!this.audioProducer) throw new Error('no talkback audio producer available');
        this.emitState('opened', { targetPeerId, producerId: this.audioProducer.id });
      } catch (error) {
        if (claimed) {
          await this.room.request('releaseAudioRestrictedSlot', { targetPeerId }).catch(() => {});
        }
        this.stopLocalAudio();
        this.targetPeerId = '';
        this.emitState('error', error);
        throw error;
      }
    }

    async closeTalk() {
      const targetPeerId = this.targetPeerId;
      this.emitState('closing', { targetPeerId });
      if (this.audioProducer) {
        await this.room.unpublish({ producerId: this.audioProducer.id });
        this.audioProducer = null;
      }
      if (targetPeerId) {
        await this.room.request('releaseAudioRestrictedSlot', { targetPeerId }).catch(() => {});
      }
      this.stopLocalAudio();
      this.targetPeerId = '';
      this.emitState('idle');
    }

    stopLocalAudio() {
      if (this.audioStream) {
        this.audioStream.getTracks().forEach(track => track.stop());
      }
      this.audioStream = null;
    }

    async openMic(targetPeerId) {
      return this.openTalkTo(targetPeerId || this.targetPeerId);
    }

    async closeMic() {
      return this.closeTalk();
    }

    async release() {
      if (!this.targetPeerId) return;
      await this.room.request('releaseAudioRestrictedSlot', { targetPeerId: this.targetPeerId }).catch(() => {});
      this.targetPeerId = '';
      this.emitState('idle');
    }
  }

  MediasoupRoomClient.version = '0.1.0';
  TalkbackClient.version = MediasoupRoomClient.version;

  window.MediasoupRoomClient = MediasoupRoomClient;
  window.TalkbackClient = TalkbackClient;
  window.MediasoupAudioTalk = TalkbackClient;
})();
