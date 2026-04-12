import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';

import {
  archiveKnownArtifacts,
  defaultCaseReportOutputPath,
  getArchiveDir,
  getReportSetPaths,
  sanitizeArchiveTimestamp,
} from './report_artifacts.mjs';

test('full and targeted report paths stay isolated', () => {
  const repoRoot = '/tmp/mediasoup-cpp';
  const full = getReportSetPaths(repoRoot, 'full');
  const targeted = getReportSetPaths(repoRoot, 'targeted');

  assert.equal(full.matrixJsonPath, '/tmp/mediasoup-cpp/docs/generated/uplink-qos-matrix-report.json');
  assert.equal(full.caseMarkdownPath, '/tmp/mediasoup-cpp/docs/uplink-qos-case-results.md');
  assert.equal(
    targeted.matrixJsonPath,
    '/tmp/mediasoup-cpp/docs/generated/uplink-qos-matrix-report.targeted.json'
  );
  assert.equal(
    targeted.caseMarkdownPath,
    '/tmp/mediasoup-cpp/docs/generated/uplink-qos-case-results.targeted.md'
  );
});

test('archive snapshots use report generated time as directory name', () => {
  assert.equal(
    sanitizeArchiveTimestamp('2026-04-12T08:53:56.649Z'),
    '2026-04-12T08-53-56.649Z'
  );
});

test('known artifacts are archived together with metadata', t => {
  const repoRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'qos-archive-test-'));
  t.after(() => fs.rmSync(repoRoot, { recursive: true, force: true }));

  const generatedAt = '2026-04-12T08:53:56.649Z';
  const fullPaths = getReportSetPaths(repoRoot, 'full');

  fs.mkdirSync(path.dirname(fullPaths.matrixJsonPath), { recursive: true });
  fs.mkdirSync(path.dirname(fullPaths.caseMarkdownPath), { recursive: true });

  fs.writeFileSync(
    fullPaths.matrixJsonPath,
    `${JSON.stringify({ generatedAt, selectedCaseIds: 'all', cases: [] }, null, 2)}\n`
  );
  fs.writeFileSync(
    fullPaths.caseMarkdownPath,
    `# 上行 QoS 逐 Case 最终结果\n\n生成时间：\`${generatedAt}\`\n`
  );

  archiveKnownArtifacts(
    repoRoot,
    [
      {
        path: fullPaths.matrixJsonPath,
        relativePath: fullPaths.matrixJsonRelativePath,
        kind: 'matrixJson',
        runType: 'full',
      },
      {
        path: fullPaths.caseMarkdownPath,
        relativePath: fullPaths.caseMarkdownRelativePath,
        kind: 'caseMarkdown',
        runType: 'full',
      },
    ],
    {
      runType: 'full',
      selectedCaseIds: 'all',
      sourceScript: 'tests/qos_harness/test.report_artifacts.mjs',
    }
  );

  const archiveDir = getArchiveDir(repoRoot, generatedAt);
  const metadata = JSON.parse(
    fs.readFileSync(path.join(archiveDir, 'metadata.json'), 'utf8')
  );

  assert.ok(
    fs.existsSync(path.join(archiveDir, fullPaths.matrixJsonRelativePath))
  );
  assert.ok(
    fs.existsSync(path.join(archiveDir, fullPaths.caseMarkdownRelativePath))
  );
  assert.equal(metadata.generatedAt, generatedAt);
  assert.deepEqual(metadata.runTypes, ['full']);
  assert.deepEqual(metadata.selectedCaseIds, 'all');
  assert.equal(
    metadata.artifacts.matrixJson,
    fullPaths.matrixJsonRelativePath
  );
  assert.equal(
    metadata.artifacts.caseMarkdown,
    fullPaths.caseMarkdownRelativePath
  );
});

test('targeted matrix input defaults to targeted markdown output', () => {
  const repoRoot = '/tmp/mediasoup-cpp';
  const targeted = getReportSetPaths(repoRoot, 'targeted');

  assert.equal(
    defaultCaseReportOutputPath(repoRoot, targeted.matrixJsonPath),
    targeted.caseMarkdownPath
  );
});
