import fs from 'node:fs';
import path from 'node:path';
import net from 'node:net';
import { execFileSync, spawn } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const tcPath = '/usr/sbin/tc';
const HARNESS_WARMUP_MS = 2000;
const DEFAULT_HARNESS_MP4_PATH = path.join(repoRoot, 'tests', 'fixtures', 'media', 'test_sweep.mp4');
const DEFAULT_HARNESS_AV_MP4_PATH = path.join(repoRoot, 'tests', 'fixtures', 'media', 'test_sweep_av.mp4');
const DEFAULT_MATRIX_MP4_PATH = path.join(repoRoot, 'tests', 'fixtures', 'media', 'test_sweep_cpp_matrix.mp4');

function runTc(args) {
  execFileSync(tcPath, args, { stdio: 'inherit' });
}

function readMediaDurationSeconds(filePath) {
  try {
    const output = execFileSync(
      'ffprobe',
      [
        '-v', 'error',
        '-show_entries', 'format=duration',
        '-of', 'default=noprint_wrappers=1:nokey=1',
        filePath,
      ],
      { encoding: 'utf8' }
    );
    const value = Number(output.trim());
    return Number.isFinite(value) ? value : null;
  } catch {
    return null;
  }
}

function ensureMatrixMp4(minDurationSeconds = 240) {
  const duration = fs.existsSync(DEFAULT_MATRIX_MP4_PATH)
    ? readMediaDurationSeconds(DEFAULT_MATRIX_MP4_PATH)
    : null;

  if (duration !== null && duration >= minDurationSeconds - 1) {
    return DEFAULT_MATRIX_MP4_PATH;
  }

  execFileSync(
    'ffmpeg',
    [
      '-y',
      '-f', 'lavfi',
      '-i', `testsrc=d=${minDurationSeconds}:s=640x480:r=25`,
      '-pix_fmt', 'yuv420p',
      '-c:v', 'libx264',
      '-profile:v', 'baseline',
      '-g', '25',
      DEFAULT_MATRIX_MP4_PATH,
    ],
    { stdio: 'ignore' }
  );

  return DEFAULT_MATRIX_MP4_PATH;
}

export function ensureHarnessMp4(minDurationSeconds = 45) {
  const duration = fs.existsSync(DEFAULT_HARNESS_MP4_PATH)
    ? readMediaDurationSeconds(DEFAULT_HARNESS_MP4_PATH)
    : null;

  if (duration !== null && duration >= minDurationSeconds - 1) {
    return DEFAULT_HARNESS_MP4_PATH;
  }

  execFileSync(
    'ffmpeg',
    [
      '-y',
      '-f', 'lavfi',
      '-i', `testsrc=d=${minDurationSeconds}:s=640x480:r=25`,
      '-pix_fmt', 'yuv420p',
      '-c:v', 'libx264',
      '-profile:v', 'baseline',
      '-g', '25',
      DEFAULT_HARNESS_MP4_PATH,
    ],
    { stdio: 'ignore' }
  );

  return DEFAULT_HARNESS_MP4_PATH;
}

export function ensureHarnessAvMp4(minDurationSeconds = 45) {
  const duration = fs.existsSync(DEFAULT_HARNESS_AV_MP4_PATH)
    ? readMediaDurationSeconds(DEFAULT_HARNESS_AV_MP4_PATH)
    : null;

  if (duration !== null && duration >= minDurationSeconds - 1) {
    return DEFAULT_HARNESS_AV_MP4_PATH;
  }

  execFileSync(
    'ffmpeg',
    [
      '-y',
      '-f', 'lavfi',
      '-i', `testsrc=d=${minDurationSeconds}:s=640x480:r=25`,
      '-f', 'lavfi',
      '-i', `sine=frequency=1000:duration=${minDurationSeconds}`,
      '-pix_fmt', 'yuv420p',
      '-c:v', 'libx264',
      '-profile:v', 'baseline',
      '-g', '25',
      '-c:a', 'aac',
      '-shortest',
      DEFAULT_HARNESS_AV_MP4_PATH,
    ],
    { stdio: 'ignore' }
  );

  return DEFAULT_HARNESS_AV_MP4_PATH;
}

export function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

