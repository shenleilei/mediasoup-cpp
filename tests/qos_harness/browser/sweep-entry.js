import * as qos from '../../../src/client/lib/qos/index.js';

// Synthetic stats provider used to validate controller behavior under
// controlled inputs. This is intentionally not a real RTP/netem harness.
class SimulatedStatsProvider {
  constructor() {
    this._bytesSent = 0;
    this._packetsSent = 0;
    this._packetsLost = 0;
    this._ts = Date.now();
    // default: healthy network
    this.config = {
      bitrateBps: 800000,
      targetBitrateBps: 900000,
      lossRate: 0,
      rttMs: 1,
      jitterMs: 0.5,
      qualityLimitationReason: 'none',
    };
  }

  setCondition(config) {
    this.config = { ...this.config, ...config };
  }

  async getSnapshot() {
    const now = Date.now();
    const dtMs = now - this._ts;
    this._ts = now;

    const bytesThisPeriod = Math.round((this.config.bitrateBps / 8) * (dtMs / 1000));
    const packetsThisPeriod = Math.max(1, Math.round(bytesThisPeriod / 1200));
    const lostThisPeriod = Math.round(packetsThisPeriod * this.config.lossRate);

    this._bytesSent += bytesThisPeriod;
    this._packetsSent += packetsThisPeriod;
    this._packetsLost += lostThisPeriod;

    return {
      timestampMs: now,
      bytesSent: this._bytesSent,
      packetsSent: this._packetsSent,
      packetsLost: this._packetsLost,
      targetBitrateBps: this.config.targetBitrateBps,
      configuredBitrateBps: 900000,
      roundTripTimeMs: this.config.rttMs,
      jitterMs: this.config.jitterMs,
      qualityLimitationReason: this.config.lossRate > 0.05 || this.config.rttMs > 150 ? 'bandwidth' : this.config.qualityLimitationReason,
      frameWidth: 640,
      frameHeight: 360,
      framesPerSecond: 24,
    };
  }
}

function createHarness() {
  const statsProvider = new SimulatedStatsProvider();
  const profile = qos.getDefaultCameraProfile();

  const executedActions = [];
  const actionExecutor = new qos.QosActionExecutor({
    async execute(action) {
      executedActions.push({ type: action.type, level: action.level, tsMs: Date.now() });
      return { applied: true };
    },
  });

  const controller = new qos.PublisherQosController({
    clock: new qos.SystemQosClock(),
    profile,
    statsProvider,
    actionExecutor,
    trackId: 'sim-camera',
    kind: 'video',
    producerId: 'sim-producer',
    sampleIntervalMs: 500,
    snapshotIntervalMs: 5000,
    allowAudioOnly: true,
    allowVideoPause: true,
  });

  controller.start();
  return { controller, statsProvider, executedActions };
}

function getState(controller) {
  return {
    state: controller.state,
    level: controller.level,
    trace: controller.getTraceEntries().map(e => ({
      tsMs: e.tsMs,
      stateBefore: e.stateBefore,
      stateAfter: e.stateAfter,
      signals: e.signals,
      plannedAction: e.plannedAction,
    })),
  };
}

let harness = null;

window.__qosSyntheticSweepHarness = {
  init() {
    harness = createHarness();
  },
  setCondition(config) {
    harness.statsProvider.setCondition(config);
  },
  getState() {
    return getState(harness.controller);
  },
  getActions() {
    return harness.executedActions;
  },
  stop() {
    if (harness) harness.controller.stop();
  },
};
