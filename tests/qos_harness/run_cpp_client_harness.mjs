import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import { createCppClientHarness, ensureHarnessMp4, ensureHarnessAvMp4, sleep } from './cpp_client_runner.mjs';
import { WsJsonClient, joinRoom } from './ws_json_client.mjs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const host = '127.0.0.1';
const port = Number(process.env.QOS_CPP_CLIENT_HARNESS_PORT) || 14020;

function loadScenario(name) {
  const filePath = path.join(__dirname, 'scenarios', `${name}.json`);
  return JSON.parse(fs.readFileSync(filePath, 'utf8'));
}

function buildTrackSignals(snapshot = {}) {
  const sent = Number(snapshot.packetsSent ?? 0);
  const lost = Number(snapshot.packetsLost ?? 0);
  const total = sent + lost;
  return {
    sendBitrateBps: snapshot.targetBitrateBps ?? snapshot.configuredBitrateBps ?? 900000,
    targetBitrateBps: snapshot.targetBitrateBps ?? snapshot.configuredBitrateBps ?? 900000,
    lossRate: total > 0 ? lost / total : 0,
    rttMs: snapshot.roundTripTimeMs ?? 120,
    jitterMs: snapshot.jitterMs ?? 8,
    frameWidth: snapshot.frameWidth,
    frameHeight: snapshot.frameHeight,
    framesPerSecond: snapshot.framesPerSecond,
    qualityLimitationReason: snapshot.qualityLimitationReason ?? 'none',
  };
}

function buildRawClientSnapshotFromSenderSnapshot(snapshot, scenario, seq) {
  return {
    schema: 'mediasoup.qos.client.v1',
    seq,
    tsMs: snapshot.timestampMs ?? Date.now(),
    peerState: {
      mode: 'audio-video',
      quality: 'excellent',
      stale: false,
    },
    tracks: [
      {
        localTrackId: scenario.trackId,
        producerId: 'cpp-test-producer',
        kind: scenario.kind,
        source: scenario.source,
        state: 'stable',
        reason: 'network',
        quality: 'excellent',
        ladderLevel: 0,
        signals: buildTrackSignals(snapshot),
      },
    ],
  };
}

function buildPublishSnapshotPayloads(scenario) {
  return scenario.snapshots.map((snapshot, index) => ({
    delayMs: index === 0 ? 500 : 300,
    payload: buildRawClientSnapshotFromSenderSnapshot(snapshot, scenario, index + 1),
  }));
}

function buildStaleSeqPayloads(scenario) {
  return [
    { delayMs: 500, payload: scenario.newer },
    { delayMs: 300, payload: scenario.older },
  ];
}

function buildAutoOverridePayloads(scenario) {
  return [
    { delayMs: 800, payload: scenario.snapshot },
  ];
}

function buildThreadedVideoSources(trackCount, mediaPath = ensureHarnessMp4()) {
  return Array.from({ length: trackCount }, () => mediaPath).join(',');
}

async function waitForLog(harness, matcher, timeoutMs = 5000) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    const logs = harness.getLogs();
    const lines = [...logs.clientStdout, ...logs.clientStderr];
    const matched = lines.find(line => {
      if (matcher instanceof RegExp) return matcher.test(line);
      return line.includes(matcher);
    });
    if (matched) return matched;
    await sleep(100);
  }
  return null;
}

async function createHarnessForScenario(scenarioName, scenario, extraEnv = {}) {
  return createCppClientHarness({
    signalingPort: port,
    roomId: scenario.roomId,
    peerId: scenario.peerId,
    mp4Path: ensureHarnessMp4(),
    extraEnv,
  });
}

async function withAdminClient(roomId, fn) {
  const ws = new WsJsonClient(host, port);
  await ws.connect();
  await joinRoom(ws, roomId, 'cpp_admin');
  try {
    return await fn(ws);
  } finally {
    ws.close();
  }
}

async function waitForPeerStats(ws, peerId, predicate, timeoutMs = 10000) {
  const deadline = Date.now() + timeoutMs;
  let lastData = null;
  while (Date.now() < deadline) {
    const statsResp = await ws.request('getStats', { peerId });
    if (!statsResp.ok) {
      throw new Error(`getStats failed: ${JSON.stringify(statsResp)}`);
    }
    lastData = statsResp.data;
    if (predicate(lastData)) return lastData;
    await sleep(250);
  }
  return lastData;
}

