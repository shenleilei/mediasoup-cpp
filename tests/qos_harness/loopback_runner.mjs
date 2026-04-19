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
const PUPPETEER_PROTOCOL_TIMEOUT_MS = 10 * 60 * 1000;
const HARNESS_WARMUP_MS = 2000;
const MAX_DIAGNOSTIC_ITEMS = 200;
const MAX_RUNTIME_SNAPSHOTS = 48;
const MAX_KERNEL_LINES = 80;
const MAX_PROCESS_LINES = 40;

function runTc(args) {
  execFileSync(tcPath, args, { stdio: 'inherit' });
}

function nowIso() {
  return new Date().toISOString();
}

function roundToOne(value) {
  return typeof value === 'number'
    ? Number(value.toFixed(1))
    : null;
}

function toMb(bytes) {
  return typeof bytes === 'number'
    ? roundToOne(bytes / (1024 * 1024))
    : null;
}

function parseIntOrNull(value) {
  const parsed = Number.parseInt(String(value), 10);
  return Number.isFinite(parsed) ? parsed : null;
}

function pushCapped(list, value, limit = MAX_DIAGNOSTIC_ITEMS) {
  list.push(value);
  if (list.length > limit) {
    list.splice(0, list.length - limit);
  }
}

function safeReadText(filePath) {
  try {
    return fs.readFileSync(filePath, 'utf8');
  } catch {
    return null;
  }
}

function readNumericFile(filePath) {
  const value = safeReadText(filePath)?.trim();
  return value ? parseIntOrNull(value) : null;
}

function parseProcStatus(text) {
  if (!text) {
    return null;
  }

  const values = {};
  for (const line of text.split(/\r?\n/)) {
    const separator = line.indexOf(':');
    if (separator === -1) {
      continue;
    }
    const key = line.slice(0, separator);
    const rawValue = line.slice(separator + 1).trim();
    values[key] = rawValue;
  }

  return {
    name: values.Name ?? null,
    state: values.State ?? null,
    ppid: parseIntOrNull(values.PPid),
    threads: parseIntOrNull(values.Threads),
    vmRssKb: parseIntOrNull(values.VmRSS),
    vmSizeKb: parseIntOrNull(values.VmSize),
    vmSwapKb: parseIntOrNull(values.VmSwap),
  };
}

function getBrowserConnectionState(browser) {
  if (!browser) {
    return null;
  }
  if (typeof browser.connected === 'boolean') {
    return browser.connected;
  }
  if (typeof browser.isConnected === 'function') {
    return browser.isConnected();
  }
  return null;
}

function getBrowserPid(browser) {
  return browser?.process?.()?.pid ?? null;
}

function collectRuntimeSnapshot(browser) {
  const browserPid = getBrowserPid(browser);
  const memoryUsage = process.memoryUsage();
  const procStatus = browserPid
    ? parseProcStatus(safeReadText(`/proc/${browserPid}/status`))
    : null;

  return {
    capturedAt: nowIso(),
    host: {
      totalMemMb: toMb(os.totalmem()),
      freeMemMb: toMb(os.freemem()),
      loadAvg: os.loadavg().map(roundToOne),
      uptimeSec: Math.round(os.uptime()),
    },
    node: {
      pid: process.pid,
      rssMb: toMb(memoryUsage.rss),
      heapUsedMb: toMb(memoryUsage.heapUsed),
      heapTotalMb: toMb(memoryUsage.heapTotal),
      externalMb: toMb(memoryUsage.external),
      arrayBuffersMb: toMb(memoryUsage.arrayBuffers),
    },
    browser: {
      pid: browserPid,
      connected: getBrowserConnectionState(browser),
      alive: browserPid ? fs.existsSync(`/proc/${browserPid}`) : false,
      oomScore: browserPid ? readNumericFile(`/proc/${browserPid}/oom_score`) : null,
      oomScoreAdj: browserPid ? readNumericFile(`/proc/${browserPid}/oom_score_adj`) : null,
      procStatus,
    },
  };
}

