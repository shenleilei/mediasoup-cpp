import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn, spawnSync } from 'node:child_process';

import puppeteer from 'puppeteer-core';

import { ensureHarnessMp4, sleep, stopChild, waitForPort } from './cpp_client_runner.mjs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const chromiumPath = '/usr/lib64/chromium-browser/headless_shell';
const signalingPort = Number(process.env.QOS_BROWSER_PUBLIC_INTEROP_PORT || 14022);

function createBufferCollector(lines) {
  return chunk => {
    const text = chunk.toString('utf8');
    const split = text.split(/\r?\n/).filter(Boolean);
    lines.push(...split);
  };
}

function tailLines(lines, count = 40) {
  return lines.slice(-count).join('\n');
}

function startSfu() {
  const stdout = [];
  const stderr = [];
  const child = spawn(
    path.join(repoRoot, 'build', 'mediasoup-sfu'),
    [
      '--nodaemon',
      `--port=${signalingPort}`,
      '--workers=1',
      '--workerBin=./mediasoup-worker',
      '--announcedIp=127.0.0.1',
      '--listenIp=127.0.0.1',
      '--redisHost=0.0.0.0',
      '--redisPort=1',
      '--noRedisRequired',
      '--recordDir=',
      '--logLevel=debug',
    ],
    {
      cwd: repoRoot,
      stdio: ['ignore', 'pipe', 'pipe'],
    },
  );

  child.stdout.on('data', createBufferCollector(stdout));
  child.stderr.on('data', createBufferCollector(stderr));

  return { child, stdout, stderr };
}

function killWorkerChildrenOf(parentPid) {
  const result = spawnSync('pgrep', ['-P', String(parentPid)], { encoding: 'utf8' });
  if (result.status !== 0 || !result.stdout) {
    return;
  }
  for (const line of result.stdout.split(/\r?\n/)) {
    const pid = Number(line.trim());
    if (Number.isInteger(pid) && pid > 0) {
      try {
        process.kill(pid, 'SIGKILL');
      } catch {
        // Ignore already-exited children.
      }
    }
  }
}

function startPlainClient(roomId, peerId, mediaPath, { videoCodec = 'h264', copyMode = true } = {}) {
  const stdout = [];
  const stderr = [];
  const args = [
    '127.0.0.1',
    String(signalingPort),
    roomId,
    peerId,
    mediaPath,
  ];
  if (copyMode) {
    args.push('--copy');
  }
  const child = spawn(
    path.join(repoRoot, 'client', 'build', 'plain-client'),
    args,
    {
      cwd: repoRoot,
      stdio: ['ignore', 'pipe', 'pipe'],
      env: {
        ...process.env,
        PLAIN_CLIENT_VIDEO_TRACK_COUNT: '1',
        PLAIN_CLIENT_VIDEO_CODEC: videoCodec,
        PLAIN_CLIENT_DISABLE_QOS: '1',
      },
    },
  );

  child.stdout.on('data', createBufferCollector(stdout));
  child.stderr.on('data', createBufferCollector(stderr));

  return { child, stdout, stderr };
}

async function waitForPlainClientWarmup(plain, timeoutMs = 4000) {
  const deadline = Date.now() + timeoutMs;

  while (Date.now() < deadline) {
    if (plain.child.exitCode !== null) {
      throw new Error(
        `plain-client exited early (code=${plain.child.exitCode})\n${tailLines(plain.stdout)}\n${tailLines(plain.stderr)}`
      );
    }
    await sleep(100);
  }
}

async function waitForPlainClientLog(plain, needle, timeoutMs = 12000) {
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    if (plain.child.exitCode !== null) {
      throw new Error(
        `plain-client exited early (code=${plain.child.exitCode})\n${tailLines(plain.stdout)}\n${tailLines(plain.stderr)}`
      );
    }
    if (plain.stdout.some(line => line.includes(needle)) || plain.stderr.some(line => line.includes(needle))) {
      return;
    }
    await sleep(100);
  }
  throw new Error(`plain-client log did not contain "${needle}" in time\n${tailLines(plain.stdout)}\n${tailLines(plain.stderr)}`);
}

