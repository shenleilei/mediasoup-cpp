import { spawn } from 'node:child_process';
import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const childScript = path.join(__dirname, 'single_worker_pressure.mjs');

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function parseArgs(argv) {
const opts = {
    wsUrl: 'wss://127.0.0.1:1770/ws',
    httpUrl: 'https://127.0.0.1:1770',
    container: 'mediasoup-cpp',
    sampleHost: '',
    roomsPerProcess: 100,
    step: 10,
    roundMs: 10000,
    steadyRoundMs: null,
    spawnIntervalMs: 10000,
    steadyRounds: 0,
    recvRatio: 0.85,
    maxProcesses: 0,
    prefix: `multi_pressure_${Date.now()}`,
    perf: false,
    perfIntervalMs: 20000,
    perfOutputDir: '',
    perfFrequency: 99,
    perfPercentLimit: 1,
  };

  for (const arg of argv) {
    if (!arg.startsWith('--')) continue;
    const [key, rawValue = ''] = arg.slice(2).split('=');
    const int = value => Number.parseInt(value, 10);
    switch (key) {
      case 'ws-url': opts.wsUrl = rawValue || opts.wsUrl; break;
      case 'http-url': opts.httpUrl = rawValue || opts.httpUrl; break;
      case 'container': opts.container = rawValue || opts.container; break;
      case 'sample-host': opts.sampleHost = rawValue || opts.sampleHost; break;
      case 'rooms-per-process': opts.roomsPerProcess = Math.max(1, int(rawValue)); break;
      case 'step': opts.step = Math.max(1, int(rawValue)); break;
      case 'round-ms': opts.roundMs = Math.max(1, int(rawValue)); break;
      case 'steady-round-ms': opts.steadyRoundMs = Math.max(1, int(rawValue)); break;
      case 'spawn-interval-ms': opts.spawnIntervalMs = Math.max(1, int(rawValue)); break;
      case 'steady-rounds': opts.steadyRounds = Math.max(0, int(rawValue)); break;
      case 'recv-ratio': opts.recvRatio = Math.min(1, Math.max(0, Number.parseFloat(rawValue))); break;
      case 'max-processes': opts.maxProcesses = Math.max(0, int(rawValue)); break;
      case 'prefix': opts.prefix = rawValue || opts.prefix; break;
      case 'perf': opts.perf = true; break;
      case 'no-perf': opts.perf = false; break;
      case 'perf-interval-ms': opts.perfIntervalMs = Math.max(1000, int(rawValue)); break;
      case 'perf-output-dir': opts.perfOutputDir = rawValue || opts.perfOutputDir; break;
      case 'perf-frequency': opts.perfFrequency = Math.max(1, int(rawValue)); break;
      case 'perf-percent-limit': opts.perfPercentLimit = Math.max(0, Number.parseFloat(rawValue)); break;
      default:
        throw new Error(`unknown option: --${key}`);
    }
  }

  return opts;
}

