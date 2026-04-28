import assert from 'node:assert/strict';
import test from 'node:test';

import { buildClientTraceFromLogs } from './cpp_client_runner.mjs';

test('cpp-client trace parser accepts spdlog-prefixed QOS_TRACE lines on stderr', () => {
  const trace = buildClientTraceFromLogs({
    clientStdout: [
      '[2026-04-28 08:55:15.005] [info] WS connected to 127.0.0.1:14019',
    ],
    clientStderr: [
      '[2026-04-28 08:55:15.219] [info] [QOS_TRACE] tsMs=1777337715219 track=video state=stable level=0 mode=audio-video sample=matrix bitrateBps=900000 sendBps=880650 lossRate=0.000000 packetsLost=65536 rttMs=50.0 jitterMs=5.0 width=640 height=480 fps=25 suppressed=0',
    ],
  });

  assert.equal(trace.trace.length, 1);
  assert.equal(trace.samples.length, 1);
  assert.equal(trace.samples[0].state, 'stable');
  assert.equal(trace.samples[0].sample, 'matrix');
  assert.equal(trace.samples[0].sendBps, 880650);
});

test('cpp-client trace parser still accepts legacy bare QOS_TRACE lines', () => {
  const trace = buildClientTraceFromLogs({
    clientStdout: [
      '[QOS_TRACE] tsMs=1777337716256 track=video state=congested level=2 mode=audio-video sample=server bitrateBps=700000 sendBps=42 lossRate=0.100000 packetsLost=12 rttMs=500.0 jitterMs=5.0 width=426 height=320 fps=20 suppressed=0',
    ],
    clientStderr: [],
  });

  assert.equal(trace.trace.length, 1);
  assert.equal(trace.samples[0].level, 2);
  assert.equal(trace.samples[0].sample, 'server');
  assert.equal(trace.samples[0].packetsLost, 12);
});