async function launchBrowser() {
  return puppeteer.launch({
    executablePath: chromiumPath,
    headless: true,
    protocolTimeout: 60000,
    args: [
      '--no-sandbox',
      '--use-fake-ui-for-media-stream',
      '--use-fake-device-for-media-stream',
      '--autoplay-policy=no-user-gesture-required',
    ],
  });
}

async function openDemoPage(browser, name) {
  const page = await browser.newPage();
  const consoleLines = [];
  const pageErrors = [];

  page.on('console', message => {
    consoleLines.push(`[${name}] ${message.type()}: ${message.text()}`);
  });
  page.on('pageerror', error => {
    pageErrors.push(`[${name}] ${error.stack || error.message}`);
  });

  await page.goto(`http://127.0.0.1:${signalingPort}/`, { waitUntil: 'load' });

  return { page, consoleLines, pageErrors };
}

async function joinRoom(page, roomId) {
  await page.$eval('#roomInput', (el, value) => {
    el.value = value;
  }, roomId);
  await page.click('#joinBtn');
  await page.waitForFunction(
    () => document.querySelector('#status')?.textContent.includes('Joined room'),
    { timeout: 15000 },
  );
}

async function publishFromPage(page) {
  await page.click('#publishBtn');
  await page.waitForFunction(
    () => document.querySelector('#status')?.textContent.includes('Publishing'),
    { timeout: 20000 },
  );
}

async function waitForRemoteVideo(page, timeoutMs = 20000) {
  await page.waitForFunction(
    () => {
      const videos = Array.from(document.querySelectorAll('#remoteVideos video'));
      return videos.some(video =>
        video.videoWidth > 0 &&
        video.videoHeight > 0 &&
        video.readyState >= HTMLMediaElement.HAVE_CURRENT_DATA &&
        video.currentTime > 0
      );
    },
    { timeout: timeoutMs },
  );
}

async function snapshotPageState(context) {
  return {
    status: await context.page.$eval('#status', el => el.textContent),
    log: await context.page.$eval('#log', el => el.textContent),
    remoteVideos: await context.page.$$eval('#remoteVideos video', els =>
      els.map(video => ({
        width: video.videoWidth,
        height: video.videoHeight,
        readyState: video.readyState,
        currentTime: video.currentTime,
        muted: video.muted,
      })),
    ),
    console: context.consoleLines.slice(),
    pageErrors: context.pageErrors.slice(),
  };
}

async function runWebToWebCase(browser) {
  const roomId = `public_web_${Date.now()}`;
  const publisher = await openDemoPage(browser, 'web-pub');
  const subscriber = await openDemoPage(browser, 'web-sub');

  try {
    await joinRoom(publisher.page, roomId);
    await joinRoom(subscriber.page, roomId);
    await publishFromPage(publisher.page);
    await waitForRemoteVideo(subscriber.page);
  } catch (error) {
    const publisherState = await snapshotPageState(publisher);
    const subscriberState = await snapshotPageState(subscriber);
    throw new Error(
      `web->web interop failed: ${error.message}\n` +
      `publisher=${JSON.stringify(publisherState, null, 2)}\n` +
      `subscriber=${JSON.stringify(subscriberState, null, 2)}`
    );
  } finally {
    await publisher.page.close();
    await subscriber.page.close();
  }
}

async function waitForRecoveryStatus(page, timeoutMs = 30000) {
  await page.waitForFunction(
    () => document.querySelector('#status')?.textContent.includes('Recovered'),
    { timeout: timeoutMs },
  );
}

async function runWebToWebRecoveryCase(browser, sfu) {
  const roomId = `public_recover_${Date.now()}`;
  const publisher = await openDemoPage(browser, 'web-recover-pub');
  const subscriber = await openDemoPage(browser, 'web-recover-sub');

  try {
    await joinRoom(publisher.page, roomId);
    await joinRoom(subscriber.page, roomId);
    await publishFromPage(publisher.page);
    await waitForRemoteVideo(subscriber.page);

    killWorkerChildrenOf(sfu.child.pid);

    await waitForRecoveryStatus(publisher.page);
    await waitForRecoveryStatus(subscriber.page);
    await waitForRemoteVideo(subscriber.page, 30000);
  } catch (error) {
    const publisherState = await snapshotPageState(publisher);
    const subscriberState = await snapshotPageState(subscriber);
    throw new Error(
      `web->web recovery failed: ${error.message}\n` +
      `publisher=${JSON.stringify(publisherState, null, 2)}\n` +
      `subscriber=${JSON.stringify(subscriberState, null, 2)}`
    );
  } finally {
    await publisher.page.close();
    await subscriber.page.close();
  }
}

