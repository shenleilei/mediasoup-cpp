import net from 'node:net';
import path from 'node:path';
import { spawn } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { createRequire } from 'node:module';
import { ensureSignalingTlsFiles } from './prepare_signaling_tls.mjs';
import { resolveChromiumExecutable } from './browser_runtime_helpers.mjs';
const require = createRequire(import.meta.url);
const puppeteer = require('puppeteer-core');

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');

const rawArgs = process.argv.slice(2);
const localMode = rawArgs.includes('--local');
const observeOnly = rawArgs.includes('--observe-only');
const args = rawArgs.filter(arg => arg !== '--local' && arg !== '--observe-only');
const argOffset = 0;
const defaultRemoteUrl = 'https://volcvideo3.zelostech.com.cn:1770/';
const baseUrlArg = args[argOffset] || defaultRemoteUrl;
const roomId = args[argOffset + 1] || `plain_abs_capture_${Date.now()}`;
const chromiumPath = args[argOffset + 2] || resolveChromiumExecutable();

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

async function allocatePort() {
  return await new Promise((resolve, reject) => {
    const server = net.createServer();
    server.once('error', reject);
    server.listen(0, '127.0.0.1', () => {
      const { port } = server.address();
      server.close(error => error ? reject(error) : resolve(port));
    });
  });
}

async function waitForPort(port, timeoutMs = 12000) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    try {
      await new Promise((resolve, reject) => {
        const socket = net.createConnection({ host: '127.0.0.1', port }, () => {
          socket.destroy();
          resolve();
        });
        socket.once('error', reject);
        socket.setTimeout(500, () => {
          socket.destroy();
          reject(new Error('timeout'));
        });
      });
      return;
    } catch {
      await sleep(100);
    }
  }
  throw new Error(`signaling port ${port} did not become ready`);
}

