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

  class MediasoupRoomClient {
    constructor(options) {
      if (!options || !options.roomId || !options.wsUrl) {
        throw new Error('roomId and wsUrl are required');
      }
      if (!window.mediasoupClient || !window.mediasoupClient.Device) {
        throw new Error('mediasoup-client bundle is required');
      }

      this.roomId = options.roomId;
      this.wsUrl = options.wsUrl;
      this.peerId = options.peerId || randomPeerId();
      this.displayName = options.displayName || this.peerId;
      this.audioRole = options.audioRole || 'normal';

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
      this.iceState = new Map();
    }

    async start() {
      this.closed = false;
      await this.connectAndJoin({ rebuildMedia: true });
      this.emitState('connected');
      return this;
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
      this.emitState('closed');
    }

    log(message, data) {
      this.onLog({ message, data: data || null, ts: Date.now() });
    }

    emitState(state, data) {
      this.onStateChange({ state, data: data || null, ts: Date.now() });
    }

    async connectAndJoin({ rebuildMedia }) {
      await this.connectWs();
      const join = await this.request('join', {
        roomId: this.roomId,
        peerId: this.peerId,
        displayName: this.displayName,
        audioRole: this.audioRole,
        rtpCapabilities: this.device ? this.device.rtpCapabilities : undefined,
      });

      this.audioRole = join.audioRole || this.audioRole;
      this.rebuildPeers(join.participants || []);

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
        if (this.transportConnected(this.recvTransport)) {
          await this.consumeInitial(join, new Set());
        } else {
          await this.restartIceForTransport(this.recvTransport);
        }
      }

      return join;
    }

    connectWs() {
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
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
          ws.onmessage = event => this.handleMessage(event);
          ws.onclose = () => {
            if (this.ws !== ws) return;
            this.ws = null;
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

    scheduleReconnect() {
      if (this.reconnectTimer || this.reconnectInFlight || this.closed) return;
      this.emitState('reconnecting');
      this.reconnectTimer = setTimeout(() => {
        this.reconnectTimer = null;
        this.reconnectInFlight = this.connectAndJoin({ rebuildMedia: false })
          .then(() => this.emitState('connected'))
          .catch(error => {
            this.log('reconnect failed', { error: error.message });
            this.emitState('reconnecting', { error: error.message });
            this.reconnectInFlight = null;
            this.scheduleReconnect();
          })
          .finally(() => {
            this.reconnectInFlight = null;
          });
      }, 1000);
    }

    request(method, data) {
      if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
        return Promise.reject(new Error('websocket is not connected'));
      }
      const id = ++this.reqId;
      return new Promise((resolve, reject) => {
        const timer = setTimeout(() => {
          this.pending.delete(id);
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
        return;
      }
      if (method === 'peerLeft') {
        this.peers.delete(data.peerId);
        this.closeConsumersWhere(info => info.peerId === data.peerId);
        this.onPeerLeft(data);
        this.onPeersChanged(this.getPeers());
      }
    }

    rebuildPeers(participants) {
      this.peers.clear();
      participants.forEach(peer => this.upsertPeer(peer));
      this.onPeersChanged(this.getPeers());
    }

    upsertPeer(peer) {
      if (!peer || !peer.peerId) return;
      const previous = this.peers.get(peer.peerId) || {};
      this.peers.set(peer.peerId, { ...previous, ...peer });
      this.onPeersChanged(this.getPeers());
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
    }

    closeConsumer(consumerId) {
      const info = this.consumers.get(consumerId);
      if (!info) return;
      this.consumers.delete(consumerId);
      try { info.consumer.close(); } catch {}
      this.onTrackClosed(info);
    }

    closeConsumersWhere(predicate) {
      for (const [consumerId, info] of Array.from(this.consumers.entries())) {
        if (predicate(info)) this.closeConsumer(consumerId);
      }
    }

    async produceAudio(track, appData) {
      const transport = await this.ensureSendTransport();
      const producer = await transport.produce({
        track,
        appData: appData || { source: 'audio' },
      });
      this.producers.set(producer.id, producer);
      producer.on('transportclose', () => this.producers.delete(producer.id));
      return producer;
    }

    async closeProducer(producerId) {
      if (!producerId) return;
      await this.request('closeProducer', { producerId }).catch(() => {});
      const producer = this.producers.get(producerId);
      if (producer) {
        try { producer.close(); } catch {}
        this.producers.delete(producerId);
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
      for (const producer of this.producers.values()) {
        try { producer.close(); } catch {}
      }
      this.producers.clear();
      if (this.sendTransport) {
        try { this.sendTransport.close(); } catch {}
      }
      this.sendTransport = null;
    }
  }

  class MediasoupAudioTalk {
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
        this.audioStream = await navigator.mediaDevices.getUserMedia({ audio: true, video: false });
        const track = this.audioStream.getAudioTracks()[0];
        if (!track) throw new Error('no microphone track available');
        this.audioProducer = await this.room.produceAudio(track, { source: 'audio', targetPeerId });
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
        await this.room.closeProducer(this.audioProducer.id);
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
  }

  window.MediasoupRoomClient = MediasoupRoomClient;
  window.MediasoupAudioTalk = MediasoupAudioTalk;
})();