async function runPlainClientToWebCase(browser) {
  const roomId = `public_plain_${Date.now()}`;
  const peerId = 'plain_cpp_single_bg';
  const mediaPath = ensureHarnessMp4();
  const subscriber = await openDemoPage(browser, 'plain-sub');

  try {
    await joinRoom(subscriber.page, roomId);
    const plain = startPlainClient(roomId, peerId, mediaPath, {
      videoCodec: 'vp8',
      copyMode: false,
    });

    try {
      await waitForPlainClientWarmup(plain);
      await waitForRemoteVideo(subscriber.page);
    } catch (error) {
      const subscriberState = await snapshotPageState(subscriber);
      throw new Error(
        `plain-client->web interop failed: ${error.message}\n` +
        `subscriber=${JSON.stringify(subscriberState, null, 2)}\n` +
        `plain_stdout=${tailLines(plain.stdout)}\n` +
        `plain_stderr=${tailLines(plain.stderr)}`
      );
    } finally {
      await stopChild(plain.child, 3000);
    }
  } finally {
    await subscriber.page.close();
  }
}

async function runPlainClientToWebRecoveryCase(browser, sfu) {
  const roomId = `public_plain_recover_${Date.now()}`;
  const peerId = 'plain_cpp_recover';
  const mediaPath = ensureHarnessMp4();
  const subscriber = await openDemoPage(browser, 'plain-recover-sub');

  try {
    await joinRoom(subscriber.page, roomId);
    const plain = startPlainClient(roomId, peerId, mediaPath, {
      videoCodec: 'vp8',
      copyMode: false,
    });

    try {
      await waitForPlainClientWarmup(plain);
      await waitForRemoteVideo(subscriber.page);

      killWorkerChildrenOf(sfu.child.pid);

      await waitForPlainClientLog(plain, 'serverRestart received', 15000);
      await waitForPlainClientLog(plain, 'recovered session', 15000);
      await waitForRecoveryStatus(subscriber.page, 30000);
      await waitForRemoteVideo(subscriber.page, 30000);
    } catch (error) {
      const subscriberState = await snapshotPageState(subscriber);
      throw new Error(
        `plain-client->web recovery failed: ${error.message}\n` +
        `subscriber=${JSON.stringify(subscriberState, null, 2)}\n` +
        `plain_stdout=${tailLines(plain.stdout)}\n` +
        `plain_stderr=${tailLines(plain.stderr)}`
      );
    } finally {
      await stopChild(plain.child, 3000);
    }
  } finally {
    await subscriber.page.close();
  }
}

async function main() {
  const sfu = startSfu();
  const browser = await launchBrowser();

  try {
    await waitForPort('127.0.0.1', signalingPort, 10000);
    await runWebToWebCase(browser);
    console.log('[PASS] public-demo web->web renders remote video');

    await runWebToWebRecoveryCase(browser, sfu);
    console.log('[PASS] public-demo web->web recovers after worker restart');

    await runPlainClientToWebCase(browser);
    console.log('[PASS] public-demo plain-client->web renders remote video');

    await runPlainClientToWebRecoveryCase(browser, sfu);
    console.log('[PASS] public-demo plain-client->web recovers after worker restart');

    console.log('\nbrowser_public_interop: all cases passed');
  } catch (error) {
    throw new Error(
      `${error.message}\n` +
      `sfu_stdout=${tailLines(sfu.stdout)}\n` +
      `sfu_stderr=${tailLines(sfu.stderr)}`
    );
  } finally {
    await browser.close();
    await stopChild(sfu.child, 3000);
  }
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
