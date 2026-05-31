import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';
import { spawn } from 'node:child_process';
import net from 'node:net';
import path from 'node:path';
import { ensureSignalingTlsFiles } from './prepare_signaling_tls.mjs';
import { resolveChromiumExecutable } from './browser_runtime_helpers.mjs';

const require = createRequire(import.meta.url);
const puppeteer = require('puppeteer-core');

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');

const args = process.argv.slice(2);
const localMode = args[0] === '--local';
const argOffset = localMode ? 1 : 0;
const defaultRemoteUrl = 'https://14.103.165.183:1770/';
const baseUrlArg = args[argOffset] || defaultRemoteUrl;
const roomId = args[argOffset + 1] || `demo_smoke_${Date.now()}`;
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
    if (lines.length > 200) {
      log = lines.slice(-200).join('\n');
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

async function setupPage(browser, peerLabel, baseUrl, room) {
  const page = await browser.newPage();
  const logs = [];

  page.on('console', msg => logs.push(`[${peerLabel}:${msg.type()}] ${msg.text()}`));
  page.on('pageerror', err => logs.push(`[${peerLabel}:pageerror] ${err.message}`));

  await page.goto(baseUrl, { waitUntil: 'domcontentloaded', timeout: 60000 });
  await page.waitForFunction(() => document.readyState === 'complete', { timeout: 15000 }).catch(() => {});
  await page.waitForFunction(() => !!window.__qosDemoDebug && !!window.__qosDemoHarness, { timeout: 15000 });
  await page.waitForSelector('#roomInput');
  await page.$eval('#roomInput', (el, value) => { el.value = value; }, room);
  await page.click('#joinBtn');
  await waitForStatus(page, 'joined|connected|已加入|已连接');

  return { page, logs };
}

async function clickPublish(page) {
  await page.evaluate(() => window.__qosDemoHarness.publish());
}

async function summarizeRtcStats(transport) {
  if (!transport || typeof transport.getStats !== 'function') {
    return { total: 0, types: {}, inbound: [], outbound: [], candidates: [] };
  }

  const report = await transport.getStats();
  const values = Array.from(report.values ? report.values() : report);
  const types = {};
  for (const item of values) {
    types[item.type] = (types[item.type] || 0) + 1;
  }

  return {
    total: values.length,
    types,
    inbound: values
      .filter(item => item.type === 'inbound-rtp')
      .map(item => ({
        id: item.id,
        kind: item.kind || item.mediaType,
        packetsReceived: item.packetsReceived,
        bytesReceived: item.bytesReceived,
        framesDecoded: item.framesDecoded,
        framesReceived: item.framesReceived,
      })),
    outbound: values
      .filter(item => item.type === 'outbound-rtp')
      .map(item => ({
        id: item.id,
        kind: item.kind || item.mediaType,
        packetsSent: item.packetsSent,
        bytesSent: item.bytesSent,
        framesEncoded: item.framesEncoded,
      })),
    candidates: values
      .filter(item => item.type === 'remote-candidate')
      .map(item => ({
        address: item.address,
        ip: item.ip,
        port: item.port,
        protocol: item.protocol,
        candidateType: item.candidateType,
      })),
  };
}

async function samplePage(page) {
  return await page.evaluate(async () => {
    const state = window.__qosDemoDebug?.getState?.();
    const remoteVideos = Array.from(document.querySelectorAll('#remoteVideos video')).map(video => ({
      readyState: video.readyState,
      paused: video.paused,
      width: video.videoWidth,
      height: video.videoHeight,
      currentTime: video.currentTime,
      tracks: video.srcObject
        ? video.srcObject.getTracks().map(track => ({
          kind: track.kind,
          readyState: track.readyState,
          muted: track.muted,
          enabled: track.enabled,
        }))
        : [],
    }));
    const remoteCards = Array.from(document.querySelectorAll('#remoteVideos .video-card')).map(card => ({
      items: Object.fromEntries(
        Array.from(card.querySelectorAll('.qos-item')).map(item => [
          item.querySelector('.label')?.textContent?.trim() || '',
          item.querySelector('.value')?.textContent?.trim() || '',
        ])
      ),
    }));
    const statsPeers = state?.latestStatsReport?.peers || [];

    return {
      status: document.getElementById('status')?.textContent || '',
      peerId: state?.peerId,
      roomId: state?.roomId,
      sendState: state?.sendTransport?.connectionState,
      recvState: state?.recvTransport?.connectionState,
      remoteVideoCount: remoteVideos.length,
      remoteVideos,
      remoteCards,
      publishedProducerCount: state?.publishedProducers?.size,
      statsReport: state?.latestStatsReport
        ? {
          peerCount: statsPeers.length,
          peers: statsPeers.map(peer => ({
            peerId: peer.peerId,
            producerCount: peer.producerCount,
            consumerCount: peer.consumerCount,
            hasClientStats: !!peer.clientStats,
            hasDownlinkClientStats: !!peer.downlinkClientStats,
            qosQuality: peer.qosQuality,
            downlinkHealth: peer.downlinkHealth,
          })),
        }
        : null,
      localDebugStats: state?.localDebugStats || null,
      sendStats: await window.__browserDemoSmokeSummarizeStats(state?.sendTransport),
      recvStats: await window.__browserDemoSmokeSummarizeStats(state?.recvTransport),
    };
  });
}

function hasDecodedRemoteVideo(state) {
  return state.remoteVideos.some(video =>
    video.readyState >= 2 &&
    video.width > 0 &&
    video.height > 0 &&
    video.tracks.some(track => track.kind === 'video' && track.readyState === 'live' && track.muted === false)
  );
}

function hasInboundVideoStats(state) {
  return state.recvStats.inbound.some(item =>
    item.kind === 'video' &&
    (item.packetsReceived || 0) > 0 &&
    ((item.bytesReceived || 0) > 0 || (item.framesDecoded || 0) > 0 || (item.framesReceived || 0) > 0)
  );
}

function pagePassed(state) {
  const primaryCard = Array.isArray(state.remoteCards) ? state.remoteCards[0]?.items || null : null;
  const cardHasMetrics = primaryCard &&
    primaryCard['Track'] &&
    primaryCard['Track'] !== '-' &&
    primaryCard['Producer Score'] &&
    primaryCard['Producer Score'] !== '-' &&
    primaryCard['Producer Bitrate'] &&
    primaryCard['Producer Bitrate'] !== '-' &&
    primaryCard['Producer Packets'] &&
    primaryCard['Producer Packets'] !== '-' &&
    primaryCard['Producer RTT'] &&
    primaryCard['Producer RTT'] !== '-' &&
    primaryCard['Capture Ts'] === 'n/a' &&
    primaryCard['Capture Offset'] === 'n/a' &&
    primaryCard['Capture->SFU'] === 'n/a' &&
    primaryCard['渲染尺寸'] &&
    primaryCard['渲染尺寸'] !== '-' &&
    primaryCard['帧率'] &&
    primaryCard['帧率'] !== '-' &&
    primaryCard['帧率'] !== '0' &&
    primaryCard['Audio Conceal'] === 'n/a';

  return state.sendState === 'connected' &&
    state.recvState === 'connected' &&
    state.remoteVideoCount > 0 &&
    hasDecodedRemoteVideo(state) &&
    hasInboundVideoStats(state) &&
    state.statsReport?.peerCount >= 2 &&
    state.statsReport.peers.every(peer => peer.hasClientStats && peer.hasDownlinkClientStats) &&
    cardHasMetrics;
}

async function installStatsHelper(page) {
  await page.evaluate(() => {
    window.__browserDemoSmokeSummarizeStats = async transport => {
      if (!transport || typeof transport.getStats !== 'function') {
        return { total: 0, types: {}, inbound: [], outbound: [], candidates: [] };
      }

      const report = await transport.getStats();
      const values = Array.from(report.values ? report.values() : report);
      const types = {};
      for (const item of values) {
        types[item.type] = (types[item.type] || 0) + 1;
      }

      return {
        total: values.length,
        types,
        inbound: values
          .filter(item => item.type === 'inbound-rtp')
          .map(item => ({
            id: item.id,
            kind: item.kind || item.mediaType,
            packetsReceived: item.packetsReceived,
            bytesReceived: item.bytesReceived,
            framesDecoded: item.framesDecoded,
            framesReceived: item.framesReceived,
          })),
        outbound: values
          .filter(item => item.type === 'outbound-rtp')
          .map(item => ({
            id: item.id,
            kind: item.kind || item.mediaType,
            packetsSent: item.packetsSent,
            bytesSent: item.bytesSent,
            framesEncoded: item.framesEncoded,
          })),
        candidates: values
          .filter(item => item.type === 'remote-candidate')
          .map(item => ({
            address: item.address,
            ip: item.ip,
            port: item.port,
            protocol: item.protocol,
            candidateType: item.candidateType,
          })),
      };
    };
  });
}

async function run() {
  let sfu = null;
  let baseUrl = normalizeDemoUrl(baseUrlArg);

  if (localMode) {
    const signalingPort = await allocatePort();
    const webRtcServerPort = await allocatePort();
    sfu = startLocalSfu(signalingPort, webRtcServerPort);
    await waitForPort(signalingPort);
    baseUrl = normalizeDemoUrl(`https://127.0.0.1:${signalingPort}/`);
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

  let pageA;
  let pageB;
  let logsA = [];
  let logsB = [];
  const timeline = [];

  try {
    ({ page: pageA, logs: logsA } = await setupPage(browser, 'A', baseUrl, roomId));
    ({ page: pageB, logs: logsB } = await setupPage(browser, 'B', baseUrl, roomId));
    await installStatsHelper(pageA);
    await installStatsHelper(pageB);

    await clickPublish(pageA);
    await clickPublish(pageB);

    let stateA = null;
    let stateB = null;
    const deadline = Date.now() + 45000;
    let tick = 0;

    while (Date.now() < deadline) {
      await sleep(1000);
      [stateA, stateB] = await Promise.all([samplePage(pageA), samplePage(pageB)]);
      timeline.push({
        tick,
        A: {
          sendState: stateA.sendState,
          recvState: stateA.recvState,
          remoteVideoCount: stateA.remoteVideoCount,
          decoded: hasDecodedRemoteVideo(stateA),
          inboundVideo: hasInboundVideoStats(stateA),
          statsPeers: stateA.statsReport?.peerCount || 0,
        },
        B: {
          sendState: stateB.sendState,
          recvState: stateB.recvState,
          remoteVideoCount: stateB.remoteVideoCount,
          decoded: hasDecodedRemoteVideo(stateB),
          inboundVideo: hasInboundVideoStats(stateB),
          statsPeers: stateB.statsReport?.peerCount || 0,
        },
      });
      tick += 1;

      if (pagePassed(stateA) && pagePassed(stateB)) {
        break;
      }
    }

    const ok = pagePassed(stateA) && pagePassed(stateB);
    console.log(JSON.stringify({
      ok,
      mode: localMode ? 'local' : 'remote',
      baseUrl,
      roomId,
      A: stateA,
      B: stateB,
      timeline,
      logs: [...logsA.slice(-20), ...logsB.slice(-20)],
      sfuLogTail: sfu ? sfu.getLog().split('\n').slice(-50) : undefined,
    }, null, 2));

    if (!ok) {
      throw new Error('public demo smoke did not reach connected media + inbound stats + statsReport on both peers');
    }
  } catch (error) {
    const safeSample = async page => {
      if (!page) return null;
      try { return await samplePage(page); } catch (sampleError) { return { error: sampleError.message }; }
    };
    console.log(JSON.stringify({
      ok: false,
      mode: localMode ? 'local' : 'remote',
      baseUrl,
      roomId,
      error: error.message,
      A: await safeSample(pageA),
      B: await safeSample(pageB),
      timeline,
      logs: [...logsA.slice(-20), ...logsB.slice(-20)],
      sfuLogTail: sfu ? sfu.getLog().split('\n').slice(-80) : undefined,
    }, null, 2));
    process.exitCode = 1;
  } finally {
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