function getRelevantProcessLines(browserPid = null) {
  try {
    const output = execFileSync(
      'ps',
      ['-eo', 'pid,ppid,stat,rss,vsz,comm,args'],
      { encoding: 'utf8', maxBuffer: 1024 * 1024 }
    );
    const interestingPids = new Set(
      [process.pid, browserPid].filter(pid => Number.isInteger(pid))
    );
    const lines = output
      .split(/\r?\n/)
      .map(line => line.trimEnd())
      .filter(Boolean);
    const header = lines.shift();
    const filtered = lines.filter(line => {
      const pid = parseIntOrNull(line.trim().split(/\s+/, 2)[0]);
      return (
        interestingPids.has(pid) ||
        /headless_shell|esbuild|run_matrix\.mjs|loopback_runner\.mjs/i.test(line)
      );
    });

    return [header, ...filtered].filter(Boolean).slice(-MAX_PROCESS_LINES);
  } catch (error) {
    return [`<ps unavailable: ${error.message}>`];
  }
}

function toDmesgSince(isoTimestamp) {
  const date = isoTimestamp ? new Date(isoTimestamp) : null;
  if (!date || Number.isNaN(date.getTime())) {
    return null;
  }

  const pad = value => String(value).padStart(2, '0');
  return [
    date.getFullYear(),
    '-',
    pad(date.getMonth() + 1),
    '-',
    pad(date.getDate()),
    ' ',
    pad(date.getHours()),
    ':',
    pad(date.getMinutes()),
    ':',
    pad(date.getSeconds()),
  ].join('');
}

function getKernelTail(createdAt) {
  const args = ['-T'];
  const since = toDmesgSince(createdAt);
  if (since) {
    args.push('--since', since);
  }

  const matcher = /headless_shell|oom-kill|Out of memory|Killed process|chromium|esbuild/i;
  try {
    const output = execFileSync('dmesg', args, {
      encoding: 'utf8',
      maxBuffer: 2 * 1024 * 1024,
    });
    const lines = output
      .split(/\r?\n/)
      .map(line => line.trimEnd())
      .filter(line => matcher.test(line));

    return lines.length > 0
      ? lines.slice(-MAX_KERNEL_LINES)
      : [`<no matching kernel log lines since ${createdAt ?? 'unknown'}>`];
  } catch (error) {
    return [`<dmesg unavailable: ${error.message}>`];
  }
}

function pushEvent(diagnostics, type, details = {}) {
  pushCapped(
    diagnostics.events,
    {
      ts: nowIso(),
      type,
      details,
    }
  );
}

function captureLocation(message) {
  try {
    const location = message.location?.();
    if (!location) {
      return null;
    }
    return {
      url: location.url ?? null,
      lineNumber: location.lineNumber ?? null,
      columnNumber: location.columnNumber ?? null,
    };
  } catch {
    return null;
  }
}

function captureFrameDetails(frame, page) {
  const details = {};

  try {
    details.url = frame.url?.() ?? null;
  } catch {
    details.url = '<unavailable>';
  }
  try {
    details.name = frame.name?.() ?? null;
  } catch {
    details.name = '<unavailable>';
  }
  try {
    details.isMainFrame = page?.mainFrame?.() === frame;
  } catch {
    details.isMainFrame = null;
  }

  return details;
}

