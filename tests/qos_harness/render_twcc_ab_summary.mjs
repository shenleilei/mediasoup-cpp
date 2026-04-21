/**
 * Render the TWCC A/B summary page from existing pairwise JSON artifacts.
 *
 * Usage:
 *   node tests/qos_harness/render_twcc_ab_summary.mjs \
 *     --output=docs/generated/twcc-ab-report.md \
 *     --raw-json=docs/generated/twcc-ab-raw-groups.json \
 *     --trace-dir=changes/.../raw \
 *     --pair-json=docs/generated/twcc-ab-g1-vs-g2.json \
 *     --pair-json=docs/generated/twcc-ab-g0-vs-g2.json
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import {
  buildPairTakeaways,
  fmtPercent,
  groupProfile,
  isFiniteNumber,
  pairPurpose,
  percentDelta,
  safe,
} from './twcc_ab_report_helpers.mjs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const args = process.argv.slice(2);

const outputArg = args.find(arg => arg.startsWith('--output='));
const rawJsonArg = args.find(arg => arg.startsWith('--raw-json='));
const traceDirArg = args.find(arg => arg.startsWith('--trace-dir='));
const pairJsonArgs = args.filter(arg => arg.startsWith('--pair-json='));

if (!outputArg || !rawJsonArg || pairJsonArgs.length === 0) {
  throw new Error('render_twcc_ab_summary.mjs requires --output, --raw-json, and at least one --pair-json');
}

function resolveArg(value) {
  return path.isAbsolute(value) ? value : path.resolve(repoRoot, value);
}

function markdownLink(fromFilePath, label, targetPath) {
  const relPath = path.relative(path.dirname(fromFilePath), targetPath).split(path.sep).join('/');
  return `[${label}](${relPath})`;
}

function pairAggregates(report) {
  const scenarios = Array.isArray(report?.scenarios) ? report.scenarios : [];
  const average = values => {
    const valid = values.filter(isFiniteNumber);
    if (valid.length === 0) return null;
    return valid.reduce((sum, value) => sum + value, 0) / valid.length;
  };

  return {
    baselineGoodput: average(scenarios.map(item => item?.baseline?.goodputKbps?.mean)),
    candidateGoodput: average(scenarios.map(item => item?.candidate?.goodputKbps?.mean)),
    baselineLoss: average(scenarios.map(item => item?.baseline?.lossPercent?.mean)),
    candidateLoss: average(scenarios.map(item => item?.candidate?.lossPercent?.mean)),
    baselineRecovery: average(scenarios.map(item => item?.baseline?.recoveryTimeMs?.mean)),
    candidateRecovery: average(scenarios.map(item => item?.candidate?.recoveryTimeMs?.mean)),
    baselineFairness: average(scenarios.map(item => item?.baseline?.fairnessDeviation)),
    candidateFairness: average(scenarios.map(item => item?.candidate?.fairnessDeviation)),
  };
}

function pairQuickSummary(report, aggregates) {
  const parts = [];
  const failedGates = (Array.isArray(report?.gates) ? report.gates : []).filter(item => item?.pass === false);
  const goodputDelta = percentDelta(aggregates.baselineGoodput, aggregates.candidateGoodput);
  const recoveryDelta = percentDelta(aggregates.baselineRecovery, aggregates.candidateRecovery);

  parts.push(report?.overall?.pass === true ? '通过全部 gate' : '未通过全部 gate');

  if (isFiniteNumber(goodputDelta)) {
    parts.push(Math.abs(goodputDelta) < 0.5 ? 'goodput 基本持平' : `goodput 变化 ${fmtPercent(goodputDelta)}`);
  }
  if (isFiniteNumber(recoveryDelta)) {
    parts.push(recoveryDelta < 0 ? `recovery 改善 ${fmtPercent(Math.abs(recoveryDelta))}` : `recovery 变慢 ${fmtPercent(recoveryDelta)}`);
  }
  if (failedGates.length > 0) {
    parts.push(`阻塞项：${failedGates.map(item => safe(item.description, item.id)).join('；')}`);
  }

  return `${parts.join('；')}。`;
}

const outputPath = resolveArg(outputArg.slice('--output='.length));
const rawJsonPath = resolveArg(rawJsonArg.slice('--raw-json='.length));
const traceDirPath = traceDirArg
  ? resolveArg(traceDirArg.slice('--trace-dir='.length))
  : null;
const pairJsonPaths = pairJsonArgs.map(arg => resolveArg(arg.slice('--pair-json='.length)));

const raw = JSON.parse(fs.readFileSync(rawJsonPath, 'utf8'));
const pairReports = pairJsonPaths.map(pairJsonPath => {
  const report = JSON.parse(fs.readFileSync(pairJsonPath, 'utf8'));
  return {
    report,
    pairJsonPath,
    pairMarkdownPath: pairJsonPath.replace(/\.json$/, '.md'),
    aggregates: pairAggregates(report),
  };
});

const groups = new Map();
for (const pair of pairReports) {
  for (const label of [pair.report?.baseline?.label, pair.report?.candidate?.label]) {
    const profile = groupProfile(label);
    groups.set(profile.groupId, profile);
  }
}

const lines = [];
lines.push('# TWCC A/B 最新报告');
lines.push('');
lines.push('> 文档性质');
lines.push('>');
lines.push('> 这是 stable TWCC A/B 总入口。');
lines.push('> 它的目标不是替代 pairwise 明细，而是先回答“本次在比什么、怎么读、这次结果大致意味着什么”。');
lines.push('');

lines.push(`生成时间：\`${safe(raw?.generatedAt, pairReports[0]?.report?.generatedAt ?? '<unknown>')}\``);
lines.push('');

lines.push('## 1. 先看这里');
lines.push('');
lines.push('- 如果只关心 transport estimate / TWCC 主路径本身的净收益，先看 `G1 vs G2`。');
lines.push('- 如果只关心新整条发送路径相对旧路径的整体效果，先看 `G0 vs G2`。');
lines.push('- overall `FAIL` 的意思是“没有满足全部预设 gate”，不自动等于运行路径已经坏了。');
lines.push('- 读法建议：先看本页 Pairwise 结果，再进入对应 pairwise 报告看 gate 和场景级结果。');
lines.push('');

lines.push('## 2. 这次运行在比较什么');
lines.push('');
lines.push('| 组 | 运行模式 | 关键开关 | 作用 |');
lines.push('|---|---|---|---|');
for (const profile of [...groups.values()].sort((a, b) => a.groupId.localeCompare(b.groupId))) {
  lines.push(
    `| \`${profile.groupId}\` | ${safe(profile.modeName)} | ${profile.toggles.length > 0 ? profile.toggles.map(item => `\`${item}\``).join('<br>') : '-'} | ${safe(profile.purpose)} |`
  );
}
lines.push('');

lines.push('## 3. 本次运行');
lines.push('');
lines.push(`- 脚本：\`node tests/qos_harness/run_twcc_ab_eval.mjs\``);
lines.push(`- 重复次数：\`${safe(raw?.repetitions, '-')}\``);
lines.push(`- matrix speed：\`${safe(raw?.matrixSpeed, '-')}\``);
lines.push(`- 场景：\`${Array.isArray(raw?.selectedAbIds) ? raw.selectedAbIds.join(',') : '-'}\``);
lines.push(`- Host：\`${safe(raw?.environment?.host, '-')}\``);
lines.push(`- OS：\`${safe(raw?.environment?.os, '-')}\``);
lines.push(`- CPU / Memory：\`${safe(raw?.environment?.cpu, '-')}\` / \`${safe(raw?.environment?.memoryGb, '-')}\` GB`);
lines.push('');

lines.push('## 4. Pairwise 结果');
lines.push('');
lines.push('| Pair | 这组回答什么 | Overall | 这次意味着什么 | Markdown | JSON |');
lines.push('|---|---|---|---|---|---|');
for (const pair of pairReports) {
  const purpose = pairPurpose(pair.report?.baseline?.label, pair.report?.candidate?.label);
  lines.push(
    `| ${safe(pair.report?.baseline?.label)} vs ${safe(pair.report?.candidate?.label)} | ${purpose.short} | \`${pair.report?.overall?.pass === true ? 'PASS' : 'FAIL'}\` | ${pairQuickSummary(pair.report, pair.aggregates)} | ${markdownLink(outputPath, path.basename(pair.pairMarkdownPath), pair.pairMarkdownPath)} | ${markdownLink(outputPath, path.basename(pair.pairJsonPath), pair.pairJsonPath)} |`
  );
}
lines.push('');

lines.push('## 5. 本次结果怎么理解');
lines.push('');
for (const pair of pairReports) {
  lines.push(`### ${safe(pair.report?.baseline?.label)} vs ${safe(pair.report?.candidate?.label)}`);
  lines.push('');
  for (const takeaway of buildPairTakeaways({
    report: pair.report,
    ...pair.aggregates,
  })) {
    lines.push(`- ${takeaway}`);
  }
  lines.push('');
}

lines.push('## 6. 推荐阅读顺序');
lines.push('');
lines.push('1. 先看本页 `Pairwise 结果`，确定要看哪一组对比。');
lines.push('2. 再看对应 pairwise Markdown 里的 `判定门槛检查` 和 `场景级结果`。');
lines.push('3. 只有在需要定位具体原因时，才继续下钻 raw JSON 与 trace/log。');
lines.push('');

lines.push('## 7. 原始产物');
lines.push('');
lines.push(`- ${markdownLink(outputPath, path.basename(rawJsonPath), rawJsonPath)}`);
if (traceDirPath) {
  lines.push(`- 原始 trace/log 目录：${markdownLink(outputPath, path.relative(repoRoot, traceDirPath), traceDirPath)}`);
}
lines.push('');

fs.mkdirSync(path.dirname(outputPath), { recursive: true });
fs.writeFileSync(outputPath, `${lines.join('\n')}\n`);

console.log(`twcc ab summary written to ${outputPath}`);
