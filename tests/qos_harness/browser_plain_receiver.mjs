import fs from 'node:fs';
import http from 'node:http';
import os from 'node:os';
import path from 'node:path';
import net from 'node:net';
import { fileURLToPath } from 'node:url';
import { spawn } from 'node:child_process';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');

let buildDir = path.join(repoRoot, 'build-webrtc-qos-plain');
let workerBin = path.join(repoRoot, 'mediasoup-worker');
let reportDir = path.join(repoRoot, 'docs', 'generated');
let artifactRoot = path.join(os.tmpdir(), 'webrtc-qos-plain-p2-browser-receiver');
let reportName = 'webrtc-qos-plain-p2-browser-receiver-report';
let chromiumPath = '/usr/lib64/chromium-browser/headless_shell';
let durationSeconds = 12;
let sourceMode = 'synthetic';
let inputFile = '';
let serverIp = '127.0.0.1';
let mediaIp = '127.0.0.1';
let basePort = 35131;

function usage() {
  return `Usage:
  node tests/qos_harness/browser_plain_receiver.mjs [options]

Options:
  --build-dir <path>          Build dir containing mediasoup-sfu and plain push client.
  --worker-bin <path>         mediasoup-worker binary. Default: ./mediasoup-worker.
  --report-dir <path>         Report output directory. Default: docs/generated.
  --report-name <name>        Report basename without extension.
  --artifact-root <path>      Runtime artifacts root.
  --chromium-path <path>      Chromium/headless_shell path.
  --duration-seconds <n>      Browser media observation window. Default: 12.
  --source <synthetic|mp4-decode-loop|copy>
  --input <path>              MP4 input for copy or mp4-decode-loop.
  --base-port <port>          SFU signaling port. Default: 35131.
  -h, --help                  Show this help.
`;
}

function requireValue(argv, index, option) {
  const value = argv[index + 1];
  if (!value) throw new Error(`missing value for ${option}`);
  return value;
}

function parseArgs(argv) {
  for (let i = 2; i < argv.length; ++i) {
    const arg = argv[i];
    const eq = arg.indexOf('=');
    const key = eq === -1 ? arg : arg.slice(0, eq);
    const inline = eq === -1 ? null : arg.slice(eq + 1);
    const value = () => inline ?? requireValue(argv, i++, arg);
    switch (key) {
      case '--build-dir':
        buildDir = path.resolve(value());
        break;
      case '--worker-bin':
        workerBin = path.resolve(value());
        break;
      case '--report-dir':
        reportDir = path.resolve(value());
        break;
      case '--report-name':
        reportName = value();
        break;
      case '--artifact-root':
        artifactRoot = path.resolve(value());
        break;
      case '--chromium-path':
        chromiumPath = value();
        break;
      case '--duration-seconds':
        durationSeconds = Number(value());
        break;
      case '--source':
        sourceMode = value();
        break;
      case '--input':
        inputFile = path.resolve(value());
        break;
      case '--base-port':
        basePort = Number(value());
        break;
      case '-h':
      case '--help':
        console.log(usage());
        process.exit(0);
        break;
      default:
        throw new Error(`unknown option: ${arg}`);
    }
  }
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function ensureExecutable(filePath, label) {
  if (!fs.existsSync(filePath)) throw new Error(`${label} not found: ${filePath}`);
  fs.accessSync(filePath, fs.constants.X_OK);
}

async function allocatePort(preferred) {
  if (preferred) {
    const available = await new Promise(resolve => {
      const server = net.createServer();
      server.once('error', () => resolve(false));
      server.listen(preferred, '127.0.0.1', () => server.close(() => resolve(true)));
    });
    if (available) return preferred;
  }
  return await new Promise((resolve, reject) => {
    const server = net.createServer();
    server.once('error', reject);
    server.listen(0, '127.0.0.1', () => {
      const { port } = server.address();
      server.close(error => error ? reject(error) : resolve(port));
    });
  });
}

async function waitForHttpReady(port, timeoutMs = 10000) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      const response = await fetch(`http://${serverIp}:${port}/readyz`);
      if (response.ok) {
        const data = await response.json();
        if (data.ok === true) return;
      }
    } catch {
      // keep polling
    }
    await sleep(200);
  }
  throw new Error(`SFU readyz timeout on port ${port}`);
}