function updateRunStateFromLine(runState, prefix, line) {
  runState.lastPressureLine = `[${prefix}] ${line}`;
  const phaseMatch = line.match(/\b(ramp|steady#\d+)\b/);
  if (phaseMatch) runState.phase = phaseMatch[1];

  const nodeRoomsMatch = line.match(/\bnodeRooms=(\d+)/);
  if (nodeRoomsMatch) {
    runState.lastRooms = Number.parseInt(nodeRoomsMatch[1], 10);
  }

  const shardRoomsMatch = line.match(/\[rooms=(\d+)\]/);
  if (shardRoomsMatch) {
    const shard = prefix.split(':')[0];
    runState.lastShardRooms.set(shard, Number.parseInt(shardRoomsMatch[1], 10));
  }
}

function prefixLog(prefix, chunk, sink, runState = null) {
  const text = chunk.toString('utf8').replace(/\r?\n$/, '');
  for (const line of text.split('\n')) {
    if (line.length === 0) continue;
    if (runState) updateRunStateFromLine(runState, prefix, line);
    sink.write(`[${prefix}] ${line}\n`);
  }
}

function spawnShard(index, opts, runState) {
  const roomPrefix = `${opts.prefix}_p${index}_${Date.now()}`;
  const args = [
    childScript,
    `--ws-url=${opts.wsUrl}`,
    `--http-url=${opts.httpUrl}`,
    `--container=${opts.container}`,
    `--sample-host=${opts.sampleHost}`,
    `--max-rooms=${opts.roomsPerProcess}`,
    `--step=${opts.step}`,
    `--round-ms=${opts.roundMs}`,
    ...(opts.steadyRoundMs ? [`--steady-round-ms=${opts.steadyRoundMs}`] : []),
    `--recv-ratio=${opts.recvRatio}`,
    '--hold-after-max',
    `--steady-rounds=${opts.steadyRounds}`,
    `--room-prefix=${roomPrefix}`,
  ];

  const child = spawn(process.execPath, args, {
    cwd: repoRoot,
    stdio: ['ignore', 'pipe', 'pipe'],
  });

  child.stdout.on('data', chunk => prefixLog(`p${index}:${child.pid}`, chunk, process.stdout, runState));
  child.stderr.on('data', chunk => prefixLog(`p${index}:${child.pid}`, chunk, process.stderr, runState));

  return child;
}

function compactTimestamp(date = new Date()) {
  const pad = value => String(value).padStart(2, '0');
  return `${date.getFullYear()}${pad(date.getMonth() + 1)}${pad(date.getDate())}-${pad(date.getHours())}-${pad(date.getMinutes())}-${pad(date.getSeconds())}`;
}

async function runCommand(command, args, options = {}) {
  return await new Promise(resolve => {
    const child = spawn(command, args, {
      stdio: ['ignore', 'pipe', 'pipe'],
      ...options,
    });
    let stdout = '';
    let stderr = '';
    child.stdout.on('data', chunk => { stdout += chunk.toString('utf8'); });
    child.stderr.on('data', chunk => { stderr += chunk.toString('utf8'); });
    child.once('error', error => resolve({ code: -1, stdout, stderr: error.message }));
    child.once('exit', (code, signal) => resolve({ code: code ?? 0, signal, stdout, stderr }));
  });
}

async function findWorkerHostPid(container) {
  const result = await runCommand('docker', ['top', container, '-eo', 'pid,comm,args']);
  if (result.code !== 0) {
    throw new Error(`docker top failed: ${result.stderr.trim() || result.stdout.trim()}`);
  }

  for (const line of result.stdout.split('\n')) {
    const parts = line.trim().split(/\s+/, 3);
    if (parts.length >= 2 && parts[1] === 'mediasoup-worke') {
      return parts[0];
    }
  }

  throw new Error(`mediasoup worker process not found in container ${container}`);
}

function snapshotRunState(runState) {
  return {
    children: runState.children,
    phase: runState.phase,
    lastRooms: runState.lastRooms,
    lastPressureLine: runState.lastPressureLine,
    lastShardRooms: Object.fromEntries(runState.lastShardRooms),
  };
}

async function sampleNodeRooms(httpUrl) {
  const url = new URL('/api/node-load', httpUrl);
  const result = await runCommand('curl', ['-fsS', url.toString()]);
  if (result.code !== 0) {
    return { error: result.stderr.trim() || result.stdout.trim() };
  }

  try {
    const parsed = JSON.parse(result.stdout);
    return {
      rooms: parsed.rooms,
      dispatchRooms: parsed.dispatchRooms,
      healthy: parsed.healthy,
      ready: parsed.ready,
      workerQueueStats: parsed.workerQueueStats,
    };
  } catch (error) {
    return { error: error.message, raw: result.stdout.slice(0, 200) };
  }
}

function formatMetaValue(value) {
  return typeof value === 'string' ? value : JSON.stringify(value);
}

async function appendMeta(metaPath, values) {
  const lines = Object.entries(values).map(([key, value]) => `${key}=${formatMetaValue(value)}`);
  await fs.appendFile(metaPath, `${lines.join('\n')}\n`);
}

async function recordPerfSample(opts, outputDir, runState) {
  const startedAt = new Date();
  const basePath = path.join(outputDir, compactTimestamp(startedAt));
  const dataPath = `${basePath}.data`;
  const reportPath = `${basePath}.report`;
  const metaPath = `${basePath}.meta`;
  const workerPid = await findWorkerHostPid(opts.container);
  const stateBefore = snapshotRunState(runState);
  const nodeBefore = await sampleNodeRooms(opts.httpUrl);

  await fs.writeFile(
    metaPath,
    [
      `startedAt=${startedAt.toISOString()}`,
      `container=${opts.container}`,
      `workerHostPid=${workerPid}`,
      `frequency=${opts.perfFrequency}`,
      `durationMs=${opts.perfIntervalMs}`,
      `dataPath=${dataPath}`,
      `reportPath=${reportPath}`,
      `stateBefore=${JSON.stringify(stateBefore)}`,
      `nodeBefore=${JSON.stringify(nodeBefore)}`,
      '',
    ].join('\n')
  );

  console.log(
    `[perf] start ts=${compactTimestamp(startedAt)} pid=${workerPid} ` +
    `phase=${stateBefore.phase} rooms=${nodeBefore.rooms ?? stateBefore.lastRooms ?? 'n/a'} ` +
    `data=${dataPath}`
  );

  const seconds = Math.max(1, Math.ceil(opts.perfIntervalMs / 1000));
  const record = await runCommand('perf', [
    'record',
    '-F', String(opts.perfFrequency),
    '-g',
    '-p', workerPid,
    '-o', dataPath,
    '--',
    'sleep', String(seconds),
  ]);
  const endedAt = new Date();
  const stateAfter = snapshotRunState(runState);
  const nodeAfter = await sampleNodeRooms(opts.httpUrl);
  await appendMeta(metaPath, {
    endedAt: endedAt.toISOString(),
    stateAfter,
    nodeAfter,
    recordExit: record.code,
    recordStderr: record.stderr,
  });

  const report = await runCommand('perf', [
    'report',
    '-i', dataPath,
    '--stdio',
    '--percent-limit', String(opts.perfPercentLimit),
    '--sort', 'symbol,dso',
    '--no-children',
  ]);
  await fs.writeFile(reportPath, report.stdout + report.stderr);
  await appendMeta(metaPath, {
    reportExit: report.code,
    reportStderr: report.stderr,
  });

  const status = record.code === 0 ? 'ok' : `record-exit-${record.code}`;
  console.log(
    `[perf] done ${status} ts=${compactTimestamp(startedAt)} pid=${workerPid} ` +
    `phase=${stateAfter.phase} rooms=${nodeAfter.rooms ?? stateAfter.lastRooms ?? 'n/a'} ` +
    `data=${dataPath} report=${reportPath}`
  );
}

async function runPerfLoop(opts, shouldStop, runState) {
  const outputDir = opts.perfOutputDir || `/tmp/mediasoup-perf-${compactTimestamp()}`;
  await fs.mkdir(outputDir, { recursive: true });
  console.log(`[perf] enabled intervalMs=${opts.perfIntervalMs} outputDir=${outputDir}`);

  while (!shouldStop()) {
    try {
      await recordPerfSample(opts, outputDir, runState);
    } catch (error) {
      console.error(`[perf] failed: ${error.message}`);
      await sleep(Math.min(opts.perfIntervalMs, 5000));
    }
  }
}

async function main() {
  const opts = parseArgs(process.argv.slice(2));
  console.log(`ws=${opts.wsUrl}`);
  console.log(`http=${opts.httpUrl}`);
  console.log(`container=${opts.container}`);
  console.log(`sampleHost=${opts.sampleHost || 'local'}`);
  console.log(`config roomsPerProcess=${opts.roomsPerProcess} step=${opts.step} roundMs=${opts.roundMs} steadyRoundMs=${opts.steadyRoundMs ?? opts.roundMs} spawnIntervalMs=${opts.spawnIntervalMs} steadyRounds=${opts.steadyRounds} recvRatio=${opts.recvRatio} maxProcesses=${opts.maxProcesses || 'unbounded'}`);

  const children = [];
  let nextIndex = 1;
  let stopping = false;
  let perfLoop = null;
  const runState = {
    children: 0,
    lastPressureLine: '',
    lastRooms: null,
    lastShardRooms: new Map(),
    phase: 'init',
  };

  const stopAll = async () => {
    if (stopping) return;
    stopping = true;
    for (const child of children) {
      if (!child.pid || child.exitCode !== null) continue;
      try { child.kill('SIGTERM'); } catch {}
    }
    await sleep(2000);
    for (const child of children) {
      if (!child.pid || child.exitCode !== null) continue;
      try { child.kill('SIGKILL'); } catch {}
    }
  };

  process.on('SIGINT', () => { void stopAll(); });
  process.on('SIGTERM', () => { void stopAll(); });

  const waitForExit = child => new Promise(resolve => child.once('exit', (code, signal) => resolve({ code, signal })));

  try {
    if (opts.perf) {
      perfLoop = runPerfLoop(opts, () => stopping, runState);
    }

    while (!stopping && (opts.maxProcesses === 0 || children.length < opts.maxProcesses)) {
      runState.phase = 'spawn';
      const child = spawnShard(nextIndex++, opts, runState);
      children.push(child);
      runState.children = children.length;
      console.log(`spawned shard pid=${child.pid} total=${children.length}`);

      child.once('exit', async (code, signal) => {
        if (stopping) return;
        console.error(`shard pid=${child.pid} exited code=${code} signal=${signal ?? 'none'}`);
        await stopAll();
        process.exitCode = code === 0 ? 1 : code || 1;
      });

      if (opts.maxProcesses !== 0 && children.length >= opts.maxProcesses) {
        break;
      }

      await sleep(opts.spawnIntervalMs);
    }

    if (!stopping) {
      runState.phase = 'steady';
      console.log(`spawn phase complete, children=${children.length}`);
      const exits = children.map(child => waitForExit(child));
      const result = await Promise.race(exits);
      if (!stopping) {
        console.error(`pressure run ended: code=${result.code} signal=${result.signal ?? 'none'}`);
        process.exitCode = result.code === 0 ? 1 : result.code || 1;
      }
    }
  } finally {
    await stopAll();
    if (perfLoop) {
      await perfLoop;
    }
  }
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