function finalizeDiagnostics(diagnostics, browser, failureContext = {}) {
  return JSON.parse(JSON.stringify({
    ...diagnostics,
    failureContext: {
      capturedAt: nowIso(),
      ...failureContext,
    },
    currentSnapshot: collectRuntimeSnapshot(browser),
    relevantProcesses: getRelevantProcessLines(getBrowserPid(browser)),
    kernelTail: getKernelTail(diagnostics.meta?.createdAt),
  }));
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
    protocolTimeout: PUPPETEER_PROTOCOL_TIMEOUT_MS,
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

  // Use a prio qdisc as root, then attach netem only to UDP traffic.
  // Band 1 = default (TCP, unmatched) → no impairment.
  // Band 2 = UDP → netem impairment.
  runTc(['qdisc', 'add', 'dev', 'lo', 'root', 'handle', '1:', 'prio', 'bands', '3', 'priomap', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', '1']);

  const netemArgs = ['qdisc', 'add', 'dev', 'lo', 'parent', '1:3', 'handle', '30:', 'netem'];
  // The matrix uses round-trip RTT targets, while netem delay is one-way.
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

  // Filter: send UDP packets to band 3 (netem), everything else stays on band 1 (no impairment).
  runTc(['filter', 'add', 'dev', 'lo', 'parent', '1:0', 'protocol', 'ip', 'u32', 'match', 'ip', 'protocol', '17', '0xff', 'flowid', '1:3']);
}

export async function createLoopbackHarness(options = {}) {
  const diagnostics = {
    meta: {
      caseId: options.caseId ?? null,
      createdAt: nowIso(),
      chromiumPath,
      nodePid: process.pid,
      tmpDir: null,
      bundlePath: null,
      browserPid: null,
      protocolTimeoutMs: PUPPETEER_PROTOCOL_TIMEOUT_MS,
      warmupMs: HARNESS_WARMUP_MS,
    },
    console: [],
    pageErrors: [],
    requestFailures: [],
    pageCrashes: [],
    pageClosed: false,
    browserDisconnected: false,
    browserProcessExits: [],
    events: [],
    runtimeSnapshots: [],
  };
  let tmpDir;
  let bundlePath;
  let browser;
  let page;

  try {
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-browser-'));
    diagnostics.meta.tmpDir = tmpDir;
    pushEvent(diagnostics, 'tmp_dir_created', { tmpDir });

    bundlePath = buildBundle(tmpDir);
    diagnostics.meta.bundlePath = bundlePath;
    pushEvent(diagnostics, 'bundle_built', { bundlePath });

    browser = await launchBrowser();
    diagnostics.meta.browserPid = getBrowserPid(browser);
    pushEvent(diagnostics, 'browser_launched', {
      browserPid: diagnostics.meta.browserPid,
      connected: getBrowserConnectionState(browser),
    });

    const browserProcess = browser.process?.();
    browserProcess?.on('exit', (code, signal) => {
      const event = {
        ts: nowIso(),
        code: code ?? null,
        signal: signal ?? null,
      };
      pushCapped(diagnostics.browserProcessExits, event, 20);
      pushEvent(diagnostics, 'browser_process_exit', event);
    });

    page = await browser.newPage();
    pushEvent(diagnostics, 'page_created', {});
    page.on('console', message => {
      pushCapped(diagnostics.console, {
        ts: nowIso(),
        type: message.type(),
        text: message.text(),
        location: captureLocation(message),
      });
    });
    page.on('pageerror', error => {
      const message = error?.stack || error?.message || String(error);
      pushCapped(diagnostics.pageErrors, {
        ts: nowIso(),
        message,
      });
      pushEvent(diagnostics, 'page_error', { message });
    });
    page.on('requestfailed', request => {
      const entry = {
        ts: nowIso(),
        url: request.url(),
        failure: request.failure()?.errorText || 'unknown',
      };
      pushCapped(diagnostics.requestFailures, entry);
      pushEvent(diagnostics, 'request_failed', entry);
    });
    page.on('error', error => {
      const message = error?.stack || error?.message || String(error);
      pushCapped(diagnostics.pageCrashes, {
        ts: nowIso(),
        message,
      });
      pushEvent(diagnostics, 'page_crash', { message });
    });
    page.on('close', () => {
      diagnostics.pageClosed = true;
      pushEvent(diagnostics, 'page_closed', {});
    });
    page.on('framenavigated', frame => {
      pushEvent(diagnostics, 'frame_navigated', captureFrameDetails(frame, page));
    });
    page.on('framedetached', frame => {
      pushEvent(diagnostics, 'frame_detached', captureFrameDetails(frame, page));
    });
    browser.on('disconnected', () => {
      diagnostics.browserDisconnected = true;
      pushEvent(diagnostics, 'browser_disconnected', {});
    });

    const bundleCode = fs.readFileSync(bundlePath, 'utf8');
    pushEvent(diagnostics, 'page_set_content_start', {});
    await page.setContent(
      '<!doctype html><html><head><meta charset="utf-8"><title>QoS Loopback</title></head><body></body></html>',
      { waitUntil: 'load' }
    );
    pushEvent(diagnostics, 'page_set_content_done', {});
    await page.addScriptTag({ content: bundleCode });
    pushEvent(diagnostics, 'bundle_injected', {});
    await page.evaluate(() => window.__qosLoopbackHarness.init());
    pushEvent(diagnostics, 'harness_init_done', {});
    pushCapped(
      diagnostics.runtimeSnapshots,
      {
        reason: 'after_init',
        ...collectRuntimeSnapshot(browser),
      },
      MAX_RUNTIME_SNAPSHOTS
    );
    // Let the loopback pair and the first sender stats samples settle before the
    // matrix baseline timer starts, otherwise startup transients bleed into mild
    // boundary cases such as R3.
    await sleep(HARNESS_WARMUP_MS);
    pushEvent(diagnostics, 'harness_warmup_done', {});

    return {
      async nowMs() {
        return page.evaluate(() => Date.now());
      },
      async getState() {
        pushEvent(diagnostics, 'get_state_start', {});
        try {
          const state = await page.evaluate(() => window.__qosLoopbackHarness.getState());
          pushEvent(diagnostics, 'get_state_done', {
            state: state?.state ?? null,
            level: state?.level ?? null,
            lastError: state?.lastError ?? null,
          });
          return state;
        } catch (error) {
          pushEvent(diagnostics, 'get_state_error', {
            message: error?.message ?? String(error),
          });
          throw error;
        }
      },
      async captureRuntimeSnapshot(reason) {
        const snapshot = {
          reason,
          ...collectRuntimeSnapshot(browser),
        };
        pushCapped(diagnostics.runtimeSnapshots, snapshot, MAX_RUNTIME_SNAPSHOTS);
        return JSON.parse(JSON.stringify(snapshot));
      },
      async stop() {
        pushEvent(diagnostics, 'stop_start', {});
        try {
          await page?.evaluate(() => window.__qosLoopbackHarness.stop());
          pushEvent(diagnostics, 'page_stop_done', {});
        } catch (error) {
          pushEvent(diagnostics, 'page_stop_error', {
            message: error?.message ?? String(error),
          });
        }
        try {
          await browser?.close();
          pushEvent(diagnostics, 'browser_close_done', {});
        } catch (error) {
          pushEvent(diagnostics, 'browser_close_error', {
            message: error?.message ?? String(error),
          });
        }
        clearNetem();
        if (tmpDir) {
          fs.rmSync(tmpDir, { recursive: true, force: true });
          pushEvent(diagnostics, 'tmp_dir_removed', { tmpDir });
        }
      },
      async getDiagnostics(failureContext = {}) {
        return finalizeDiagnostics(diagnostics, browser, failureContext);
      },
    };
  } catch (error) {
    pushEvent(diagnostics, 'create_harness_error', {
      message: error?.message ?? String(error),
      stack: error?.stack ?? null,
    });
    error.qosDiagnostics = finalizeDiagnostics(diagnostics, browser, {
      reason: 'create_loopback_harness',
      errorMessage: error?.message ?? String(error),
      errorStack: error?.stack ?? null,
    });
    try {
      await browser?.close();
    } catch {}
    clearNetem();
    if (tmpDir) {
      fs.rmSync(tmpDir, { recursive: true, force: true });
    }
    throw error;
  }
}
