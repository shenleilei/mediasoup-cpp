import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn } from 'node:child_process';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const esbuild = require('esbuild');
const puppeteer = require('puppeteer-core');

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');

function resolveChromiumPath() {
  if (process.env.CHROMIUM_PATH) {
    return process.env.CHROMIUM_PATH;
  }

  try {
    const cacheRoot = path.join(os.homedir(), '.cache', 'puppeteer', 'chrome');
    if (fs.existsSync(cacheRoot)) {
      const stack = [cacheRoot];
      while (stack.length > 0) {
        const current = stack.pop();
        const entries = fs.readdirSync(current, { withFileTypes: true });
        for (const entry of entries) {
          const full = path.join(current, entry.name);
          if (entry.isDirectory()) {
            stack.push(full);
          } else if (entry.isFile() && entry.name === 'chrome') {
            return full;
          }
        }
      }
    }
  } catch {}

  const candidates = [
    '/usr/lib64/chromium-browser/headless_shell',
    '/usr/bin/headless_shell',
    '/usr/bin/chromium-browser',
    '/usr/bin/chromium',
    '/usr/bin/google-chrome',
    '/usr/bin/google-chrome-stable',
  ];

  for (const candidate of candidates) {
    try {
      if (fs.existsSync(candidate)) {
        return candidate;
      }
    } catch {}
  }

  return candidates[0];
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function withTimeout(label, promise, timeoutMs) {
  let timer = null;
  try {
    return await Promise.race([
      promise,
      new Promise((_, reject) => {
        timer = setTimeout(() => reject(new Error(`${label} timed out after ${timeoutMs} ms`)), timeoutMs);
      }),
    ]);
  } finally {
    if (timer) {
      clearTimeout(timer);
    }
  }
}

function formatMbps(bps) {
  return `${(bps / 1_000_000).toFixed(2)} Mbps`;
}

function parseIntegerArg(value, name) {
  const parsed = Number.parseInt(value, 10);
  if (!Number.isFinite(parsed) || parsed < 0) {
    throw new Error(`invalid ${name}: ${value}`);
  }
  return parsed;
}

function parseArgs(argv) {
  const defaults = {
    port: 0,
    wsUrl: '',
    workers: 1,
    step: 5,
    maxRooms: 50,
    churnCycles: 0,
    churnBatch: 1,
    settleMs: 3_000,
    sampleMs: 8_000,
    createConcurrency: 4,
    width: 1920,
    height: 1080,
    fps: 30,
    targetBitrateBps: 6_000_000,
    minSendBitrateBps: 1_500_000,
    minRecvBitrateBps: 1_500_000,
    minWidth: 1600,
    minHeight: 900,
    roomPrefix: `capacity_room_${Date.now()}`,
  };

  for (const arg of argv) {
    if (arg === '-h' || arg === '--help') {
      defaults.help = true;
      continue;
    }

    if (!arg.startsWith('--')) {
      throw new Error(`unknown positional argument: ${arg}`);
    }

    const [key, rawValue = ''] = arg.slice(2).split('=');
    switch (key) {
      case 'port':
        defaults.port = parseIntegerArg(rawValue, 'port');
        break;
      case 'ws-url':
        defaults.wsUrl = rawValue;
        break;
      case 'workers':
        defaults.workers = Math.max(1, parseIntegerArg(rawValue, 'workers'));
        break;
      case 'step':
        defaults.step = Math.max(1, parseIntegerArg(rawValue, 'step'));
        break;
      case 'max-rooms':
        defaults.maxRooms = Math.max(1, parseIntegerArg(rawValue, 'max-rooms'));
        break;
      case 'churn-cycles':
        defaults.churnCycles = Math.max(0, parseIntegerArg(rawValue, 'churn-cycles'));
        break;
      case 'churn-batch':
        defaults.churnBatch = Math.max(1, parseIntegerArg(rawValue, 'churn-batch'));
        break;
      case 'settle-ms':
        defaults.settleMs = parseIntegerArg(rawValue, 'settle-ms');
        break;
      case 'sample-ms':
        defaults.sampleMs = Math.max(1, parseIntegerArg(rawValue, 'sample-ms'));
        break;
      case 'create-concurrency':
        defaults.createConcurrency = Math.max(1, parseIntegerArg(rawValue, 'create-concurrency'));
        break;
      case 'width':
        defaults.width = Math.max(1, parseIntegerArg(rawValue, 'width'));
        break;
      case 'height':
        defaults.height = Math.max(1, parseIntegerArg(rawValue, 'height'));
        break;
      case 'fps':
        defaults.fps = Math.max(1, parseIntegerArg(rawValue, 'fps'));
        break;
      case 'target-bitrate-bps':
        defaults.targetBitrateBps = parseIntegerArg(rawValue, 'target-bitrate-bps');
        break;
      case 'min-send-bitrate-bps':
        defaults.minSendBitrateBps = parseIntegerArg(rawValue, 'min-send-bitrate-bps');
        break;
      case 'min-recv-bitrate-bps':
        defaults.minRecvBitrateBps = parseIntegerArg(rawValue, 'min-recv-bitrate-bps');
        break;
      case 'min-width':
        defaults.minWidth = Math.max(1, parseIntegerArg(rawValue, 'min-width'));
        break;
      case 'min-height':
        defaults.minHeight = Math.max(1, parseIntegerArg(rawValue, 'min-height'));
        break;
      case 'room-prefix':
        defaults.roomPrefix = rawValue || defaults.roomPrefix;
        break;
      default:
        throw new Error(`unknown option: --${key}`);
    }
  }

  return defaults;
}

function printUsage() {
  console.log(`Usage:
  node tests/qos_harness/browser_capacity_rooms.mjs [options]

Options:
  --workers=1
  --step=5
  --max-rooms=50
  --churn-cycles=0
  --churn-batch=1
  --settle-ms=3000
  --sample-ms=8000
  --create-concurrency=4
  --width=1920
  --height=1080
  --fps=30
  --target-bitrate-bps=6000000
  --min-send-bitrate-bps=1500000
  --min-recv-bitrate-bps=1500000
  --min-width=1600
  --min-height=900
  --room-prefix=capacity_room
  --port=0
  --ws-url=ws://127.0.0.1:1770/ws
`);
}

async function allocatePort() {
  const net = await import('node:net');
  return await new Promise((resolve, reject) => {
    const server = net.createServer();
    server.once('error', reject);
    server.listen(0, '127.0.0.1', () => {
      const { port } = server.address();
      server.close(error => error ? reject(error) : resolve(port));
    });
  });
}

async function buildBundle(tmpDir) {
  const outfile = path.join(tmpDir, 'bundle.js');
  await esbuild.build({
    entryPoints: [path.join(__dirname, 'browser', 'capacity-rooms-entry.js')],
    outfile,
    bundle: true,
    platform: 'browser',
    format: 'iife',
    target: ['chrome120'],
    nodePaths: [path.join(__dirname, 'node_modules')],
    plugins: [
      {
        name: 'npm-events-package-shim',
        setup(build) {
          build.onResolve({ filter: /^npm-events-package$/ }, () => ({
            path: path.join(__dirname, 'shims', 'npm-events-package.js'),
          }));
        },
      },
    ],
  });
  return outfile;
}

function startSfu({ port, workers }) {
  const child = spawn(
    path.join(repoRoot, 'build', 'mediasoup-sfu'),
    [
      '--nodaemon',
      `--port=${port}`,
      `--workers=${workers}`,
      '--workerBin=./mediasoup-worker',
      '--announcedIp=127.0.0.1',
      '--listenIp=127.0.0.1',
      '--redisHost=0.0.0.0',
      '--redisPort=1',
      '--noRedisRequired',
    ],
    {
      cwd: repoRoot,
      stdio: ['ignore', 'pipe', 'pipe'],
    }
  );

  child.stdout.on('data', () => {});
  child.stderr.on('data', () => {});
  return child;
}

function stopSfu(child, timeoutMs = 3000) {
  return new Promise(resolve => {
    if (!child.pid || child.exitCode !== null) {
      resolve();
      return;
    }
    const timer = setTimeout(() => {
      try {
        child.kill('SIGKILL');
      } catch {}
      resolve();
    }, timeoutMs);
    child.once('close', () => {
      clearTimeout(timer);
      resolve();
    });
    child.kill('SIGTERM');
  });
}

async function waitForPort(port, timeoutMs = 7000) {
  const net = await import('node:net');
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      await new Promise((resolve, reject) => {
        const socket = net.createConnection({ host: '127.0.0.1', port }, () => {
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
  throw new Error(`port ${port} not ready`);
}

async function createHarnessPage(browser, bundlePath) {
  const page = await browser.newPage();
  page.setDefaultTimeout(120_000);
  page.setDefaultNavigationTimeout(120_000);
  const diagnostics = {
    console: [],
    pageErrors: [],
    requestFailures: [],
  };

  page.on('console', message => {
    diagnostics.console.push({
      type: message.type(),
      text: message.text(),
    });
  });
  page.on('pageerror', error => {
    diagnostics.pageErrors.push(error?.stack || error?.message || String(error));
  });
  page.on('requestfailed', request => {
    diagnostics.requestFailures.push({
      url: request.url(),
      failure: request.failure()?.errorText || 'unknown',
    });
  });

  const bundleCode = fs.readFileSync(bundlePath, 'utf8');
  const inlineBundle = bundleCode.replace(/<\/script/gi, '<\\/script');
  await withTimeout(
    'page.setContent',
    page.setContent(
      `<!doctype html><html><head><meta charset="utf-8"><title>Capacity Rooms</title></head><body><script>${inlineBundle}</script></body></html>`,
      { waitUntil: 'domcontentloaded' }
    ),
    45_000
  );

  page.__diagnostics = diagnostics;
  return page;
}

function printStep(step) {
  console.log(
    `[rooms=${step.totalRooms}] healthy=${step.sample.healthyRooms}/${step.sample.totalRooms}` +
      ` send(avg/min)=${formatMbps(step.sample.sendBitrateAvgBps)}/${formatMbps(step.sample.sendBitrateMinBps)}` +
      ` recv(avg/min)=${formatMbps(step.sample.recvBitrateAvgBps)}/${formatMbps(step.sample.recvBitrateMinBps)}`
  );

  if (!step.sample.healthy) {
    const unhealthy = step.sample.details.filter(item => !item.healthy).slice(0, 5);
    for (const item of unhealthy) {
      console.log(`  FAIL ${item.roomId}: ${item.reasons.join(', ')}`);
    }
  }
}

async function run() {
  const options = parseArgs(process.argv.slice(2));
  if (options.help) {
    printUsage();
    return;
  }

  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-capacity-rooms-'));
  const bundlePath = await buildBundle(tmpDir);
  let signalingPort = options.port;
  let sfu = null;
  const chromiumPath = resolveChromiumPath();
  if (!options.wsUrl) {
    signalingPort = options.port > 0 ? options.port : await allocatePort();
    options.port = signalingPort;
    sfu = startSfu({ port: signalingPort, workers: options.workers });
  }

  const browser = await puppeteer.launch({
    executablePath: chromiumPath,
    headless: true,
    protocolTimeout: 10 * 60 * 1000,
    args: [
      '--no-sandbox',
      '--autoplay-policy=no-user-gesture-required',
    ],
  });
  console.log(`browser launched with ${chromiumPath}`);

  const page = await createHarnessPage(browser, bundlePath);
  console.log('harness page ready');
  const steps = [];
  let lastHealthyRooms = 0;
  let failureStep = null;

  try {
    if (sfu) {
      await waitForPort(signalingPort);
    }
    const wsUrl = options.wsUrl || `ws://127.0.0.1:${signalingPort}/ws`;
    console.log(`connecting harness to ${wsUrl}`);
    await page.evaluate(
      (url, config) => window.__capacityRoomsHarness.init(url, config),
      wsUrl,
      {
        roomPrefix: options.roomPrefix,
        subscriberCount: 2,
        width: options.width,
        height: options.height,
        fps: options.fps,
        targetBitrateBps: options.targetBitrateBps,
        minSendBitrateBps: options.minSendBitrateBps,
        minRecvBitrateBps: options.minRecvBitrateBps,
        minWidth: options.minWidth,
        minHeight: options.minHeight,
        createConcurrency: options.createConcurrency,
      }
    );
    console.log('harness init complete');

    let activeRooms = 0;
    while (activeRooms < options.maxRooms) {
      const addCount = Math.min(options.step, options.maxRooms - activeRooms);
      console.log(`adding ${addCount} room(s), active=${activeRooms}`);
      await withTimeout(
        `addRooms(+${addCount})`,
        page.evaluate(count => window.__capacityRoomsHarness.addRooms(count), addCount),
        Math.max(60_000, addCount * 20_000)
      );
      activeRooms += addCount;

      if (options.settleMs > 0) {
        await sleep(options.settleMs);
      }

      const sample = await withTimeout(
        `sample(${activeRooms})`,
        page.evaluate(sampleMs => window.__capacityRoomsHarness.sample(sampleMs), options.sampleMs),
        options.sampleMs + 60_000
      );

      const step = {
        totalRooms: activeRooms,
        sample,
      };
      steps.push(step);
      printStep(step);

      if (!sample.healthy) {
        failureStep = step;
        break;
      }

      lastHealthyRooms = activeRooms;
    }

    if (options.churnCycles > 0) {
      for (let cycle = 0; cycle < options.churnCycles; cycle += 1) {
        const churnCount = Math.min(options.churnBatch, activeRooms);
        if (churnCount > 0) {
          console.log(`churn cycle ${cycle + 1}: removing ${churnCount} room(s)`);
          await withTimeout(
            `removeRooms(${churnCount})`,
            page.evaluate(count => window.__capacityRoomsHarness.removeRooms(count), churnCount),
            Math.max(60_000, churnCount * 15_000)
          );
          activeRooms -= churnCount;
        }

        await sleep(options.settleMs);

        console.log(`churn cycle ${cycle + 1}: re-adding ${churnCount} room(s)`);
        await withTimeout(
          `reAddRooms(+${churnCount})`,
          page.evaluate(count => window.__capacityRoomsHarness.addRooms(count), churnCount),
          Math.max(60_000, churnCount * 20_000)
        );
        activeRooms += churnCount;

        if (options.settleMs > 0) {
          await sleep(options.settleMs);
        }

        const sample = await withTimeout(
          `sample(churn:${cycle + 1})`,
          page.evaluate(sampleMs => window.__capacityRoomsHarness.sample(sampleMs), options.sampleMs),
          options.sampleMs + 60_000
        );
        console.log(`churn cycle ${cycle + 1} sample complete`);

        const step = {
          totalRooms: activeRooms,
          sample,
          churnCycle: cycle + 1,
        };
        steps.push(step);
        printStep(step);

        if (!sample.healthy) {
          failureStep = step;
          break;
        }
      }
    }

    const result = {
      config: options,
      lastHealthyRooms,
      lastHealthyRoutes: lastHealthyRooms,
      failureAtRooms: failureStep?.totalRooms ?? null,
      failureAtRoutes: failureStep?.totalRooms ?? null,
      steps,
      diagnostics: page.__diagnostics,
    };

    console.log('');
    console.log(`last healthy capacity: ${lastHealthyRooms} room(s) / ${lastHealthyRooms} route(s)`);
    if (failureStep) {
      console.log(`first failing step: ${failureStep.totalRooms} room(s) / ${failureStep.totalRooms} route(s)`);
    } else {
      console.log(`no failure observed up to ${options.maxRooms} room(s)`);
    }

    if (process.env.QOS_PRINT_JSON === '1') {
      console.log(JSON.stringify(result, null, 2));
    }
  } finally {
    try {
      await page.evaluate(() => window.__capacityRoomsHarness.stop());
    } catch {}
    await browser.close();
    await stopSfu(sfu);
    fs.rmSync(tmpDir, { recursive: true, force: true });
  }
}

run().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