async function collectPeerStatsWindow(ws, peerId, samples = 6, intervalMs = 1000) {
  const collected = [];
  for (let i = 0; i < samples; ++i) {
    const statsResp = await ws.request('getStats', { peerId });
    if (!statsResp.ok) {
      throw new Error(`getStats failed: ${JSON.stringify(statsResp)}`);
    }
    collected.push(statsResp.data);
    if (i + 1 < samples) await sleep(intervalMs);
  }
  return collected;
}

async function runPublishSnapshotScenario() {
  const scenario = loadScenario('publish_snapshot');
  const harness = await createHarnessForScenario(
    'publish_snapshot',
    scenario,
    {
      QOS_TEST_CLIENT_STATS_PAYLOADS: JSON.stringify(buildPublishSnapshotPayloads(scenario)),
    }
  );

  try {
    await withAdminClient(scenario.roomId, async ws => {
      await sleep(1500);
      const statsResp = await ws.request('getStats', { peerId: scenario.peerId });
      if (!statsResp.ok) {
        throw new Error(`getStats failed: ${JSON.stringify(statsResp)}`);
      }
      const data = statsResp.data;
      if (!data.clientStats || !data.qos) {
        throw new Error(`missing qos aggregate in stats: ${JSON.stringify(data)}`);
      }
      if (data.clientStats.seq !== scenario.expect.clientStatsSeq) {
        throw new Error(`unexpected clientStats seq: ${data.clientStats.seq}`);
      }
      if (data.qos.quality !== scenario.expect.qosQuality) {
        throw new Error(`unexpected qos quality: ${data.qos.quality}`);
      }
    });
  } finally {
    await harness.stop();
  }
}

async function runThreadedPublishSnapshotScenario() {
  const scenario = loadScenario('publish_snapshot');
  const harness = await createHarnessForScenario(
    'threaded_publish_snapshot',
    scenario,
    {
      PLAIN_CLIENT_THREADED: '1',
      QOS_TEST_CLIENT_STATS_PAYLOADS: JSON.stringify(buildPublishSnapshotPayloads(scenario)),
    }
  );

  try {
    await withAdminClient(scenario.roomId, async ws => {
      const data = await waitForPeerStats(
        ws,
        scenario.peerId,
        stats => Boolean(stats?.clientStats && stats?.qos),
        12000
      );
      if (!data.clientStats || !data.qos) {
        throw new Error(`missing qos aggregate in stats: ${JSON.stringify(data)}`);
      }
      if (data.clientStats.seq !== scenario.expect.clientStatsSeq) {
        throw new Error(`unexpected clientStats seq: ${data.clientStats.seq}`);
      }
    });
  } finally {
    await harness.stop();
  }
}

async function runThreadedRejectedAckRetryScenario() {
  const scenario = loadScenario('threaded_rejected_ack_retry');
  const harness = await createHarnessForScenario(
    'threaded_rejected_ack_retry',
    scenario,
    {
      PLAIN_CLIENT_THREADED: '1',
      QOS_TEST_REJECT_FIRST_SET_ENCODING_TRACK_INDEX: '0',
      QOS_TEST_MATRIX_PROFILE: JSON.stringify(scenario.matrixProfile),
      QOS_TEST_MATRIX_LOCAL_ONLY: '1',
    }
  );

  try {
    const rejected = await waitForLog(harness, /THREADED_ACK.*applied=0.*test-reject-first-set-encoding/, 12000);
    if (!rejected) {
      throw new Error('threaded_rejected_ack_retry did not observe rejected encoding ack');
    }
    const applied = await waitForLog(harness, /THREADED_ACK.*applied=1/, 12000);
    if (!applied) {
      throw new Error('threaded_rejected_ack_retry did not observe later applied encoding ack');
    }
  } finally {
    await harness.stop();
  }
}