function startProcess(command, args, cwd, stdoutPath, stderrPath, extraEnv = {}) {
  const stdout = fs.openSync(stdoutPath, 'w');
  const stderr = fs.openSync(stderrPath, 'w');
  const child = spawn(command, args, {
    cwd,
    detached: true,
    env: { ...process.env, ...extraEnv },
    stdio: ['ignore', stdout, stderr],
  });
  child.once('exit', () => {
    fs.closeSync(stdout);
    fs.closeSync(stderr);
  });
  return child;
}

function processAlive(child) {
  if (!child?.pid || child.exitCode !== null) return false;
  try {
    process.kill(child.pid, 0);
    return true;
  } catch {
    return false;
  }
}

async function stopProcess(child, timeoutMs = 3000) {
  if (!processAlive(child)) return;
  try {
    process.kill(-child.pid, 'SIGTERM');
  } catch {
    try { child.kill('SIGTERM'); } catch {}
  }
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline && processAlive(child)) await sleep(100);
  if (processAlive(child)) {
    try {
      process.kill(-child.pid, 'SIGKILL');
    } catch {
      try { child.kill('SIGKILL'); } catch {}
    }
  }
}

function pushArgs({ pushBin, port, roomId, peerId, caseDir }) {
  const args = [
    `--server-ip=${serverIp}`,
    `--server-port=${port}`,
    `--room=${roomId}`,
    `--peer=${peerId}`,
    '--video-ssrc=22334455',
    `--media-remote-ip=${mediaIp}`,
    `--log-dir=${path.join(caseDir, 'push')}`,
  ];
  if (sourceMode === 'synthetic') {
    args.push('--input-synthetic=true', '--encoder=x264', '--synthetic-width=320', '--synthetic-height=180', '--synthetic-fps=15');
  } else if (sourceMode === 'mp4-decode-loop') {
    if (!inputFile) throw new Error('--input is required for mp4-decode-loop source');
    args.push(`--input=${inputFile}`, '--input-decode-loop=true', '--loop-input=true', '--encoder=x264');
  } else if (sourceMode === 'copy') {
    if (!inputFile) throw new Error('--input is required for copy source');
    args.push(`--input=${inputFile}`, '--loop-input=true');
  } else {
    throw new Error(`unsupported source: ${sourceMode}`);
  }
  return [pushBin, args];
}

function parsePushLog(caseDir) {
  const logPath = path.join(caseDir, 'push', 'push.log');
  const text = fs.existsSync(logPath) ? fs.readFileSync(logPath, 'utf8') : '';
  const publish = text.match(/plain_publish_ok .*producerId=([^ ]+) .*ssrc=(\d+) pt=(\d+) twccExtId=(\d+)/);
  const stop = text.match(/push_runtime_stopped pushedAu=(\d+)/);
  const metrics = Array.from(text.matchAll(/push_metrics pushedAu=(\d+) targetBps=(\d+) pacingBps=(\d+) finalTargetBps=(\d+) rttMs=([0-9.]+) loss=([0-9.]+)/g));
  return {
    logPath,
    producerId: publish?.[1] ?? null,
    ssrc: publish ? Number(publish[2]) : null,
    payloadType: publish ? Number(publish[3]) : null,
    twccExtId: publish ? Number(publish[4]) : null,
    pushedAu: stop ? Number(stop[1]) : (metrics.length ? Number(metrics[metrics.length - 1][1]) : null),
    metricSamples: metrics.length,
  };
}

async function waitForPushPublish(caseDir, timeoutMs = 5000) {
  const deadline = Date.now() + timeoutMs;
  let metrics = parsePushLog(caseDir);
  while (!metrics.producerId && Date.now() < deadline) {
    await sleep(100);
    metrics = parsePushLog(caseDir);
  }
  return metrics;
}