function stopChild(child, timeoutMs = 3000) {
  return new Promise(resolve => {
    if (!child || !child.pid || child.exitCode !== null) {
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
    child.kill('SIGTERM');
  });
}

function normalizeDemoUrl(rawUrl) {
  const url = new URL(rawUrl);
  if (!url.searchParams.has('qos')) {
    url.searchParams.set('qos', 'full');
  }
  return url.toString();
}

function startLocalSfu(signalingPort, webRtcServerPort) {
  ensureSignalingTlsFiles();
  const child = spawn(
    path.join(repoRoot, 'build', 'mediasoup-sfu'),
    [
      '--nodaemon',
      `--port=${signalingPort}`,
      '--listenIp=127.0.0.1',
      `--webRtcServerPort=${webRtcServerPort}`,
      '--workers=1',
      '--workerBin=./mediasoup-worker',
    ],
    { cwd: repoRoot, stdio: ['ignore', 'pipe', 'pipe'] },
  );

  let log = '';
  const collect = chunk => {
    log += chunk.toString();
    const lines = log.split('\n');
    if (lines.length > 300) {
      log = lines.slice(-300).join('\n');
    }
  };
  child.stdout.on('data', collect);
  child.stderr.on('data', collect);
  return { child, getLog: () => log };
}

async function waitForStatus(page, pattern, timeoutMs = 15000) {
  await page.waitForFunction(
    expected => {
      const el = document.getElementById('status');
      return !!el && new RegExp(expected, 'i').test(el.textContent || '');
    },
    { timeout: timeoutMs },
    pattern,
  );
}

async function setupObserverPage(browser, baseUrl, room) {
  const page = await browser.newPage();
  const logs = [];

  page.on('console', msg => logs.push(`[page:${msg.type()}] ${msg.text()}`));
  page.on('pageerror', err => logs.push(`[pageerror] ${err.message}`));

  await page.goto(baseUrl, { waitUntil: 'domcontentloaded', timeout: 60000 });
  await page.waitForFunction(() => document.readyState === 'complete', { timeout: 15000 }).catch(() => {});
  await page.waitForFunction(() => !!window.__qosDemoDebug && !!window.__qosDemoHarness, { timeout: 15000 });
  await page.waitForSelector('#roomInput');
  await page.$eval('#roomInput', (el, value) => { el.value = value; }, room);
  await page.click('#joinBtn');
  await waitForStatus(page, 'joined|connected|已加入|已连接');

  return { page, logs };
}

async function waitForCaptureMetrics(page, timeoutMs = 15000) {
  const deadline = Date.now() + timeoutMs;

  while (Date.now() < deadline) {
    const sample = await page.evaluate(() => {
      const state = window.__qosDemoDebug?.getState?.();
      const consumers = state?.remoteVideoConsumers instanceof Map
        ? Array.from(state.remoteVideoConsumers.values())
        : Array.isArray(state?.remoteVideoConsumers)
          ? state.remoteVideoConsumers
          : [];
      const primaryConsumer = consumers[0] || null;
      const producerId = primaryConsumer?.producerId || null;
      const peers = Array.isArray(state?.latestStatsReport?.peers) ? state.latestStatsReport.peers : [];
      const remotePeer = peers.find(peer => peer?.producers && producerId && peer.producers[producerId]) || null;
      const remoteProducer = remotePeer?.producers?.[producerId] || null;
      const remoteProducerStat = Array.isArray(remoteProducer?.stats)
        ? remoteProducer.stats.find(item => item && item.type === 'inbound-rtp') || null
        : null;
      const cardItems = Array.from(document.querySelectorAll('#remoteVideos .qos-item')).map(item => ({
        label: item.querySelector('.label')?.textContent?.trim() || '',
        value: item.querySelector('.value')?.textContent?.trim() || '',
      }));
      const byLabel = Object.fromEntries(cardItems.map(item => [item.label, item.value]));
      const videos = Array.from(document.querySelectorAll('#remoteVideos video')).map(video => ({
        readyState: video.readyState,
        width: video.videoWidth,
        height: video.videoHeight,
        currentTime: video.currentTime,
        paused: video.paused,
        tracks: video.srcObject
          ? video.srcObject.getTracks().map(track => ({
              kind: track.kind,
              readyState: track.readyState,
              muted: track.muted,
            }))
          : [],
      }));
      const recvTransport = state?.recvTransport || null;
      return {
        remoteVideoConsumers: state?.remoteVideoConsumers?.size ?? 0,
        producerId,
        remoteProducerStat,
        captureTsText: byLabel['Capture Ts'] || null,
        captureOffsetText: byLabel['Capture Offset'] || null,
        captureToSfuText: byLabel['Capture->SFU'] || null,
        renderSizeText: byLabel['渲染尺寸'] || null,
        fpsText: byLabel['帧率'] || null,
        recvTransportState: recvTransport?.connectionState || null,
        videos,
      };
    });

    const hasRenderableVideo = sample.videos.some(video =>
      video.readyState >= 2 &&
      video.width > 0 &&
      video.height > 0
    );

    if (
      sample.remoteVideoConsumers > 0 &&
      sample.remoteProducerStat?.absCaptureTimestampMs !== undefined &&
      sample.remoteProducerStat?.absCaptureReceiveDeltaMs !== undefined &&
      sample.captureTsText &&
      sample.captureTsText !== '-' &&
      sample.captureToSfuText &&
      sample.captureToSfuText !== '-' &&
      sample.renderSizeText &&
      sample.renderSizeText !== '-' &&
      sample.fpsText &&
      sample.fpsText !== '-' &&
      sample.fpsText !== '0' &&
      sample.recvTransportState === 'connected' &&
      hasRenderableVideo
    ) {
      return sample;
    }

    await sleep(500);
  }

  throw new Error(`capture metrics did not appear within ${timeoutMs}ms`);
}

async function run() {
  let signalingPort = null;
  let sfu = null;
  let baseUrl = normalizeDemoUrl(baseUrlArg);

  if (localMode) {
    signalingPort = await allocatePort();
    const webRtcServerPort = await allocatePort();
    sfu = startLocalSfu(signalingPort, webRtcServerPort);
    baseUrl = normalizeDemoUrl(`https://127.0.0.1:${signalingPort}/`);
    await waitForPort(signalingPort);
  } else {
    signalingPort = Number(new URL(baseUrl).port || 443);
  }

  const browser = await puppeteer.launch({
    executablePath: chromiumPath,
    headless: true,
    protocolTimeout: 120000,
    args: [
      '--no-sandbox',
      '--ignore-certificate-errors',
      '--allow-insecure-localhost',
      '--use-fake-device-for-media-stream',
      '--use-fake-ui-for-media-stream',
      '--autoplay-policy=no-user-gesture-required',
    ],
  });

  let pageLogs = [];
  let page;
  let sender;
  let senderOut = '';

  try {
    ({ page, logs: pageLogs } = await setupObserverPage(browser, baseUrl, roomId));

    if (!observeOnly) {
      const wsUrl = new URL(baseUrl);
      sender = spawn(
        process.execPath,
        [
          path.join(repoRoot, 'tests', 'qos_harness', 'remote_plain_publish_abs_capture_sender.mjs'),
          localMode ? '127.0.0.1' : wsUrl.hostname,
          String(signalingPort),
          roomId,
          'plain-publisher',
          '20000',
          '5000',
        ],
        { cwd: repoRoot, stdio: ['ignore', 'pipe', 'pipe'] },
      );

      sender.stdout.on('data', chunk => { senderOut += chunk.toString(); });
      sender.stderr.on('data', chunk => { senderOut += chunk.toString(); });
    }

    await page.evaluate(() => window.__qosDemoHarness.waitForRemoteVideos(1, 15000));
    const sample = await waitForCaptureMetrics(page, 15000);

    console.log(JSON.stringify({
      ok: true,
      roomId,
      baseUrl,
      mode: localMode ? 'local' : 'remote',
      observeOnly,
      sample,
      senderOutput: senderOut.split('\n').slice(-40),
      pageLogs: pageLogs.slice(-20),
      sfuLogTail: sfu ? sfu.getLog().split('\n').slice(-80) : undefined,
    }, null, 2));
  } catch (error) {
    const pageState = page
      ? await page.evaluate(() => {
          const state = window.__qosDemoDebug?.getState?.();
          return {
            peerId: state?.peerId,
            roomId: state?.roomId,
            remoteVideoConsumers: state?.remoteVideoConsumers?.size ?? 0,
            latestStatsReport: state?.latestStatsReport || null,
          };
        }).catch(sampleError => ({ error: sampleError.message }))
      : null;

    console.log(JSON.stringify({
      ok: false,
      roomId,
      baseUrl,
      mode: localMode ? 'local' : 'remote',
      observeOnly,
      error: error.message,
      pageState,
      senderOutput: senderOut.split('\n').slice(-40),
      pageLogs: pageLogs.slice(-20),
      sfuLogTail: sfu ? sfu.getLog().split('\n').slice(-100) : undefined,
    }, null, 2));
    process.exitCode = 1;
  } finally {
    if (sender) {
      await stopChild(sender);
    }
    await browser.close().catch(() => {});
    if (sfu) {
      await stopChild(sfu.child);
    }
  }
}

run().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