async function runThreadedAudioStatsSmokeScenario() {
  const scenario = {
    roomId: 'qos_harness_room_threaded_audio',
    peerId: 'alice',
  };
  const harness = await createCppClientHarness({
    signalingPort: port,
    roomId: scenario.roomId,
    peerId: scenario.peerId,
    mp4Path: ensureHarnessAvMp4(),
    extraEnv: {
      PLAIN_CLIENT_THREADED: '1',
    },
  });

  try {
    await withAdminClient(scenario.roomId, async ws => {
      const data = await waitForPeerStats(
        ws,
        scenario.peerId,
        stats => {
          const producers = stats?.producers ?? {};
          const kinds = Object.values(producers).map(p => p?.kind);
          return kinds.includes('audio') && kinds.includes('video');
        },
        12000
      );

      const producers = data?.producers ?? {};
      const kinds = Object.values(producers).map(p => p?.kind).sort();
      if (!(kinds.includes('audio') && kinds.includes('video'))) {
        throw new Error(`threaded audio smoke missing audio/video producers: ${JSON.stringify(data)}`);
      }
    });
  } finally {
    await harness.stop();
  }
}

async function runStaleSeqScenario() {
  const scenario = loadScenario('stale_seq');
  const harness = await createHarnessForScenario(
    'stale_seq',
    scenario,
    {
      QOS_TEST_CLIENT_STATS_PAYLOADS: JSON.stringify(buildStaleSeqPayloads(scenario)),
    }
  );

  try {
    await withAdminClient(scenario.roomId, async ws => {
      await sleep(1500);
      const statsResp = await ws.request('getStats', { peerId: scenario.peerId });
      if (!statsResp.ok) {
        throw new Error(`getStats failed: ${JSON.stringify(statsResp)}`);
      }
      const data = statsResp.data;
      if (data.clientStats.seq !== 50 || data.qos.seq !== 50 || data.qos.quality !== 'good') {
        throw new Error(`stale seq handling failed: ${JSON.stringify(data)}`);
      }
    });
  } finally {
    await harness.stop();
  }
}

async function runPolicyUpdateScenario() {
  const scenario = loadScenario('policy_update');
  const harness = await createHarnessForScenario(
    'policy_update',
    scenario,
    {
      QOS_TEST_SELF_REQUESTS: JSON.stringify([
        {
          delayMs: 500,
          method: 'setQosPolicy',
          data: {
            policy: scenario.policy,
          },
        },
      ]),
    }
  );

  try {
    const line = await waitForLog(harness, /policy updated: sampleMs=1500 snapshotMs=4000/, 5000);
    if (!line) {
      throw new Error('policy update log not observed on plain-client');
    }
  } finally {
    await harness.stop();
  }
}

async function runOverrideScenario() {
  const scenario = loadScenario('override_force_audio_only');
  const harness = await createHarnessForScenario(
    'override_force_audio_only',
    scenario,
    {
      QOS_TEST_SELF_REQUESTS: JSON.stringify([
        {
          delayMs: 500,
          method: 'setQosOverride',
          data: {
            override: scenario.override,
          },
        },
      ]),
    }
  );

  try {
    const overrideLine = await waitForLog(harness, /override: reason=room_protection/, 5000);
    const suppressedLine = await waitForLog(harness, /video suppressed/, 5000);
    if (!overrideLine || !suppressedLine) {
      throw new Error('override_force_audio_only did not suppress video on plain-client');
    }
  } finally {
    await harness.stop();
  }
}

async function runAutomaticOverrideScenario() {
  const scenario = loadScenario('auto_override_poor');
  const harness = await createHarnessForScenario(
    'auto_override_poor',
    scenario,
    {
      QOS_TEST_CLIENT_STATS_PAYLOADS: JSON.stringify(buildAutoOverridePayloads(scenario)),
    }
  );

  try {
    const line = await waitForLog(harness, /override: reason=server_auto_poor/, 6000);
    if (!line) {
      throw new Error('automatic poor override was not observed on plain-client');
    }
  } finally {
    await harness.stop();
  }
}

async function runManualClearScenario() {
  const scenario = loadScenario('manual_clear');
  const harness = await createHarnessForScenario(
    'manual_clear',
    scenario,
    {
      QOS_TEST_SELF_REQUESTS: JSON.stringify([
        {
          delayMs: 500,
          method: 'setQosOverride',
          data: {
            override: scenario.override,
          },
        },
        {
          delayMs: 6000,
          method: 'setQosOverride',
          data: {
            override: scenario.clearOverride,
          },
        },
      ]),
    }
  );

  try {
    const suppressedLine = await waitForLog(harness, /video suppressed/, 7000);
    if (!suppressedLine) {
      throw new Error('manual override did not suppress video on plain-client');
    }

    const resumedLine = await waitForLog(harness, /video resumed/, 12000);
    if (!resumedLine) {
      throw new Error('manual clear did not resume video on plain-client');
    }
  } finally {
    await harness.stop();
  }
}