function makeCheck(name, passed, evidence) {
  return { name, status: passed ? 'PASS' : 'FAIL', evidence };
}

function makeHarnessHtml({ sfuPort, roomId, peerId, durationSeconds, resultUrl }) {
  const mediasoupBundle = fs.readFileSync(path.join(repoRoot, 'public', 'mediasoup-client.bundle.js'), 'utf8');
  const harness = fs.readFileSync(path.join(__dirname, 'browser', 'plain-receiver-entry.js'), 'utf8');
  return `<!doctype html>
<html>
<head><meta charset="utf-8"><title>P2 Browser Receiver</title></head>
<body>
<script>${mediasoupBundle.replace(/<\/script/gi, '<\\/script')}</script>
<script>${harness.replace(/<\/script/gi, '<\\/script')}</script>
<script>
(async () => {
  try {
    const init = await window.__plainReceiverHarness.init(
      'ws://${serverIp}:${sfuPort}/ws',
      '${roomId}',
      '${peerId}'
    );
    const metrics = await window.__plainReceiverHarness.waitForMedia(${durationSeconds * 1000}, 500);
    metrics.consumerCount = init.consumerCount;
    metrics.precreatedConsumers = init.precreatedConsumers;
    metrics.notificationConsumers = init.notificationConsumers;
    metrics.consumers = init.consumers;
    metrics.diagnostics = window.__plainReceiverHarness.diagnostics || {};
    await fetch('${resultUrl}', {
      method: 'POST',
      headers: { 'content-type': 'application/json' },
      body: JSON.stringify({ ok: true, init, metrics })
    });
  } catch (error) {
    await fetch('${resultUrl}', {
      method: 'POST',
      headers: { 'content-type': 'application/json' },
      body: JSON.stringify({
        ok: false,
        error: error && (error.stack || error.message || String(error)),
        errorCode: error && error.code || null,
        diagnostics: window.__plainReceiverHarness && window.__plainReceiverHarness.diagnostics || {}
      })
    });
  }
})();
</script>
</body>
</html>`;
}

function startResultServer(getHtml, resultHolder) {
  const server = http.createServer((req, res) => {
    if (req.method === 'POST' && req.url === '/result') {
      let body = '';
      req.on('data', chunk => { body += chunk; });
      req.on('end', () => {
        try {
          resultHolder.result = JSON.parse(body);
        } catch (error) {
          resultHolder.result = { ok: false, error: `invalid result json: ${error.message}` };
        }
        res.writeHead(200, { 'content-type': 'text/plain' });
        res.end('ok');
        resultHolder.resolve?.(resultHolder.result);
      });
      return;
    }
    if (req.method === 'GET' && req.url === '/') {
      res.writeHead(200, { 'content-type': 'text/html' });
      res.end(getHtml());
      return;
    }
    res.writeHead(404);
    res.end('not found');
  });
  return new Promise(resolve => {
    server.listen(0, '127.0.0.1', () => resolve(server));
  });
}

function waitForBrowserResult(holder, timeoutMs) {
  if (holder.result) return Promise.resolve(holder.result);
  return new Promise(resolve => {
    const timer = setTimeout(() => resolve({ ok: false, error: 'browser result timeout' }), timeoutMs);
    holder.resolve = result => {
      clearTimeout(timer);
      resolve(result);
    };
  });
}

function isBrowserH264Unsupported(result) {
  return result?.errorCode === 'BROWSER_H264_UNSUPPORTED' ||
    String(result?.error || '').includes('browser does not expose H264 packetization-mode=1');
}

