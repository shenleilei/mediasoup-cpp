import fs from 'node:fs';
import http from 'node:http';
import os from 'node:os';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { createRequire } from 'node:module';

import {
  deriveCaseEvaluation,
  extractTiming,
  getPhaseNetwork,
  getImpairedStateForEvaluation,
  summarizePhaseState,
  toSyntheticCondition,
} from './synthetic_sweep_shared.mjs';

const require = createRequire(import.meta.url);
const esbuild = require('esbuild');
const puppeteer = require('puppeteer-core');

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const chromiumPath = '/usr/lib64/chromium-browser/headless_shell';

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function buildBundle(tmpDir) {
  const outfile = path.join(tmpDir, 'synthetic-sweep-bundle.js');
  esbuild.buildSync({
    entryPoints: [path.join(__dirname, 'browser', 'sweep-entry.js')],
    outfile,
    bundle: true,
    platform: 'browser',
    format: 'iife',
    target: ['chrome120'],
  });
  return outfile;
}

function startStaticServer(bundlePath) {
  const html = '<!doctype html><html><body><script src="/bundle.js"></script></body></html>';
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
    server.listen(0, '127.0.0.1', () => resolve(server));
  });
}

async function launchBrowser() {
  return puppeteer.launch({
    executablePath: chromiumPath,
    headless: true,
    args: ['--no-sandbox'],
  });
}

async function runCase(page, caseDefn) {
  const { caseId, baselineMs, impairmentMs, recoveryMs } = caseDefn;

  console.error(`[${caseId}] init + synthetic baseline ${baselineMs}ms`);
  await page.evaluate(() => window.__qosSyntheticSweepHarness.init());

  const baselineNetwork = getPhaseNetwork(caseDefn, 'baseline');
  const baselineCond = toSyntheticCondition(baselineNetwork);
  await page.evaluate(condition => window.__qosSyntheticSweepHarness.setCondition(condition), baselineCond);
  await sleep(baselineMs);
  const baseline = await page.evaluate(() => window.__qosSyntheticSweepHarness.getState());
  const baselineEndMs = await page.evaluate(() => Date.now());

  const impairedNetwork = getPhaseNetwork(caseDefn, 'impaired');
  const impairedCond = toSyntheticCondition(impairedNetwork);
  console.error(
    `[${caseId}] impairment ${impairmentMs}ms (send=${impairedCond.bitrateBps} target=${impairedCond.targetBitrateBps} loss=${impairedCond.lossRate} rtt=${impairedCond.rttMs} jitter=${impairedCond.jitterMs})`
  );
  await page.evaluate(condition => window.__qosSyntheticSweepHarness.setCondition(condition), impairedCond);
  await sleep(impairmentMs);
  const impaired = await page.evaluate(() => window.__qosSyntheticSweepHarness.getState());
  const impairmentEndMs = await page.evaluate(() => Date.now());

  const recoveryNetwork = getPhaseNetwork(caseDefn, 'recovery');
  const recoveryCond = toSyntheticCondition(recoveryNetwork);
  console.error(`[${caseId}] recovery ${recoveryMs}ms`);
  await page.evaluate(condition => window.__qosSyntheticSweepHarness.setCondition(condition), recoveryCond);
  await sleep(recoveryMs);
  const recovered = await page.evaluate(() => window.__qosSyntheticSweepHarness.getState());
  const recoveryEndMs = await page.evaluate(() => Date.now());

  const actions = await page.evaluate(() => window.__qosSyntheticSweepHarness.getActions());
  await page.evaluate(() => window.__qosSyntheticSweepHarness.stop());

  const allTrace = recovered.trace || impaired.trace || baseline.trace || [];
  const baselineSummary = summarizePhaseState(allTrace, 0, baseline, baselineEndMs);
  const impairmentSummary = summarizePhaseState(
    allTrace,
    baselineEndMs,
    impaired,
    impairmentEndMs
  );
  const recoverySummary = summarizePhaseState(
    allTrace,
    impairmentEndMs,
    recovered,
    recoveryEndMs
  );
  const impairedStateForEvaluation = getImpairedStateForEvaluation(
    caseDefn,
    impairmentSummary
  );
  const evaluation = deriveCaseEvaluation(
    caseDefn,
    baselineSummary.current,
    impairedStateForEvaluation,
    recoverySummary.best
  );
  const timing = {
    impairment: extractTiming(allTrace, baselineEndMs, impairmentEndMs),
    recovery:
      caseDefn.expect?.recovery === false
        ? {}
        : extractTiming(allTrace, impairmentEndMs, recoveryEndMs),
  };

  return {
    caseId,
    input: {
      bandwidth: caseDefn.bandwidth,
      rtt: caseDefn.rtt,
      loss: caseDefn.loss,
      jitter: caseDefn.jitter,
    },
    baselineNetwork,
    impairedNetwork,
    recoveryNetwork,
    baselineCondition: baselineCond,
    impairedCondition: impairedCond,
    recoveryCondition: recoveryCond,
    baseline: { state: baselineSummary.current.state, level: baselineSummary.current.level },
    impaired: { state: impaired.state, level: impaired.level },
    recovered: { state: recovered.state, level: recovered.level },
    phaseSummary: {
      baseline: baselineSummary,
      impairment: impairmentSummary,
      recovery: recoverySummary,
    },
    timing,
    analysis: evaluation.analysis,
    verdict: {
      passed: evaluation.passed,
      reason: evaluation.reason,
      expectation: caseDefn.expect ?? {},
    },
    actionCount: actions.length,
  };
}