async function runMultiVideoBudgetScenario() {
  const scenario = loadScenario('multi_video_budget');
  const harness = await createHarnessForScenario(
    'multi_video_budget',
    scenario,
    {
      PLAIN_CLIENT_VIDEO_TRACK_COUNT: String(scenario.trackCount),
      PLAIN_CLIENT_VIDEO_TRACK_WEIGHTS: scenario.weights.join(','),
      QOS_TEST_MATRIX_PROFILE: JSON.stringify(scenario.matrixProfile),
      QOS_TEST_MATRIX_LOCAL_ONLY: '1',
    }
  );

  try {
    await sleep(14000);
    const trace = harness.getTrace();
    const samplesByTrack = trace.samplesByTrack ?? {};
    const expectedTrackIds = scenario.expect.trackIds;
    for (const trackId of expectedTrackIds) {
      const samples = samplesByTrack[trackId];
      if (!Array.isArray(samples) || samples.length === 0) {
        throw new Error(`multi_video_budget missing samples for ${trackId}`);
      }
    }

    const latestByTrack = Object.fromEntries(
      expectedTrackIds.map(trackId => [trackId, samplesByTrack[trackId].at(-1)])
    );

    const highestBitrateTrack = scenario.expect.highestBitrateTrack;
    const lowestBitrateTrack = scenario.expect.lowestBitrateTrack;
    const highestBitrate = latestByTrack[highestBitrateTrack]?.bitrateBps;
    const middleBitrate = latestByTrack['video-1']?.bitrateBps;
    const lowestBitrate = latestByTrack[lowestBitrateTrack]?.bitrateBps;

    if (!(highestBitrate > middleBitrate && middleBitrate >= lowestBitrate)) {
      throw new Error(
        `unexpected multi-track bitrate ordering: ${JSON.stringify(
          Object.fromEntries(
            Object.entries(latestByTrack).map(([trackId, sample]) => [
              trackId,
              sample?.bitrateBps ?? null,
            ])
          )
        )}`
      );
    }
  } finally {
    await harness.stop();
  }
}