export function clearNetem(iface = 'lo') {
  try {
    execFileSync(tcPath, ['qdisc', 'del', 'dev', iface, 'root'], { stdio: 'ignore' });
  } catch {}
}

export function applyNetemConfig(config = {}, iface = 'lo') {
  const { bandwidth, rtt, loss, jitter } = config;
  clearNetem(iface);

  runTc([
    'qdisc', 'add', 'dev', iface, 'root', 'handle', '1:',
    'prio', 'bands', '3',
    'priomap', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1',
  ]);

  const netemArgs = ['qdisc', 'add', 'dev', iface, 'parent', '1:3', 'handle', '30:', 'netem'];
  const delayBase = typeof rtt === 'number' ? Math.max(1, Math.round(rtt / 2)) : 1;
  netemArgs.push('delay', `${delayBase}ms`);

  if (typeof jitter === 'number' && jitter > 0) {
    netemArgs.push(`${Math.round(jitter)}ms`, 'distribution', 'normal');
  }
  if (typeof loss === 'number' && loss > 0) {
    netemArgs.push('loss', `${loss}%`);
  }
  if (typeof bandwidth === 'number' && bandwidth > 0) {
    const effectiveBandwidthKbps = Math.max(
      1,
      Math.round(
        bandwidth >= 3000
          ? bandwidth * 0.85
          : bandwidth * 0.7
      )
    );
    netemArgs.push('rate', `${effectiveBandwidthKbps}kbit`);
  }

  runTc(netemArgs);
  runTc([
    'filter', 'add', 'dev', iface, 'parent', '1:0', 'protocol', 'ip', 'u32',
    'match', 'ip', 'protocol', '17', '0xff', 'flowid', '1:3',
  ]);
}

export async function waitForPort(hostname, port, timeoutMs = 5000) {
  const deadline = Date.now() + timeoutMs;

  while (Date.now() < deadline) {
    try {
      await new Promise((resolve, reject) => {
        const socket = net.createConnection({ host: hostname, port }, () => {
          socket.destroy();
          resolve();
        });
        socket.once('error', reject);
      });
      return;
    } catch {
      await sleep(100);
    }
  }

  throw new Error(`port ${port} did not become ready`);
}

export function stopChild(child, timeoutMs = 3000) {
  return new Promise(resolve => {
    if (!child?.pid || child.exitCode !== null) {
      resolve();
      return;
    }

    const timer = setTimeout(() => {
      try { child.kill('SIGKILL'); } catch {}
      resolve();
    }, timeoutMs);

    child.once('close', () => {
      clearTimeout(timer);
      resolve();
    });

    try {
      child.kill('SIGTERM');
    } catch {
      clearTimeout(timer);
      resolve();
    }
  });
}

function createLineCollector(target, onLine) {
  let pending = '';

  return chunk => {
    pending += chunk.toString('utf8');
    let newlineIndex = pending.indexOf('\n');
    while (newlineIndex !== -1) {
      const line = pending.slice(0, newlineIndex).replace(/\r$/, '');
      pending = pending.slice(newlineIndex + 1);
      target.push(line);
      onLine?.(line);
      newlineIndex = pending.indexOf('\n');
    }
  };
}

function tailLines(lines, count = 20) {
  return lines.slice(-count).join('\n');
}

function parseQosTraceLine(line) {
  if (!line.startsWith('[QOS_TRACE]')) {
    return null;
  }

  const fields = {};
  for (const token of line.replace('[QOS_TRACE]', '').trim().split(/\s+/)) {
    const separatorIndex = token.indexOf('=');
    if (separatorIndex === -1) {
      continue;
    }
    const key = token.slice(0, separatorIndex);
    const value = token.slice(separatorIndex + 1);
    fields[key] = value;
  }

  const tsMs = Number(fields.tsMs);
  const level = Number(fields.level);
  if (!Number.isFinite(tsMs) || !Number.isFinite(level) || !fields.state) {
    return null;
  }

  const readNumber = key => {
    const value = Number(fields[key]);
    return Number.isFinite(value) ? value : undefined;
  };

  return {
    tsMs,
    trackId: fields.track ?? 'video',
    state: fields.state,
    level,
    mode: fields.mode ?? 'audio-video',
    sample: fields.sample ?? 'unknown',
    bitrateBps: readNumber('bitrateBps'),
    sendBps: readNumber('sendBps'),
    packetsLost: readNumber('packetsLost'),
    rttMs: readNumber('rttMs'),
    jitterMs: readNumber('jitterMs'),
    width: readNumber('width'),
    height: readNumber('height'),
    fps: readNumber('fps'),
    suppressed: readNumber('suppressed') === 1,
    line,
  };
}

