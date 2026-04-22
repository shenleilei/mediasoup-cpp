import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import {
  createCppClientHarness,
  ensureHarnessMp4,
  ensureHarnessAvMp4,
  scheduleNetemPhaseProfile,
  sleep,
} from './cpp_client_runner.mjs';
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

function hasExpectedTrackIds(clientStats, expectedTrackIds) {
  if (!clientStats || !Array.isArray(clientStats.tracks)) {
    return false;
  }

  const actualTrackIds = new Set(clientStats.tracks.map(track => track.localTrackId));
  return expectedTrackIds.every(trackId => actualTrackIds.has(trackId));
}

function hasPositiveSendBitrateForTrackSet(clientStats, expectedTrackIds) {
  if (!hasExpectedTrackIds(clientStats, expectedTrackIds)) {
    return false;
  }

  const tracksById = Object.fromEntries(
    clientStats.tracks.map(track => [track.localTrackId, track])
  );
  return expectedTrackIds.every(trackId =>
    Number(tracksById[trackId]?.signals?.sendBitrateBps ?? 0) > 0
  );
}

async function runPublishSnapshotScenario() {
  const scenario = loadScenario('publish_snapshot');
  const harness = await createHarnessForScenario('publish_snapshot', scenario);

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
      if (!Array.isArray(data.clientStats.tracks) || data.clientStats.tracks.length === 0) {
        throw new Error(`missing clientStats tracks: ${JSON.stringify(data)}`);
      }
    });
  } finally {
    await harness.stop();
  }
}

async function runThreadedPublishSnapshotScenario() {
  const scenario = loadScenario('publish_snapshot');
  const harness = await createHarnessForScenario('threaded_publish_snapshot', scenario);

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
      if (!Array.isArray(data.clientStats.tracks) || data.clientStats.tracks.length === 0) {
        throw new Error(`missing clientStats tracks: ${JSON.stringify(data)}`);
      }
    });
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


async function runThreadedMultiVideoBudgetScenario() {
  const scenario = loadScenario('multi_video_budget');
  const harness = await createHarnessForScenario(
    'threaded_multi_video_budget',
    scenario,
    {
      PLAIN_CLIENT_VIDEO_TRACK_COUNT: String(scenario.trackCount),
      PLAIN_CLIENT_VIDEO_TRACK_WEIGHTS: scenario.weights.join(','),
      PLAIN_CLIENT_VIDEO_SOURCES: buildThreadedVideoSources(scenario.trackCount),
    }
  );
  const scheduledProfile = scheduleNetemPhaseProfile(scenario.matrixProfile);

  try {
    // scheduleNetemPhaseProfile keeps the final phase active until stop(), so
    // this extra settling time is still measured under the impaired link.
    await sleep(scheduledProfile.totalDurationMs + 1500);
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
    const alignedLength = Math.min(...expectedTrackIds.map(trackId => samplesByTrack[trackId].length));
    const startIndex = Math.max(0, alignedLength - 10);
    let degradationObserved = false;
    for (let i = startIndex; i < alignedLength; ++i) {
      const bitrates = expectedTrackIds.map(trackId => samplesByTrack[trackId][i]?.bitrateBps ?? 0);
      if (bitrates.every(bitrate => bitrate > 0 && bitrate < 900000)
        && Math.max(...bitrates) <= 850000) {
        degradationObserved = true;
        break;
      }
    }

    if (!degradationObserved) {
      throw new Error(
        `threaded multi-track degradation was not observed under constrained network: ${JSON.stringify(
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
        stats => hasExpectedTrackIds(stats?.clientStats, expectedTrackIds),
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
    });
  } finally {
    scheduledProfile.stop();
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
      PLAIN_CLIENT_VIDEO_TRACK_COUNT: String(scenario.trackCount),
      PLAIN_CLIENT_VIDEO_SOURCES: buildThreadedVideoSources(scenario.trackCount),
    }
  );

  try {
    await withAdminClient(scenario.roomId, async ws => {
      const data = await waitForPeerStats(
        ws,
        scenario.peerId,
        stats => hasExpectedTrackIds(stats?.clientStats, scenario.expect.trackIds),
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

async function runThreadedGenerationSwitchScenario() {
  const scenario = loadScenario('threaded_generation_switch');
  const harness = await createHarnessForScenario('threaded_generation_switch', scenario);
  const scheduledProfile = scheduleNetemPhaseProfile(scenario.matrixProfile);

  try {
    const applied = await waitForLog(harness, /THREADED_ACK.*gen=[1-9][0-9]*.*applied=1/, 15000);
    if (!applied) {
      throw new Error('threaded_generation_switch did not observe configGeneration bump ack');
    }
  } finally {
    scheduledProfile.stop();
    await harness.stop();
  }
}

async function runThreadedQuickScenario() {
  await runThreadedPublishSnapshotScenario();
  await sleep(1000);
  await runThreadedAudioStatsSmokeScenario();
  await sleep(1000);
  await runThreadedMultiTrackSnapshotScenario();
  await sleep(1000);
  await runThreadedMultiVideoBudgetScenario();
  await sleep(1000);
  await runThreadedDefaultMultiTrackSnapshotScenario();
  await sleep(1000);
  await runThreadedGenerationSwitchScenario();
}

async function runThreadedDefaultMultiTrackSnapshotScenario() {
  const scenario = loadScenario('multi_track_snapshot');
  const harness = await createHarnessForScenario(
    'threaded_default_multi_track_snapshot',
    scenario,
    {
      PLAIN_CLIENT_VIDEO_TRACK_COUNT: String(scenario.trackCount),
    }
  );

  try {
    const threadedStart = await waitForLog(harness, /starting multi-thread pipeline/i, 8000);
    if (!threadedStart) {
      throw new Error('default multi-track startup did not enter the threaded runtime');
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

    const logs = harness.getLogs();
    const unexpectedLegacyLine = [...logs.clientStdout, ...logs.clientStderr].find(line =>
      /using legacy path|sender QoS runtime disabled/i.test(line)
    );
    if (unexpectedLegacyLine) {
      throw new Error(`unexpected legacy fallback log observed: ${unexpectedLegacyLine}`);
    }
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
  if (scenario === 'threaded_multi_video_budget') {
    await runThreadedMultiVideoBudgetScenario();
  }
  if (scenario === 'multi_track_snapshot') {
    await runMultiTrackSnapshotScenario();
  }
  if (scenario === 'threaded_multi_track_snapshot') {
    await runThreadedMultiTrackSnapshotScenario();
  }
  if (scenario === 'threaded_default_multi_track_snapshot') {
    await runThreadedDefaultMultiTrackSnapshotScenario();
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
