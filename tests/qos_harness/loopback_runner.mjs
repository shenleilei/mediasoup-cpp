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
const MAX_SYSTEM_LINES = 80;
const MAX_PROCESS_LINES = 40;
const MAX_BROWSER_IO_LINES = 120;
const RUNTIME_SNAPSHOT_INTERVAL_MS = 5000;

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

function kbToMb(kb) {
  return typeof kb === 'number'
    ? roundToOne(kb / 1024)
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

function readByteCountFile(filePath) {
  const value = safeReadText(filePath)?.trim();
  if (!value) {
    return null;
  }

  const parsed = Number(value);
  if (!Number.isFinite(parsed) || parsed > Number.MAX_SAFE_INTEGER) {
    return null;
  }

  return parsed;
}

function parseMemInfo() {
  const text = safeReadText('/proc/meminfo');
  if (!text) {
    return null;
  }

  const values = {};
  for (const line of text.split(/\r?\n/)) {
    const match = line.match(/^([^:]+):\s+(\d+)/);
    if (!match) {
      continue;
    }
    values[match[1]] = Number(match[2]);
  }

  return {
    memAvailableMb: kbToMb(values.MemAvailable),
    swapFreeMb: kbToMb(values.SwapFree),
    swapTotalMb: kbToMb(values.SwapTotal),
  };
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

function readProcCmdline(pid) {
  const raw = safeReadText(`/proc/${pid}/cmdline`);
  return raw
    ? raw.replace(/\0/g, ' ').trim() || null
    : null;
}

function readProcCgroup(pid) {
  const text = safeReadText(`/proc/${pid}/cgroup`);
  if (!text) {
    return null;
  }

  const entries = [];
  for (const line of text.split(/\r?\n/)) {
    if (!line) {
      continue;
    }
    const [hierarchyId, controllers, cgroupPath] = line.split(':');
    entries.push({
      hierarchyId: parseIntOrNull(hierarchyId),
      controllers: controllers ? controllers.split(',').filter(Boolean) : [],
      path: cgroupPath ?? null,
    });
  }

  return entries;
}

function readMemoryCgroupSnapshot(cgroupEntries) {
  const memoryEntry = cgroupEntries?.find(entry => entry.controllers.includes('memory'));
  if (!memoryEntry?.path) {
    return null;
  }

  const root = '/sys/fs/cgroup/memory';
  const suffix = memoryEntry.path === '/' ? '' : memoryEntry.path;
  const basePath = `${root}${suffix}`;

  return {
    path: memoryEntry.path,
    usageMb: toMb(readByteCountFile(`${basePath}/memory.usage_in_bytes`)),
    limitMb: toMb(readByteCountFile(`${basePath}/memory.limit_in_bytes`)),
    memswUsageMb: toMb(readByteCountFile(`${basePath}/memory.memsw.usage_in_bytes`)),
    memswLimitMb: toMb(readByteCountFile(`${basePath}/memory.memsw.limit_in_bytes`)),
    failcnt: readNumericFile(`${basePath}/memory.failcnt`),
  };
}

function parseProcessRow(line) {
  const match = line
    .trim()
    .match(/^(\d+)\s+(\d+)\s+(\d+)\s+(\S+)\s+(\d+)\s+(\d+)\s+(\S+)\s*(.*)$/);
  if (!match) {
    return null;
  }

  return {
    pid: Number(match[1]),
    ppid: Number(match[2]),
    sess: Number(match[3]),
    stat: match[4],
    rssKb: Number(match[5]),
    vszKb: Number(match[6]),
    comm: match[7],
    args: match[8] || match[7],
  };
}

function getProcessTableRows() {
  try {
    const output = execFileSync(
      'ps',
      ['-eo', 'pid=,ppid=,sess=,stat=,rss=,vsz=,comm=,args='],
      { encoding: 'utf8', maxBuffer: 1024 * 1024 }
    );
    return output
      .split(/\r?\n/)
      .map(parseProcessRow)
      .filter(Boolean);
  } catch {
    return [];
  }
}

function getPidTreeRows(rootPid, rows) {
  if (!Number.isInteger(rootPid) || rootPid <= 0) {
    return [];
  }

  const childrenByParent = new Map();
  for (const row of rows) {
    if (!childrenByParent.has(row.ppid)) {
      childrenByParent.set(row.ppid, []);
    }
    childrenByParent.get(row.ppid).push(row);
  }

  const rowByPid = new Map(rows.map(row => [row.pid, row]));
  const ordered = [];
  const queue = [rootPid];
  const seen = new Set();

  while (queue.length > 0) {
    const pid = queue.shift();
    if (seen.has(pid)) {
      continue;
    }
    seen.add(pid);

    const current = rowByPid.get(pid);
    if (current) {
      ordered.push(current);
    }

    for (const child of childrenByParent.get(pid) ?? []) {
      queue.push(child.pid);
    }
  }

  return ordered;
}

function formatProcessRows(rows) {
  if (!rows || rows.length === 0) {
    return [];
  }

  return [
    'PID    PPID SESS STAT   RSS    VSZ COMMAND         COMMAND',
    ...rows.slice(0, MAX_PROCESS_LINES - 1).map(
      row =>
        `${String(row.pid).padStart(6)} ${String(row.ppid).padStart(6)} ${String(row.sess).padStart(4)} ` +
        `${String(row.stat).padEnd(4)} ${String(row.rssKb).padStart(6)} ${String(row.vszKb).padStart(7)} ` +
        `${String(row.comm).padEnd(15)} ${row.args}`
    ),
  ];
}

function collectPidContext(pid, rows = null) {
  if (!Number.isInteger(pid) || pid <= 0) {
    return null;
  }

  const processRows = rows ?? getProcessTableRows();
  const row = processRows.find(entry => entry.pid === pid) ?? null;
  const cgroup = readProcCgroup(pid);

  return {
    pid,
    alive: fs.existsSync(`/proc/${pid}`),
    ppid: row?.ppid ?? null,
    sessionId: row?.sess ?? null,
    cmdline: readProcCmdline(pid),
    procStatus: parseProcStatus(safeReadText(`/proc/${pid}/status`)),
    cgroup,
    memoryCgroup: readMemoryCgroupSnapshot(cgroup),
  };
}

function collectProcessDiagnostics(browserPid = null) {
  const rows = getProcessTableRows();
  const runnerTreeRows = getPidTreeRows(process.pid, rows);
  const browserTreeRows = getPidTreeRows(browserPid, rows);
  const excludedPids = new Set([
    ...runnerTreeRows.map(row => row.pid),
    ...browserTreeRows.map(row => row.pid),
  ]);
  const otherBrowserRows = rows.filter(
    row =>
      row.comm === 'headless_shell' &&
      !excludedPids.has(row.pid) &&
      !/--type=/.test(row.args)
  );

  return {
    browserContext: collectPidContext(browserPid, rows),
    runnerProcessTree: formatProcessRows(runnerTreeRows),
    browserProcessTree: formatProcessRows(browserTreeRows),
    otherBrowserProcesses: formatProcessRows(otherBrowserRows),
  };
}

function createLineCollector(list, limit = MAX_BROWSER_IO_LINES) {
  let remainder = '';

  const pushLine = line => {
    pushCapped(
      list,
      {
        ts: nowIso(),
        line,
      },
      limit
    );
  };

  return {
    push(chunk) {
      remainder += chunk.toString('utf8');
      const parts = remainder.split(/\r?\n/);
      remainder = parts.pop() ?? '';
      for (const line of parts) {
        pushLine(line);
      }
    },
    flush() {
      if (remainder) {
        pushLine(remainder);
        remainder = '';
      }
    },
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
  const memInfo = parseMemInfo();
  const procStatus = browserPid
    ? parseProcStatus(safeReadText(`/proc/${browserPid}/status`))
    : null;
  const browserContext = collectPidContext(browserPid);

  return {
    capturedAt: nowIso(),
    host: {
      totalMemMb: toMb(os.totalmem()),
      freeMemMb: toMb(os.freemem()),
      memAvailableMb: memInfo?.memAvailableMb ?? null,
      swapFreeMb: memInfo?.swapFreeMb ?? null,
      swapTotalMb: memInfo?.swapTotalMb ?? null,
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
      sessionId: browserContext?.sessionId ?? null,
      memoryCgroupPath: browserContext?.memoryCgroup?.path ?? null,
      memoryCgroupUsageMb: browserContext?.memoryCgroup?.usageMb ?? null,
      memoryCgroupFailcnt: browserContext?.memoryCgroup?.failcnt ?? null,
      procStatus,
    },
  };
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

function getJournalLines(args, matcher, emptyMessage) {
  try {
    const output = execFileSync('journalctl', args, {
      encoding: 'utf8',
      maxBuffer: 4 * 1024 * 1024,
    });
    const lines = output
      .split(/\r?\n/)
      .map(line => line.trimEnd())
      .filter(line => matcher.test(line));

    return lines.length > 0
      ? lines.slice(-MAX_SYSTEM_LINES).map(line => `[journalctl] ${line}`)
      : [`<${emptyMessage}>`];
  } catch {
    return null;
  }
}

function getKernelTail(createdAt) {
  const since = toDmesgSince(createdAt);
  const matcher = /headless_shell|oom-kill|Out of memory|Killed process|chromium|esbuild/i;
  const journalArgs = ['-k', '-o', 'short-iso'];
  if (since) {
    journalArgs.push('--since', since);
  }

  const journalLines = getJournalLines(
    journalArgs,
    matcher,
    `no matching kernel log lines since ${createdAt ?? 'unknown'}`
  );
  if (journalLines) {
    return journalLines.slice(-MAX_KERNEL_LINES);
  }

  const dmesgArgs = ['-T'];
  if (since) {
    dmesgArgs.push('--since', since);
  }

  try {
    const output = execFileSync('dmesg', dmesgArgs, {
      encoding: 'utf8',
      maxBuffer: 2 * 1024 * 1024,
    });
    const lines = output
      .split(/\r?\n/)
      .map(line => line.trimEnd())
      .filter(line => matcher.test(line));

    return lines.length > 0
      ? lines.slice(-MAX_KERNEL_LINES).map(line => `[dmesg] ${line}`)
      : [`<no matching kernel log lines since ${createdAt ?? 'unknown'}>`];
  } catch (error) {
    try {
      const output = execFileSync('dmesg', ['-T'], {
        encoding: 'utf8',
        maxBuffer: 2 * 1024 * 1024,
      });
      const lines = output
        .split(/\r?\n/)
        .map(line => line.trimEnd())
        .filter(line => matcher.test(line));

      return lines.length > 0
        ? lines.slice(-MAX_KERNEL_LINES).map(line => `[dmesg-fallback] ${line}`)
        : ['<no matching kernel log lines in full dmesg>'];
    } catch (fallbackError) {
      return [`<dmesg unavailable: ${error.message}; fallback failed: ${fallbackError.message}>`];
    }
  }
}

function getSystemTail(createdAt) {
  const since = toDmesgSince(createdAt);
  const matcher =
    /headless_shell|chromium|esbuild|run_matrix|oom|Killed process|systemd-coredump|session-[0-9]+\.scope/i;
  const journalArgs = ['-o', 'short-iso'];
  if (since) {
    journalArgs.push('--since', since);
  }

  const journalLines = getJournalLines(
    journalArgs,
    matcher,
    `no matching system log lines since ${createdAt ?? 'unknown'}`
  );
  return journalLines ?? ['<system journal unavailable>'];
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
  const browserPid = diagnostics.meta?.browserPid ?? getBrowserPid(browser);
  const processDiagnostics = collectProcessDiagnostics(browserPid);

  return JSON.parse(JSON.stringify({
    ...diagnostics,
    failureContext: {
      capturedAt: nowIso(),
      ...failureContext,
    },
    currentSnapshot: collectRuntimeSnapshot(browser),
    browserContext:
      processDiagnostics.browserContext ??
      diagnostics.browserContextAtExit ??
      diagnostics.browserContextAtLaunch ??
      null,
    runnerProcessTree:
      processDiagnostics.runnerProcessTree.length > 0
        ? processDiagnostics.runnerProcessTree
        : diagnostics.runnerProcessTreeAtExit ?? [],
    browserProcessTree:
      processDiagnostics.browserProcessTree.length > 0
        ? processDiagnostics.browserProcessTree
        : diagnostics.browserProcessTreeAtExit ?? [],
    otherBrowserProcesses: processDiagnostics.otherBrowserProcesses,
    relevantProcesses:
      processDiagnostics.runnerProcessTree.length > 0
        ? processDiagnostics.runnerProcessTree
        : diagnostics.runnerProcessTreeAtExit ?? [],
    kernelTail: getKernelTail(diagnostics.meta?.createdAt),
    systemTail: getSystemTail(diagnostics.meta?.createdAt),
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
    browserStdout: [],
    browserStderr: [],
    browserContextAtLaunch: null,
    browserContextAtExit: null,
    browserProcessTreeAtExit: [],
    runnerProcessTreeAtExit: [],
    events: [],
    runtimeSnapshots: [],
  };
  let tmpDir;
  let bundlePath;
  let browser;
  let page;
  let browserStdoutCollector;
  let browserStderrCollector;
  let runtimeSnapshotTimer;

  try {
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-browser-'));
    diagnostics.meta.tmpDir = tmpDir;
    pushEvent(diagnostics, 'tmp_dir_created', { tmpDir });

    bundlePath = buildBundle(tmpDir);
    diagnostics.meta.bundlePath = bundlePath;
    pushEvent(diagnostics, 'bundle_built', { bundlePath });

    browser = await launchBrowser();
    diagnostics.meta.browserPid = getBrowserPid(browser);
    diagnostics.browserContextAtLaunch = collectPidContext(diagnostics.meta.browserPid);
    pushEvent(diagnostics, 'browser_launched', {
      browserPid: diagnostics.meta.browserPid,
      connected: getBrowserConnectionState(browser),
    });

    const browserProcess = browser.process?.();
    browserStdoutCollector = createLineCollector(diagnostics.browserStdout);
    browserStderrCollector = createLineCollector(diagnostics.browserStderr);
    browserProcess?.stdout?.on('data', chunk => browserStdoutCollector.push(chunk));
    browserProcess?.stderr?.on('data', chunk => browserStderrCollector.push(chunk));
    browserProcess?.on('exit', (code, signal) => {
      browserStdoutCollector.flush();
      browserStderrCollector.flush();
      diagnostics.browserContextAtExit = collectPidContext(diagnostics.meta.browserPid);
      const processDiagnostics = collectProcessDiagnostics(diagnostics.meta.browserPid);
      diagnostics.browserProcessTreeAtExit = processDiagnostics.browserProcessTree;
      diagnostics.runnerProcessTreeAtExit = processDiagnostics.runnerProcessTree;
      const event = {
        ts: nowIso(),
        code: code ?? null,
        signal: signal ?? null,
      };
      pushCapped(diagnostics.browserProcessExits, event, 20);
      pushEvent(diagnostics, 'browser_process_exit', event);
      pushCapped(
        diagnostics.runtimeSnapshots,
        {
          reason: 'browser_process_exit',
          ...collectRuntimeSnapshot(browser),
        },
        MAX_RUNTIME_SNAPSHOTS
      );
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
    runtimeSnapshotTimer = setInterval(() => {
      pushCapped(
        diagnostics.runtimeSnapshots,
        {
          reason: 'interval',
          ...collectRuntimeSnapshot(browser),
        },
        MAX_RUNTIME_SNAPSHOTS
      );
    }, RUNTIME_SNAPSHOT_INTERVAL_MS);
    runtimeSnapshotTimer.unref?.();
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
        browserStdoutCollector?.flush();
        browserStderrCollector?.flush();
        if (runtimeSnapshotTimer) {
          clearInterval(runtimeSnapshotTimer);
          runtimeSnapshotTimer = null;
        }
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
    browserStdoutCollector?.flush();
    browserStderrCollector?.flush();
    if (runtimeSnapshotTimer) {
      clearInterval(runtimeSnapshotTimer);
      runtimeSnapshotTimer = null;
    }
    clearNetem();
    if (tmpDir) {
      fs.rmSync(tmpDir, { recursive: true, force: true });
    }
    throw error;
  }
}