function inferActionType(current, previous) {
  if (!previous) {
    return 'noop';
  }
  if (current.mode === 'audio-only' && previous.mode !== 'audio-only') {
    return 'enterAudioOnly';
  }
  if (current.mode !== 'audio-only' && previous.mode === 'audio-only') {
    return 'exitAudioOnly';
  }
  if (current.level !== previous.level) {
    return 'setEncodingParameters';
  }
  return 'noop';
}

function buildTraceFromSamples(samples) {
  const trace = [];
  let previous = null;

  for (const sample of samples) {
    const actionType = inferActionType(sample, previous);
    trace.push({
      tsMs: sample.tsMs,
      stateAfter: sample.state,
      plannedAction: {
        type: actionType,
        level: sample.level,
      },
      sample: {
        mode: sample.mode,
        source: sample.sample,
        bitrateBps: sample.bitrateBps,
        sendBps: sample.sendBps,
        packetsLost: sample.packetsLost,
        rttMs: sample.rttMs,
        jitterMs: sample.jitterMs,
        width: sample.width,
        height: sample.height,
        fps: sample.fps,
        suppressed: sample.suppressed,
      },
    });
    previous = sample;
  }

  return trace;
}

function buildTrace(lines) {
  const samples = lines
    .map(parseQosTraceLine)
    .filter(Boolean)
    .sort((left, right) => left.tsMs - right.tsMs);

  const samplesByTrack = new Map();
  for (const sample of samples) {
    const trackSamples = samplesByTrack.get(sample.trackId) ?? [];
    trackSamples.push(sample);
    samplesByTrack.set(sample.trackId, trackSamples);
  }

  const primaryTrackId = samplesByTrack.has('video')
    ? 'video'
    : (samplesByTrack.keys().next().value ?? 'video');
  const primarySamples = samplesByTrack.get(primaryTrackId) ?? [];
  const traceByTrack = new Map();
  for (const [trackId, trackSamples] of samplesByTrack.entries()) {
    traceByTrack.set(trackId, buildTraceFromSamples(trackSamples));
  }

  return {
    primaryTrackId,
    samples: primarySamples,
    trace: buildTraceFromSamples(primarySamples),
    allSamples: samples,
    samplesByTrack,
    traceByTrack,
  };
}

export function latestStateBefore(samples, endMs, fallbackState) {
  let latest = null;
  for (const sample of samples) {
    if (typeof endMs === 'number' && sample.tsMs > endMs) {
      break;
    }
    latest = sample;
  }

  if (!latest) {
    return {
      state: fallbackState?.state ?? 'stable',
      level: typeof fallbackState?.level === 'number' ? fallbackState.level : 0,
    };
  }

  return {
    state: latest.state,
    level: latest.level,
  };
}

