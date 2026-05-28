import { createRequire } from 'node:module';

const require = createRequire(import.meta.url);
const puppeteer = require('puppeteer-core');

const baseUrl = process.argv[2] || 'https://14.103.165.183:1770/';
const roomId = process.argv[3] || `demo_smoke_${Date.now()}`;
const chromiumPath =
  process.argv[4] ||
  '/usr/lib64/chromium-browser/headless_shell';

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
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

async function setupPage(page, peerLabel, room) {
  const logs = [];
  page.on('console', msg => logs.push(`[${peerLabel}:${msg.type()}] ${msg.text()}`));
  page.on('pageerror', err => logs.push(`[${peerLabel}:pageerror] ${err.message}`));

  await page.goto(baseUrl, { waitUntil: 'domcontentloaded', timeout: 60000 });
  await page.waitForFunction(() => document.readyState === 'complete', { timeout: 15000 }).catch(() => {});
  await page.waitForSelector('#roomInput');
  await page.$eval('#roomInput', (el, value) => { el.value = value; }, room);
  await page.click('#joinBtn');
  await waitForStatus(page, 'joined|connected|已加入|已连接');

  return logs;
}

async function clickPublishAndWait(page) {
  await page.click('#publishBtn');
  await sleep(3000);
}

async function countRemoteVideos(page) {
  return await page.evaluate(() => {
    const remote = document.getElementById('remoteVideos');
    if (!remote) return 0;
    return remote.querySelectorAll('video').length;
  });
}

async function run() {
  const deadline = Date.now() + 45000;
  const browser = await puppeteer.launch({
    executablePath: chromiumPath,
    headless: true,
    protocolTimeout: 120000,
    args: [
      '--no-sandbox',
      '--ignore-certificate-errors',
      '--use-fake-device-for-media-stream',
      '--use-fake-ui-for-media-stream',
      '--autoplay-policy=no-user-gesture-required',
    ],
  });

  const pageA = await browser.newPage();
  const pageB = await browser.newPage();
  let logsA = [];
  let logsB = [];
  const timeline = [];

  const sampleState = async page => {
    try {
      return await page.evaluate(() => ({
        status: document.getElementById('status')?.textContent || '',
        remoteVideos: document.querySelectorAll('#remoteVideos video').length,
        localVideos: document.querySelectorAll('#localVideos video').length,
      }));
    } catch (error) {
      return { error: error.message };
    }
  };

  try {
    logsA = await setupPage(pageA, 'A', roomId);
    logsB = await setupPage(pageB, 'B', roomId);

    await clickPublishAndWait(pageA);
    await clickPublishAndWait(pageB);

    for (let i = 0; i < 20; i += 1) {
      // Poll the rendered DOM instead of waiting on a single fragile promise so
      // the script always emits useful evidence, even if remote rendering stalls.
      const [stateA, stateB] = await Promise.all([sampleState(pageA), sampleState(pageB)]);
      timeline.push({ tick: i, A: stateA, B: stateB });
      if (stateA.remoteVideos > 0 || stateB.remoteVideos > 0) {
        break;
      }
      if (Date.now() >= deadline) {
        break;
      }
      await sleep(1000);
    }

    const [remoteA, remoteB, statusA, statusB] = await Promise.all([
      countRemoteVideos(pageA),
      countRemoteVideos(pageB),
      pageA.$eval('#status', el => el.textContent || ''),
      pageB.$eval('#status', el => el.textContent || ''),
    ]);

    console.log(JSON.stringify({
      ok: remoteA > 0 || remoteB > 0,
      roomId,
      remoteA,
      remoteB,
      statusA,
      statusB,
      timeline,
      logs: [...logsA.slice(-10), ...logsB.slice(-10)],
    }, null, 2));

    if (!(remoteA > 0 || remoteB > 0)) {
      throw new Error('no remote video element observed after publish');
    }
  } catch (error) {
    const [stateA, stateB] = await Promise.all([sampleState(pageA), sampleState(pageB)]);
    console.log(JSON.stringify({
      ok: false,
      roomId,
      error: error.message,
      A: stateA,
      B: stateB,
      timeline,
      logs: [...logsA.slice(-20), ...logsB.slice(-20)],
    }, null, 2));
    process.exitCode = 1;
  } finally {
    await browser.close();
  }
}
run().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
