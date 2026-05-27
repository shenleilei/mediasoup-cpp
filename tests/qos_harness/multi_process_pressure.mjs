import { spawn } from 'node:child_process';
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
    wsUrl: 'ws://127.0.0.1:1770/ws',
    httpUrl: 'http://127.0.0.1:1770',
    container: 'mediasoup-cpp',
    sampleHost: '',
    roomsPerProcess: 100,
    step: 10,
    roundMs: 10000,
    spawnIntervalMs: 10000,
    steadyRounds: 0,
    recvRatio: 0.85,
    maxProcesses: 0,
    prefix: `multi_pressure_${Date.now()}`,
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
      case 'spawn-interval-ms': opts.spawnIntervalMs = Math.max(1, int(rawValue)); break;
      case 'steady-rounds': opts.steadyRounds = Math.max(0, int(rawValue)); break;
      case 'recv-ratio': opts.recvRatio = Math.min(1, Math.max(0, Number.parseFloat(rawValue))); break;
      case 'max-processes': opts.maxProcesses = Math.max(0, int(rawValue)); break;
      case 'prefix': opts.prefix = rawValue || opts.prefix; break;
      default:
        throw new Error(`unknown option: --${key}`);
    }
  }

  return opts;
}

function prefixLog(prefix, chunk, sink) {
  const text = chunk.toString('utf8').replace(/\r?\n$/, '');
  for (const line of text.split('\n')) {
    if (line.length === 0) continue;
    sink.write(`[${prefix}] ${line}\n`);
  }
}

function spawnShard(index, opts) {
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
    `--recv-ratio=${opts.recvRatio}`,
    '--hold-after-max',
    `--steady-rounds=${opts.steadyRounds}`,
    `--room-prefix=${roomPrefix}`,
  ];

  const child = spawn(process.execPath, args, {
    cwd: repoRoot,
    stdio: ['ignore', 'pipe', 'pipe'],
  });

  child.stdout.on('data', chunk => prefixLog(`p${index}:${child.pid}`, chunk, process.stdout));
  child.stderr.on('data', chunk => prefixLog(`p${index}:${child.pid}`, chunk, process.stderr));

  return child;
}

async function main() {
  const opts = parseArgs(process.argv.slice(2));
  console.log(`ws=${opts.wsUrl}`);
  console.log(`http=${opts.httpUrl}`);
  console.log(`container=${opts.container}`);
  console.log(`sampleHost=${opts.sampleHost || 'local'}`);
  console.log(`config roomsPerProcess=${opts.roomsPerProcess} step=${opts.step} roundMs=${opts.roundMs} spawnIntervalMs=${opts.spawnIntervalMs} steadyRounds=${opts.steadyRounds} recvRatio=${opts.recvRatio} maxProcesses=${opts.maxProcesses || 'unbounded'}`);

  const children = [];
  let nextIndex = 1;
  let stopping = false;

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
    while (!stopping && (opts.maxProcesses === 0 || children.length < opts.maxProcesses)) {
      const child = spawnShard(nextIndex++, opts);
      children.push(child);
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
  }
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
