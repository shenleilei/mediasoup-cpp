import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');

function makeBaseCase(caseId, options = {}) {
  const stable = { state: 'stable', level: 0 };
  const result = {
    caseId,
    phaseSummary: {
      baseline: { current: stable, peak: stable, best: stable },
      impairment: { current: stable, peak: stable, best: stable },
      recovery: { current: stable, peak: stable, best: stable },
    },
    baseline: { state: stable },
    impairment: { state: stable },
    recovery: { state: stable },
    actionTypes: [],
    actionCount: 0,
  };

  if (options.includeStoredVerdict !== false) {
    result.verdict = {
      passed: options.verdictPassed ?? false,
      reason: options.verdictReason ?? 'forced-fail',
    };
  }

  return result;
}

function renderWith(scriptName, caseId, options = {}) {
  const tmpRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-render-test-'));
  const inputPath = path.join(tmpRoot, 'input.json');
  const outputPath = path.join(tmpRoot, 'output.md');
  const caseResult = options.caseResult ?? makeBaseCase(caseId);
  const report = {
    generatedAt: '2026-04-28T00:00:00.000Z',
    runner: 'test',
    includedCaseIds: [caseId],
    summary: options.summary ?? {
      total: 1,
      executed: 1,
      passed: 0,
      failed: 1,
      errors: 0,
    },
    cases: [caseResult],
  };

  fs.writeFileSync(inputPath, `${JSON.stringify(report, null, 2)}\n`);
  execFileSync('node', [
    path.join(repoRoot, 'tests/qos_harness', scriptName),
    `--input=${inputPath}`,
    `--output=${outputPath}`,
  ], { cwd: repoRoot });
  const rendered = fs.readFileSync(outputPath, 'utf8');
  fs.rmSync(tmpRoot, { recursive: true, force: true });
  return rendered;
}

test('browser uplink renderer honors stored verdict when it disagrees with derived pass', () => {
  const rendered = renderWith('render_case_report.mjs', 'B1');
  assert.match(rendered, /\| 实际结果 \| FAIL（forced-fail） \|/);
  assert.match(rendered, /\| 重点分析 \| 判定=forced-fail。/);
  assert.match(rendered, /- 通过：`0`/);
  assert.match(rendered, /- 失败：`1`/);
  assert.doesNotMatch(rendered, /\| 实际结果 \| PASS（符合） \|/);
  assert.doesNotMatch(rendered, /\| 重点分析 \| 判定=符合。/);
});