function writeReport({ report, reportJson, reportMd }) {
  fs.mkdirSync(path.dirname(reportJson), { recursive: true });
  fs.writeFileSync(reportJson, `${JSON.stringify(report, null, 2)}\n`);

  const cell = value => String(value ?? '')
    .replace(/\|/g, '\\|')
    .replace(/\r?\n/g, '<br>');

  const c = report.case;
  const lines = [];
  lines.push('# WebRTC QoS Plain P2 Browser Receiver Report');
  lines.push('');
  lines.push('| Item | Value |');
  lines.push('|---|---|');
  lines.push(`| Overall | \`${report.overallStatus}\` |`);
  lines.push(`| Generated At | \`${report.generatedAt}\` |`);
  lines.push(`| Run Dir | \`${report.runDir}\` |`);
  lines.push(`| Source Mode | \`${report.runConfig.sourceMode}\` |`);
  lines.push(`| Duration Seconds | \`${report.runConfig.durationSeconds}\` |`);
  lines.push('');
  lines.push('## Checks');
  lines.push('');
  lines.push('| Check | Status | Evidence |');
  lines.push('|---|---:|---|');
  for (const check of c.checks) {
    lines.push(`| \`${cell(check.name)}\` | \`${cell(check.status)}\` | \`${cell(check.evidence)}\` |`);
  }
  lines.push('');
  lines.push('## Browser Metrics');
  lines.push('');
  lines.push('| Metric | Value |');
  lines.push('|---|---:|');
  lines.push(`| consumerCount | ${c.metrics.browser.consumerCount ?? 0} |`);
  lines.push(`| packetsReceivedDelta | ${c.metrics.browser.delta?.packetsReceived ?? 0} |`);
  lines.push(`| bytesReceivedDelta | ${c.metrics.browser.delta?.bytesReceived ?? 0} |`);
  lines.push(`| framesDecodedDelta | ${c.metrics.browser.delta?.framesDecoded ?? 0} |`);
  lines.push(`| framesReceivedDelta | ${c.metrics.browser.delta?.framesReceived ?? 0} |`);
  lines.push(`| currentTimeDeltaMs | ${c.metrics.browser.delta?.currentTimeMs ?? 0} |`);
  lines.push(`| finalFrameSize | ${c.metrics.browser.last?.frameWidth ?? 0}x${c.metrics.browser.last?.frameHeight ?? 0} |`);
  lines.push(`| keyframeRequests | ${c.metrics.browser.keyframeRequests ?? 0} |`);
  lines.push('');
  lines.push('## Browser Diagnostics');
  lines.push('');
  lines.push('| Item | Value |');
  lines.push('|---|---|');
  lines.push(`| handlerName | \`${c.metrics.browser.diagnostics?.device?.handlerName ?? ''}\` |`);
  lines.push(`| supportsH264Packetization1 | \`${c.metrics.browser.diagnostics?.device?.supportsH264Packetization1 ?? ''}\` |`);
  lines.push(`| deviceVideoCodecs | \`${JSON.stringify(c.metrics.browser.diagnostics?.device?.videoCodecs || [])}\` |`);
  lines.push(`| routerVideoCodecs | \`${JSON.stringify(c.metrics.browser.diagnostics?.join?.routerVideoCodecs || [])}\` |`);
  lines.push('');
  lines.push('## Artifacts');
  lines.push('');
  lines.push(`- JSON report: \`${reportJson}\``);
  lines.push(`- Runtime logs: \`${report.runDir}\``);
  lines.push('');
  lines.push('## Interpretation');
  lines.push('');
  lines.push('- `browser-receiver-media-flow=PASS` means Chromium consumed the plain push video through a real WebRTC recv transport and inbound RTP stats increased.');
  lines.push('- `browser-h264-capability=SKIP` means the local browser binary does not expose H264 packetization-mode=1; rerun the same command with a Chromium build that includes H264 to turn this into a blocking PASS/FAIL gate.');
  lines.push('- This browser report does not replace native weak-network QoS smoke; it covers P2-M7 browser receiver compatibility.');
  fs.writeFileSync(reportMd, `${lines.join('\n')}\n`);
}