async function runThreadedMultiVideoBudgetScenario() {
  const scenario = loadScenario('multi_video_budget');
  const harness = await createHarnessForScenario(
    'threaded_multi_video_budget',
    scenario,
    {
      PLAIN_CLIENT_THREADED: '1',
      PLAIN_CLIENT_VIDEO_TRACK_COUNT: String(scenario.trackCount),
      PLAIN_CLIENT_VIDEO_TRACK_WEIGHTS: scenario.weights.join(','),
      PLAIN_CLIENT_VIDEO_SOURCES: buildThreadedVideoSources(scenario.trackCount),
      QOS_TEST_MATRIX_PROFILE: JSON.stringify(scenario.matrixProfile),
      QOS_TEST_MATRIX_LOCAL_ONLY: '1',
    }
  );

  try {
    await sleep(19000);
    const trace = harness.getTrace();
    const samplesByTrack = trace.samplesByTrack ?? {};
    const expectedTrackIds = scenario.expect.trackIds;
    for (const trackId of expectedTrackIds) {
      const samples = samplesByTrack[trackId];
      if (!Array.isArray(samples) || samples.length === 0) {
        throw new Error(`threaded_multi_video_budget missing samples for ${trackId}`);
      }
    }

    const highestBitrateTrack = scenario.expect.highestBitrateTrack;
    const lowestBitrateTrack = scenario.expect.lowestBitrateTrack;
    const alignedLength = Math.min(...expectedTrackIds.map(trackId => samplesByTrack[trackId].length));
    const startIndex = Math.max(0, alignedLength - 10);
    let orderingObserved = false;
    for (let i = startIndex; i < alignedLength; ++i) {
      const highestBitrate = samplesByTrack[highestBitrateTrack][i]?.bitrateBps;
      const middleBitrate = samplesByTrack['video-1'][i]?.bitrateBps;
      const lowestBitrate = samplesByTrack[lowestBitrateTrack][i]?.bitrateBps;
      if (highestBitrate > middleBitrate && middleBitrate >= lowestBitrate) {
        orderingObserved = true;
        break;
      }
    }

    if (!orderingObserved) {
      throw new Error(
        `unexpected threaded multi-track bitrate ordering: ${JSON.stringify(
          Object.fromEntries(
            expectedTrackIds.map(trackId => [trackId, samplesByTrack[trackId].slice(-5).map(sample => sample?.bitrateBps ?? null)])
          )
        )}`
      );
    }

    await withAdminClient(scenario.roomId, async ws => {
      const data = await waitForPeerStats(
        ws,
        scenario.peerId,
        stats => Array.isArray(stats?.clientStats?.tracks) && stats.clientStats.tracks.length >= expectedTrackIds.length,
        12000
      );

      const clientStats = data?.clientStats;
      if (!clientStats || !Array.isArray(clientStats.tracks)) {
        throw new Error(`threaded_multi_video_budget missing clientStats tracks: ${JSON.stringify(data)}`);
      }

      const actualTrackIds = clientStats.tracks
        .map(track => track.localTrackId)
        .sort();
      const expectedSortedTrackIds = [...expectedTrackIds].sort();
      if (JSON.stringify(actualTrackIds) !== JSON.stringify(expectedSortedTrackIds)) {
        throw new Error(`unexpected threaded clientStats trackIds: expected=${JSON.stringify(expectedSortedTrackIds)} actual=${JSON.stringify(actualTrackIds)}`);
      }

      const statsWindow = await collectPeerStatsWindow(ws, scenario.peerId, 4, 750);
      let serverOrderingObserved = false;
      for (const windowData of statsWindow) {
        const tracks = windowData?.clientStats?.tracks;
        if (!Array.isArray(tracks)) continue;
        const byId = Object.fromEntries(tracks.map(track => [track.localTrackId, track]));
        const highestBitrate = Number(byId[highestBitrateTrack]?.signals?.sendBitrateBps ?? 0);
        const middleBitrate = Number(byId['video-1']?.signals?.sendBitrateBps ?? 0);
        const lowestBitrate = Number(byId[lowestBitrateTrack]?.signals?.sendBitrateBps ?? 0);
        // Server-side clientStats carry observed sendBitrateBps, not local target caps.
        // Require the sacrificial track to stay clearly lowest, and both higher-weight
        // tracks to remain above it, but do not require strict ordering between them.
        if (lowestBitrate > 0
          && middleBitrate > lowestBitrate
          && highestBitrate > lowestBitrate) {
          serverOrderingObserved = true;
          break;
        }
      }
      if (!serverOrderingObserved) {
        throw new Error(`threaded_multi_video_budget did not observe server-side clientStats ordering over window: ${JSON.stringify(
          statsWindow.map(windowData => {
            const tracks = windowData?.clientStats?.tracks ?? [];
            return Object.fromEntries(tracks.map(track => [track.localTrackId, track?.signals?.sendBitrateBps ?? null]));
          })
        )}`);
      }
    });
  } finally {
    await harness.stop();
  }
}

async function runMultiTrackSnapshotScenario() {
  const scenario = loadScenario('multi_track_snapshot');
  const harness = await createHarnessForScenario(
    'multi_track_snapshot',
    scenario,
    {
      PLAIN_CLIENT_VIDEO_TRACK_COUNT: String(scenario.trackCount),
    }
  );

  try {
    await withAdminClient(scenario.roomId, async ws => {
      await sleep(4500);
      const statsResp = await ws.request('getStats', { peerId: scenario.peerId });
      if (!statsResp.ok) {
        throw new Error(`getStats failed: ${JSON.stringify(statsResp)}`);
      }

      const clientStats = statsResp.data?.clientStats;
      if (!clientStats || !Array.isArray(clientStats.tracks)) {
        throw new Error(`multi_track_snapshot missing clientStats tracks: ${JSON.stringify(statsResp.data)}`);
      }

      const actualTrackIds = clientStats.tracks
        .map(track => track.localTrackId)
        .sort();
      const expectedTrackIds = [...scenario.expect.trackIds].sort();

      if (actualTrackIds.length !== expectedTrackIds.length) {
        throw new Error(`unexpected track count: expected=${expectedTrackIds.length} actual=${actualTrackIds.length} payload=${JSON.stringify(clientStats)}`);
      }

      for (const trackId of expectedTrackIds) {
        if (!actualTrackIds.includes(trackId)) {
          throw new Error(`missing aggregated track ${trackId}: ${JSON.stringify(clientStats)}`);
        }
      }
    });
  } finally {
    await harness.stop();
  }
}

