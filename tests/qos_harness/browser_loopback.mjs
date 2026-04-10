import fs from 'node:fs';
import http from 'node:http';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFileSync, spawn } from 'node:child_process';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const esbuild = require('esbuild');
const puppeteer = require('puppeteer-core');

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const chromiumPath = '/usr/lib64/chromium-browser/headless_shell';
const tcPath = '/usr/sbin/tc';

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function stateRank(state) {
  switch (state) {
    case 'stable':
      return 0;
    case 'early_warning':
      return 1;
    case 'recovering':
      return 2;
    case 'congested':
      return 3;
    default:
      return 99;
  }
}

function buildBundle(tmpDir) {
  const outfile = path.join(tmpDir, 'bundle.js');

  esbuild.buildSync({
    entryPoints: [path.join(__dirname, 'browser', 'loopback-entry.js')],
    outfile,
    bundle: true,
    platform: 'browser',
    format: 'iife',
    target: ['chrome120'],
  });

  return outfile;
}

function startStaticServer(bundlePath) {
  const html = `<!doctype html>
<html>
  <head><meta charset="utf-8"><title>QoS Loopback</title></head>
  <body>
    <script src="/bundle.js"></script>
  </body>
</html>`;

  const server = http.createServer((req, res) => {
    if (req.url === '/bundle.js') {
      res.writeHead(200, { 'content-type': 'application/javascript' });
      res.end(fs.readFileSync(bundlePath));
      return;
    }

    res.writeHead(200, { 'content-type': 'text/html' });
    res.end(html);
  });

  return new Promise(resolve => {
    server.listen(0, '127.0.0.1', () => {
      resolve(server);
    });
  });
}

function getServerUrl(server) {
  const address = server.address();
  return `http://127.0.0.1:${address.port}`;
}

function clearNetem() {
  try {
    execFileSync(tcPath, ['qdisc', 'del', 'dev', 'lo', 'root'], {
      stdio: 'ignore',
    });
  } catch {}
}

function applyUdpNetem({ delayMs = 120, lossPct = 10 }) {
  clearNetem();
  execFileSync(tcPath, ['qdisc', 'add', 'dev', 'lo', 'root', 'handle', '1:', 'prio'], {
    stdio: 'inherit',
  });
  execFileSync(
    tcPath,
    [
      'qdisc',
      'add',
      'dev',
      'lo',
      'parent',
      '1:3',
      'handle',
      '30:',
      'netem',
      'delay',
      `${delayMs}ms`,
      'loss',
      `${lossPct}%`,
    ],
    { stdio: 'inherit' }
  );
  execFileSync(
    tcPath,
    [
      'filter',
      'add',
      'dev',
      'lo',
      'protocol',
      'ip',
      'parent',
      '1:0',
      'prio',
      '3',
      'u32',
      'match',
      'ip',
      'protocol',
      '17',
      '0xff',
      'flowid',
      '1:3',
    ],
    { stdio: 'inherit' }
  );
}

async function launchBrowser() {
  return puppeteer.launch({
    executablePath: chromiumPath,
    headless: true,
    args: [
      '--no-sandbox',
      '--autoplay-policy=no-user-gesture-required',
      '--use-fake-device-for-media-stream',
    ],
  });
}

async function runScenario() {
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-browser-'));
  const bundlePath = buildBundle(tmpDir);
  const server = await startStaticServer(bundlePath);
  const browser = await launchBrowser();
  const page = await browser.newPage();

  try {
    await page.goto(getServerUrl(server), { waitUntil: 'load' });
    await page.evaluate(() => window.__qosLoopbackHarness.init());

    await sleep(2000);
    const baseline = await page.evaluate(() => window.__qosLoopbackHarness.getState());

    applyUdpNetem({ delayMs: 120, lossPct: 10 });
    await sleep(3000);
    const impaired = await page.evaluate(() => window.__qosLoopbackHarness.getState());
    clearNetem();

    await sleep(8000);
    const recovered = await page.evaluate(() => window.__qosLoopbackHarness.getState());

    const degraded =
      impaired.level > baseline.level ||
      stateRank(impaired.state) > stateRank(baseline.state) ||
      impaired.state === 'congested' ||
      impaired.trace.some(entry => entry.plannedAction.type !== 'noop');
    if (!degraded) {
      throw new Error(`browser loopback did not degrade under netem: ${JSON.stringify(impaired)}`);
    }

    if (recovered.state === 'congested') {
      throw new Error(`browser loopback remained congested after netem removal: ${JSON.stringify(recovered)}`);
    }

    if (
      stateRank(recovered.state) > stateRank(impaired.state) &&
      recovered.level > impaired.level
    ) {
      throw new Error(`browser loopback did not recover after netem removal: ${JSON.stringify({ baseline, impaired, recovered })}`);
    }

    if (process.env.QOS_PRINT_JSON === '1') {
      console.log(
        JSON.stringify(
          {
            baseline,
            impaired,
            recovered,
            netem: {
              delayMs: 120,
              lossPct: 10,
            },
          },
          null,
          2
        )
      );
    }

    console.log('browser_loopback_e2e passed');
  } finally {
    clearNetem();
    try {
      await page.evaluate(() => window.__qosLoopbackHarness.stop());
    } catch {}
    await browser.close();
    await new Promise(resolve => server.close(resolve));
  }
}

runScenario().catch(error => {
  clearNetem();
  console.error(error);
  process.exitCode = 1;
});