export async function createCppClientHarness({
  signalingPort,
  roomId,
  peerId,
  mp4Path = ensureMatrixMp4(),
  iface = 'lo',
  serverHost = '127.0.0.1',
  copyMode = false,
  warmupMs = HARNESS_WARMUP_MS,
  extraEnv = {},
} = {}) {
  if (!fs.existsSync(mp4Path)) {
    throw new Error(`mp4 file not found: ${mp4Path}`);
  }

  const diagnostics = {
    sfuStdout: [],
    sfuStderr: [],
    clientStdout: [],
    clientStderr: [],
    sfuExitCode: null,
    clientExitCode: null,
    roomId,
    peerId,
    signalingPort,
  };

  let traceCache = { samples: [], trace: [] };
  let client = null;
  const sfu = spawn(
    path.join(repoRoot, 'build', 'mediasoup-sfu'),
    [
      '--nodaemon',
      `--port=${signalingPort}`,
      '--workers=1',
      '--workerBin=./mediasoup-worker',
      '--announcedIp=127.0.0.1',
      '--listenIp=127.0.0.1',
      '--redisHost=0.0.0.0',
      '--redisPort=1',
    ],
    {
      cwd: repoRoot,
      stdio: ['ignore', 'pipe', 'pipe'],
    }
  );

  const rebuildTraceCache = () => {
    traceCache = buildTrace(diagnostics.clientStdout);
  };

  sfu.stdout.on('data', createLineCollector(diagnostics.sfuStdout));
  sfu.stderr.on('data', createLineCollector(diagnostics.sfuStderr));

  sfu.once('close', code => {
    diagnostics.sfuExitCode = code;
  });

  const waitForClientTrace = async timeoutMs => {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      if (traceCache.trace.length > 0) {
        return;
      }
      if (diagnostics.clientExitCode !== null && diagnostics.clientExitCode !== 0) {
        throw new Error(
          `plain-client exited early (code=${diagnostics.clientExitCode})\n${tailLines(diagnostics.clientStdout, 40)}\n${tailLines(diagnostics.clientStderr, 40)}`
        );
      }
      await sleep(100);
    }

    throw new Error(
      `plain-client did not emit QOS_TRACE in time\n${tailLines(diagnostics.clientStdout, 40)}\n${tailLines(diagnostics.clientStderr, 40)}`
    );
  };

  const stopHarness = async () => {
    clearNetem(iface);
    await stopChild(client);
    await stopChild(sfu);
  };

  try {
    await waitForPort(serverHost, signalingPort, 5000);

    client = spawn(
      'stdbuf',
      [
        '-oL',
        '-eL',
        path.join(repoRoot, 'client', 'build', 'plain-client'),
        serverHost,
        String(signalingPort),
        roomId,
        peerId,
        mp4Path,
        ...(copyMode ? ['--copy'] : []),
      ],
      {
        cwd: repoRoot,
        env: {
          ...process.env,
          ...extraEnv,
        },
        stdio: ['ignore', 'pipe', 'pipe'],
      }
    );

    client.stdout.on('data', createLineCollector(diagnostics.clientStdout, rebuildTraceCache));
    client.stderr.on('data', createLineCollector(diagnostics.clientStderr));
    client.once('close', code => {
      diagnostics.clientExitCode = code;
    });

    await waitForClientTrace(15000);
    await sleep(warmupMs);

    return {
      nowMs() {
        return Date.now();
      },
      getTrace() {
        return {
          samples: traceCache.samples.map(sample => ({ ...sample })),
          trace: traceCache.trace.map(entry => JSON.parse(JSON.stringify(entry))),
          primaryTrackId: traceCache.primaryTrackId,
          allSamples: traceCache.allSamples.map(sample => ({ ...sample })),
          samplesByTrack: Object.fromEntries(
            Array.from(traceCache.samplesByTrack.entries()).map(([trackId, samples]) => [
              trackId,
              samples.map(sample => ({ ...sample })),
            ])
          ),
          traceByTrack: Object.fromEntries(
            Array.from(traceCache.traceByTrack.entries()).map(([trackId, trace]) => [
              trackId,
              trace.map(entry => JSON.parse(JSON.stringify(entry))),
            ])
          ),
        };
      },
      getLogs() {
        return {
          sfuStdout: [...diagnostics.sfuStdout],
          sfuStderr: [...diagnostics.sfuStderr],
          clientStdout: [...diagnostics.clientStdout],
          clientStderr: [...diagnostics.clientStderr],
        };
      },
      async stop() {
        await stopHarness();
      },
      async getDiagnostics() {
        return {
          ...diagnostics,
          sfuStdoutTail: tailLines(diagnostics.sfuStdout),
          sfuStderrTail: tailLines(diagnostics.sfuStderr),
          clientStdoutTail: tailLines(diagnostics.clientStdout),
          clientStderrTail: tailLines(diagnostics.clientStderr),
        };
      },
    };
  } catch (error) {
    error.qosDiagnostics = {
      ...diagnostics,
      sfuStdoutTail: tailLines(diagnostics.sfuStdout),
      sfuStderrTail: tailLines(diagnostics.sfuStderr),
      clientStdoutTail: tailLines(diagnostics.clientStdout),
      clientStderrTail: tailLines(diagnostics.clientStderr),
    };
    await stopHarness();
    throw error;
  }
}