async function runThreadedMultiTrackSnapshotScenario() {
  const scenario = loadScenario('multi_track_snapshot');
  const harness = await createHarnessForScenario(
    'threaded_multi_track_snapshot',
    scenario,
    {
      PLAIN_CLIENT_THREADED: '1',
      PLAIN_CLIENT_VIDEO_TRACK_COUNT: String(scenario.trackCount),
      PLAIN_CLIENT_VIDEO_SOURCES: buildThreadedVideoSources(scenario.trackCount),
    }
  );

  try {
    await withAdminClient(scenario.roomId, async ws => {
      const data = await waitForPeerStats(
        ws,
        scenario.peerId,
        stats => Array.isArray(stats?.clientStats?.tracks),
        12000
      );
      const clientStats = data?.clientStats;
      if (!clientStats || !Array.isArray(clientStats.tracks)) {
        throw new Error(`threaded_multi_track_snapshot missing clientStats tracks: ${JSON.stringify(data)}`);
      }

      const actualTrackIds = clientStats.tracks
        .map(track => track.localTrackId)
        .sort();
      const expectedTrackIds = [...scenario.expect.trackIds].sort();

      if (actualTrackIds.length !== expectedTrackIds.length) {
        throw new Error(`unexpected track count: expected=${expectedTrackIds.length} actual=${actualTrackIds.length} payload=${JSON.stringify(clientStats)}`);
      }

      for (const trackId of expectedTrackIds) {
        if (!actualTrackIds.includes(trackId)) {
          throw new Error(`missing aggregated track ${trackId}: ${JSON.stringify(clientStats)}`);
        }
      }
    });
  } finally {
    await harness.stop();
  }
}

async function runThreadedStaleTrackSkipScenario() {
  const scenario = loadScenario('threaded_stale_track_skip');
  const harness = await createHarnessForScenario(
    'threaded_stale_track_skip',
    scenario,
    {
      PLAIN_CLIENT_THREADED: '1',
      PLAIN_CLIENT_VIDEO_TRACK_COUNT: String(scenario.trackCount),
      PLAIN_CLIENT_VIDEO_SOURCES: buildThreadedVideoSources(scenario.trackCount),
      QOS_TEST_FORCE_STALE_TRACK_INDEX: String(scenario.staleTrackIndex),
    }
  );

  try {
    await withAdminClient(scenario.roomId, async ws => {
      const data = await waitForPeerStats(
        ws,
        scenario.peerId,
        stats => Array.isArray(stats?.clientStats?.tracks),
        12000
      );
      const clientStats = data?.clientStats;
      if (!clientStats || !Array.isArray(clientStats.tracks)) {
        throw new Error(`threaded_stale_track_skip missing clientStats tracks: ${JSON.stringify(data)}`);
      }
      const actualTrackIds = clientStats.tracks.map(track => track.localTrackId).sort();
      const expectedTrackIds = [...scenario.expectPresentTrackIds].sort();
      if (JSON.stringify(actualTrackIds) !== JSON.stringify(expectedTrackIds)) {
        throw new Error(`unexpected sampled track set: expected=${JSON.stringify(expectedTrackIds)} actual=${JSON.stringify(actualTrackIds)}`);
      }
    });
  } finally {
    await harness.stop();
  }
}

async function runThreadedGenerationSwitchScenario() {
  const scenario = loadScenario('threaded_generation_switch');
  const harness = await createHarnessForScenario(
    'threaded_generation_switch',
    scenario,
    {
      PLAIN_CLIENT_THREADED: '1',
      QOS_TEST_MATRIX_PROFILE: JSON.stringify(scenario.matrixProfile),
      QOS_TEST_MATRIX_LOCAL_ONLY: '1',
    }
  );

  try {
    const applied = await waitForLog(harness, /THREADED_ACK.*gen=[1-9][0-9]*.*applied=1/, 15000);
    if (!applied) {
      throw new Error('threaded_generation_switch did not observe configGeneration bump ack');
    }
  } finally {
    await harness.stop();
  }
}

