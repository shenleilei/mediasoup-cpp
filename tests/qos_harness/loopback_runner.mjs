import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const esbuild = require('esbuild');
const puppeteer = require('puppeteer-core');

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const chromiumPath = '/usr/lib64/chromium-browser/headless_shell';
const tcPath = '/usr/sbin/tc';

function runTc(args) {
  execFileSync(tcPath, args, { stdio: 'inherit' });
}

export function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

export function stateRank(state) {
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

export function clearNetem() {
  try {
    execFileSync(tcPath, ['qdisc', 'del', 'dev', 'lo', 'root'], { stdio: 'ignore' });
  } catch {}
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

export async function launchBrowser() {
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

export function applyNetemConfig(config = {}) {
  const { bandwidth, rtt, loss, jitter } = config;
  clearNetem();

  const netemArgs = ['qdisc', 'replace', 'dev', 'lo', 'root', 'handle', '1:', 'netem'];
  // The matrix uses round-trip RTT targets, while netem delay is one-way.
  const delayBase = typeof rtt === 'number' ? Math.max(1, Math.round(rtt / 2)) : 1;
  netemArgs.push('delay', `${delayBase}ms`);
  if (typeof jitter === 'number' && jitter > 0) {
    netemArgs.push(`${Math.round(jitter)}ms`, 'distribution', 'normal');
  }
  if (typeof loss === 'number' && loss > 0) {
    netemArgs.push('loss', `${loss}%`);
  }
  runTc(netemArgs);

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
}

export async function createLoopbackHarness() {
  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-browser-'));
  const bundlePath = buildBundle(tmpDir);
  const browser = await launchBrowser();
  const page = await browser.newPage();
  const diagnostics = {
    console: [],
    pageErrors: [],
    requestFailures: [],
    pageCrashes: [],
    pageClosed: false,
    browserDisconnected: false,
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
  page.on('error', error => {
    diagnostics.pageCrashes.push(error?.stack || error?.message || String(error));
  });
  page.on('close', () => {
    diagnostics.pageClosed = true;
  });
  browser.on('disconnected', () => {
    diagnostics.browserDisconnected = true;
  });
  const bundleCode = fs.readFileSync(bundlePath, 'utf8');
  await page.setContent(
    '<!doctype html><html><head><meta charset="utf-8"><title>QoS Loopback</title></head><body></body></html>',
    { waitUntil: 'load' }
  );
  await page.addScriptTag({ content: bundleCode });
  await page.evaluate(() => window.__qosLoopbackHarness.init());

  return {
    async nowMs() {
      return page.evaluate(() => Date.now());
    },
    async getState() {
      return page.evaluate(() => window.__qosLoopbackHarness.getState());
    },
    async stop() {
      try {
        await page.evaluate(() => window.__qosLoopbackHarness.stop());
      } catch {}
      await browser.close();
      clearNetem();
      fs.rmSync(tmpDir, { recursive: true, force: true });
    },
    async getDiagnostics() {
      return JSON.parse(JSON.stringify(diagnostics));
    },
  };
}