async function run() {
  parseArgs(process.argv);
  if (!Number.isInteger(durationSeconds) || durationSeconds < 3) {
    throw new Error('--duration-seconds must be an integer >= 3');
  }

  const sfuBin = path.join(buildDir, 'mediasoup-sfu');
  const pushBin = path.join(buildDir, 'webrtc-qos-plain-push-client');
  ensureExecutable(sfuBin, 'mediasoup-sfu');
  ensureExecutable(pushBin, 'webrtc-qos-plain-push-client');
  ensureExecutable(workerBin, 'mediasoup-worker');
  ensureExecutable(chromiumPath, 'chromium');

  const runId = new Date().toISOString().replace(/[-:]/g, '').replace(/\.\d{3}Z$/, 'Z');
  const runDir = path.join(artifactRoot, runId);
  const caseDir = path.join(runDir, 'browser_receiver');
  fs.mkdirSync(path.join(caseDir, 'push'), { recursive: true });
  fs.mkdirSync(reportDir, { recursive: true });

  const sfuPort = await allocatePort(basePort);
  const roomId = `p2-browser-${runId}`;
  const pushPeer = 'p2-plain-push';
  const browserPeer = 'p2-browser-recv';
  const reportJson = path.join(reportDir, `${reportName}.json`);
  const reportMd = path.join(reportDir, `${reportName}.md`);

  let sfu = null;
  let push = null;
  let chromium = null;
  let resultServer = null;
  const resultHolder = {};

  try {
    sfu = startProcess(
      sfuBin,
      [
        '--nodaemon',
        `--port=${sfuPort}`,
        '--workers=1',
        '--workerThreads=1',
        `--listenIp=${serverIp}`,
        `--announcedIp=${serverIp}`,
        `--workerBin=${workerBin}`,
        '--noRedisRequired',
      ],
      repoRoot,
      path.join(caseDir, 'sfu.stdout.log'),
      path.join(caseDir, 'sfu.stderr.log'),
    );
    await waitForHttpReady(sfuPort);

    const [pushCommand, pushCommandArgs] = pushArgs({ pushBin, port: sfuPort, roomId, peerId: pushPeer, caseDir });
    push = startProcess(
      pushCommand,
      pushCommandArgs,
      repoRoot,
      path.join(caseDir, 'push.stdout.log'),
      path.join(caseDir, 'push.stderr.log'),
    );

    await sleep(1800);
    if (!processAlive(push)) throw new Error('plain push process exited before browser receiver started');
    const publishWarmupMetrics = await waitForPushPublish(caseDir);
    if (!publishWarmupMetrics.producerId) {
      throw new Error(`plain push did not publish before browser receiver started: ${JSON.stringify(publishWarmupMetrics)}`);
    }

    let servedHtml = '<!doctype html><html><body>starting</body></html>';
    resultServer = await startResultServer(() => servedHtml, resultHolder);
    const resultPort = resultServer.address().port;
    const resultUrl = `http://127.0.0.1:${resultPort}/result`;
    servedHtml = makeHarnessHtml({ sfuPort, roomId, peerId: browserPeer, durationSeconds, resultUrl });
    const pageUrl = `http://127.0.0.1:${resultPort}/`;

    chromium = startProcess(
      chromiumPath,
      [
        '--headless',
        '--no-sandbox',
        '--autoplay-policy=no-user-gesture-required',
        '--use-fake-ui-for-media-stream',
        '--disable-gpu',
        '--disable-dev-shm-usage',
        pageUrl,
      ],
      repoRoot,
      path.join(caseDir, 'chromium.stdout.log'),
      path.join(caseDir, 'chromium.stderr.log'),
    );

    const browserResult = await waitForBrowserResult(resultHolder, Math.max(10000, durationSeconds * 1000 + 8000));
    const browserH264Unsupported = isBrowserH264Unsupported(browserResult);
    const browserMetrics = browserResult.metrics || {
      consumerCount: 0,
      delta: {},
      last: { videos: [] },
      keyframeRequests: 0,
      diagnostics: browserResult.diagnostics || {},
    };
    if (!browserMetrics.diagnostics && browserResult.diagnostics) {
      browserMetrics.diagnostics = browserResult.diagnostics;
    }
    const pushMetrics = parsePushLog(caseDir);
    const checks = [
      makeCheck('plain-push-alive', processAlive(push), `pid=${push.pid}`),
      makeCheck('plain-publish-ok', Boolean(pushMetrics.producerId), JSON.stringify(pushMetrics)),
      {
        name: 'browser-h264-capability',
        status: browserH264Unsupported ? 'SKIP' : 'PASS',
        evidence: browserH264Unsupported
          ? String(browserResult.error || 'browser H264 unsupported')
          : `supportsH264Packetization1=${browserMetrics.diagnostics?.device?.supportsH264Packetization1 ?? true}`,
      },
      makeCheck(
        'browser-harness-ok',
        browserH264Unsupported || browserResult.ok === true,
        browserH264Unsupported ? 'skipped after codec diagnostics' : (browserResult.ok ? 'ok' : String(browserResult.error || 'unknown'))
      ),
      {
        name: 'browser-consumer-created',
        status: browserH264Unsupported ? 'SKIP' : ((browserMetrics.consumerCount || 0) > 0 ? 'PASS' : 'FAIL'),
        evidence: `consumerCount=${browserMetrics.consumerCount || 0}`,
      },
      {
        name: 'browser-keyframe-requested',
        status: browserH264Unsupported ? 'SKIP' : ((browserMetrics.keyframeRequests || 0) > 0 ? 'PASS' : 'FAIL'),
        evidence: `keyframeRequests=${browserMetrics.keyframeRequests || 0}`,
      },
      {
        name: 'browser-receiver-media-flow',
        status: browserH264Unsupported ? 'SKIP' : (
          (browserMetrics.delta?.packetsReceived || 0) > 0 &&
          ((browserMetrics.delta?.framesDecoded || 0) > 0 ||
            (browserMetrics.delta?.framesReceived || 0) > 0 ||
            (browserMetrics.delta?.currentTimeMs || 0) > 0 ||
            (browserMetrics.last?.frameWidth || 0) > 0)
            ? 'PASS'
            : 'FAIL'
        ),
        evidence: JSON.stringify(browserMetrics.delta || {}),
      },
      {
        name: 'browser-track-live',
        status: browserH264Unsupported ? 'SKIP' : ((browserMetrics.last?.videos || []).some(video => video.readyState === 'live') ? 'PASS' : 'FAIL'),
        evidence: JSON.stringify(browserMetrics.last?.videos || []),
      },
    ];

    const failed = checks.filter(check => check.status === 'FAIL');
    const skipped = checks.filter(check => check.status === 'SKIP');
    const report = {
      schemaVersion: 1,
      generatedAt: new Date().toISOString().replace(/\.\d{3}Z$/, 'Z'),
      overallStatus: failed.length === 0 ? (skipped.length === 0 ? 'PASS' : 'PARTIAL') : 'FAIL',
      runDir,
      runConfig: {
        sourceMode,
        durationSeconds,
        buildDir,
        workerBin,
        chromiumPath,
        roomId,
        pushPeer,
        browserPeer,
        sfuPort,
        inputFile: inputFile || null,
      },
      case: {
        name: 'browser_receiver',
        status: failed.length === 0 ? (skipped.length === 0 ? 'PASS' : 'PARTIAL') : 'FAIL',
        checks,
        metrics: {
          push: pushMetrics,
          browser: browserMetrics,
        },
        browserError: browserResult.ok ? null : browserResult.error,
        browserErrorCode: browserResult.errorCode || null,
        artifacts: {
          caseDir,
          pushLog: pushMetrics.logPath,
          sfuStdout: path.join(caseDir, 'sfu.stdout.log'),
          chromiumStderr: path.join(caseDir, 'chromium.stderr.log'),
        },
      },
    };
    writeReport({ report, reportJson, reportMd });

    if (failed.length > 0) throw new Error(`browser receiver smoke failed: ${failed.map(check => check.name).join(', ')}`);

    console.log(reportJson);
    console.log(reportMd);
  } finally {
    if (resultServer) await new Promise(resolve => resultServer.close(resolve));
    await stopProcess(chromium);
    await stopProcess(push);
    await stopProcess(sfu);
  }
}

run().catch(error => {
  console.error(error?.stack || error?.message || String(error));
  process.exitCode = 1;
});