async function runThreadedQuickScenario() {
  await runThreadedPublishSnapshotScenario();
  await runThreadedAudioStatsSmokeScenario();
  await runThreadedMultiTrackSnapshotScenario();
  await runThreadedMultiVideoBudgetScenario();
  await runThreadedFallbackMultiTrackSnapshotScenario();
  await runThreadedRejectedAckRetryScenario();
  await runThreadedStaleTrackSkipScenario();
  await runThreadedGenerationSwitchScenario();
}

async function runThreadedFallbackMultiTrackSnapshotScenario() {
  const scenario = loadScenario('multi_track_snapshot');
  const harness = await createHarnessForScenario(
    'threaded_fallback_multi_track_snapshot',
    scenario,
    {
      PLAIN_CLIENT_THREADED: '1',
      PLAIN_CLIENT_VIDEO_TRACK_COUNT: String(scenario.trackCount),
    }
  );

  try {
    const fallbackLine = await waitForLog(harness, /using legacy path/i, 8000);
    if (!fallbackLine) {
      throw new Error('threaded fallback warning not observed on plain-client');
    }

    await withAdminClient(scenario.roomId, async ws => {
      const data = await waitForPeerStats(
        ws,
        scenario.peerId,
        stats => Array.isArray(stats?.clientStats?.tracks),
        12000
      );
      const clientStats = data?.clientStats;
      if (!clientStats || !Array.isArray(clientStats.tracks)) {
        throw new Error(`multi_track_snapshot missing clientStats tracks: ${JSON.stringify(data)}`);
      }

      const actualTrackIds = clientStats.tracks
        .map(track => track.localTrackId)
        .sort();
      const expectedTrackIds = [...scenario.expect.trackIds].sort();

      if (actualTrackIds.length !== expectedTrackIds.length) {
        throw new Error(`unexpected track count: expected=${expectedTrackIds.length} actual=${actualTrackIds.length} payload=${JSON.stringify(clientStats)}`);
      }

      for (const trackId of expectedTrackIds) {
        if (!actualTrackIds.includes(trackId)) {
          throw new Error(`missing aggregated track ${trackId}: ${JSON.stringify(clientStats)}`);
        }
      }
    });
  } finally {
    await harness.stop();
  }
}

async function main() {
  const scenario = process.argv[2] || 'quick';

  if (scenario === 'quick' || scenario === 'publish_snapshot') {
    await runPublishSnapshotScenario();
  }
  if (scenario === 'threaded_publish_snapshot') {
    await runThreadedPublishSnapshotScenario();
  }
  if (scenario === 'threaded_audio_stats_smoke') {
    await runThreadedAudioStatsSmokeScenario();
  }
  if (scenario === 'threaded_rejected_ack_retry') {
    await runThreadedRejectedAckRetryScenario();
  }
  if (scenario === 'quick' || scenario === 'stale_seq') {
    await runStaleSeqScenario();
  }
  if (scenario === 'quick' || scenario === 'policy_update') {
    await runPolicyUpdateScenario();
  }
  if (scenario === 'override_force_audio_only') {
    await runOverrideScenario();
  }
  if (scenario === 'auto_override_poor') {
    await runAutomaticOverrideScenario();
  }
  if (scenario === 'manual_clear') {
    await runManualClearScenario();
  }
  if (scenario === 'multi_video_budget') {
    await runMultiVideoBudgetScenario();
  }
  if (scenario === 'threaded_multi_video_budget') {
    await runThreadedMultiVideoBudgetScenario();
  }
  if (scenario === 'multi_track_snapshot') {
    await runMultiTrackSnapshotScenario();
  }
  if (scenario === 'threaded_multi_track_snapshot') {
    await runThreadedMultiTrackSnapshotScenario();
  }
  if (scenario === 'threaded_stale_track_skip') {
    await runThreadedStaleTrackSkipScenario();
  }
  if (scenario === 'threaded_fallback_multi_track_snapshot') {
    await runThreadedFallbackMultiTrackSnapshotScenario();
  }
  if (scenario === 'threaded_generation_switch') {
    await runThreadedGenerationSwitchScenario();
  }
  if (scenario === 'threaded_quick') {
    await runThreadedQuickScenario();
  }

  console.log(`cpp_client_harness ${scenario} passed`);
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