async function main() {
  const args = process.argv.slice(2);
  let caseFilter = 'P0';
  for (const arg of args) {
    if (arg.startsWith('--cases=')) {
      caseFilter = arg.slice(8);
    }
  }

  const allCases = JSON.parse(
    fs.readFileSync(path.join(__dirname, 'scenarios', 'sweep_cases.json'), 'utf8')
  );

  let cases;
  if (caseFilter === 'all') {
    cases = allCases;
  } else if (caseFilter === 'P0') {
    cases = allCases.filter(caseDefn => caseDefn.priority === 'P0');
  } else {
    const ids = new Set(caseFilter.split(','));
    cases = allCases.filter(caseDefn => ids.has(caseDefn.caseId));
  }

  console.error(`Running ${cases.length} synthetic cases (filter=${caseFilter})`);

  const tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-synthetic-sweep-'));
  const bundlePath = buildBundle(tmpDir);
  const server = await startStaticServer(bundlePath);
  const browser = await launchBrowser();
  const serverUrl = `http://127.0.0.1:${server.address().port}`;

  const results = [];

  try {
    for (const caseDefn of cases) {
      const page = await browser.newPage();
      try {
        await page.goto(serverUrl, { waitUntil: 'load' });
        const result = await runCase(page, caseDefn);
        results.push(result);
        const mark = result.verdict?.passed === true ? '✓' : '✗';
        console.error(
          `[${result.caseId}] ${mark} ${result.analysis.verdict} | impaired=${result.phaseSummary.impairment.peak.state}/L${result.phaseSummary.impairment.peak.level} recovered=${result.phaseSummary.recovery.best.state}/L${result.phaseSummary.recovery.best.level}`
        );
      } catch (error) {
        console.error(`[${caseDefn.caseId}] ERROR: ${error.message}`);
        results.push({ caseId: caseDefn.caseId, error: error.message });
      } finally {
        await page.close();
      }
    }
  } finally {
    await browser.close();
    await new Promise(resolve => server.close(resolve));
  }

  console.log(JSON.stringify(results, null, 2));

  const passed = results.filter(result => result.verdict?.passed === true).length;
  const failed = results.filter(result => result.analysis && result.verdict?.passed !== true).length;
  const errors = results.filter(result => result.error).length;

  console.error(
    `\nSynthetic summary: ${passed} 符合, ${failed} 不符合, ${errors} errors out of ${results.length} cases`
  );

  if (failed > 0 || errors > 0) {
    process.exitCode = 1;
  }
}

const isMainModule =
  process.argv[1] &&
  pathToFileURL(process.argv[1]).href === import.meta.url;

if (isMainModule) {
  main().catch(error => {
    console.error(error);
    process.exitCode = 1;
  });
}
