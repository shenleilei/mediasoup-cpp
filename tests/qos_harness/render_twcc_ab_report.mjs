/**
 * Render TWCC A/B effectiveness report from JSON metrics.
 *
 * Usage:
 *   node tests/qos_harness/render_twcc_ab_report.mjs
 *   node tests/qos_harness/render_twcc_ab_report.mjs --input=path/to/metrics.json --output=path/to/report.md
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const args = process.argv.slice(2);

const DEFAULT_INPUT_REL =
  'changes/2026-04-21-plain-client-sender-transport-control/twcc-ab-metrics-template.json';
const DEFAULT_OUTPUT_REL =
  'changes/2026-04-21-plain-client-sender-transport-control/twcc-ab-result.generated.md';

function resolveArg(value) {
  return path.isAbsolute(value) ? value : path.resolve(repoRoot, value);
}

const inputArg = args.find(arg => arg.startsWith('--input='));
const outputArg = args.find(arg => arg.startsWith('--output='));
const inputPath = inputArg
  ? resolveArg(inputArg.slice('--input='.length))
  : path.resolve(repoRoot, DEFAULT_INPUT_REL);
const outputPath = outputArg
  ? resolveArg(outputArg.slice('--output='.length))
  : path.resolve(repoRoot, DEFAULT_OUTPUT_REL);

const report = JSON.parse(fs.readFileSync(inputPath, 'utf8'));

function isFiniteNumber(value) {
  return typeof value === 'number' && Number.isFinite(value);
}

function fmtNum(value, digits = 2) {
  return isFiniteNumber(value) ? value.toFixed(digits) : '-';
}

function fmtInt(value) {
  return isFiniteNumber(value) ? `${Math.round(value)}` : '-';
}

function fmtPercent(value, digits = 2) {
  return isFiniteNumber(value) ? `${value.toFixed(digits)}%` : '-';
}

function safe(value, fallback = '-') {
  if (value == null) return fallback;
  if (typeof value === 'string' && value.trim() === '') return fallback;
  return String(value);
}

function average(values) {
  const valid = values.filter(isFiniteNumber);
  if (valid.length === 0) return null;
  const total = valid.reduce((sum, v) => sum + v, 0);
  return total / valid.length;
}

function percentDelta(baselineValue, candidateValue) {
  if (!isFiniteNumber(baselineValue) || !isFiniteNumber(candidateValue)) return null;
  if (baselineValue === 0) return null;
  return ((candidateValue - baselineValue) / baselineValue) * 100;
}

function sumQueuePressure(queuePressure = {}) {
  const fields = [
    queuePressure.wouldBlockTotal,
    queuePressure.queuedVideoRetentions,
    queuePressure.audioDeadlineDrops,
    queuePressure.retransmissionDrops,
  ];
  const valid = fields.filter(isFiniteNumber);
  if (valid.length === 0) return null;
  return valid.reduce((sum, value) => sum + value, 0);
}

function formatNetwork(network = {}) {
  return [
    `bw ${safe(network.bandwidthKbps, '-') }kbps`,
    `rtt ${safe(network.rttMs, '-') }ms`,
    `loss ${safe(network.lossRate, '-') }`,
    `jitter ${safe(network.jitterMs, '-') }ms`,
  ].join(' / ');
}

function formatWhiteBox(whiteBox = {}) {
  const parts = [
    `senderUsage=${fmtNum(whiteBox.senderUsageBps, 0)}`,
    `estimate=${fmtNum(whiteBox.transportEstimateBps, 0)}`,
    `pacing=${fmtNum(whiteBox.effectivePacingBps, 0)}`,
    `fb=${fmtNum(whiteBox.feedbackReports, 0)}`,
    `probePkts=${fmtNum(whiteBox.probePackets, 0)}`,
    `probeStarts=${fmtNum(whiteBox.probeClusterStarts, 0)}`,
    `probeDone=${fmtNum(whiteBox.probeClusterCompletes, 0)}`,
    `probeEarlyStop=${fmtNum(whiteBox.probeClusterEarlyStops, 0)}`,
    `probeBytes=${fmtNum(whiteBox.probeBytesSent, 0)}`,
    `rtxSent=${fmtNum(whiteBox.retransmissionSent, 0)}`,
    `qFresh=${fmtNum(whiteBox.queuedFreshVideoPackets, 0)}`,
    `qAudio=${fmtNum(whiteBox.queuedAudioPackets, 0)}`,
    `qRtx=${fmtNum(whiteBox.queuedRetransmissionPackets, 0)}`,
    `probeActive=${whiteBox.probeActiveObserved === true ? 'yes' : whiteBox.probeActiveObserved === false ? 'no' : '-'}`,
  ];
  return parts.join(' / ');
}

const scenarios = Array.isArray(report.scenarios) ? report.scenarios : [];
const gates = Array.isArray(report.gates) ? report.gates : [];
const passedScenarios = scenarios.filter(item => item && item.pass === true).length;
const failedScenarios = scenarios.filter(item => item && item.pass === false).length;

const baselineGoodput = average(
  scenarios.map(item => item?.baseline?.goodputKbps?.mean)
);
const candidateGoodput = average(
  scenarios.map(item => item?.candidate?.goodputKbps?.mean)
);
const baselineLoss = average(
  scenarios.map(item => item?.baseline?.lossPercent?.mean)
);
const candidateLoss = average(
  scenarios.map(item => item?.candidate?.lossPercent?.mean)
);
const baselineRecovery = average(
  scenarios.map(item => item?.baseline?.recoveryTimeMs?.mean)
);
const candidateRecovery = average(
  scenarios.map(item => item?.candidate?.recoveryTimeMs?.mean)
);
const baselineStability = average(
  scenarios.map(item => item?.baseline?.stabilityKbpsP90MinusP50)
);
const candidateStability = average(
  scenarios.map(item => item?.candidate?.stabilityKbpsP90MinusP50)
);
const baselineFairness = average(
  scenarios.map(item => item?.baseline?.fairnessDeviation)
);
const candidateFairness = average(
  scenarios.map(item => item?.candidate?.fairnessDeviation)
);
const baselineQueuePressure = average(
  scenarios.map(item => sumQueuePressure(item?.baseline?.queuePressure))
);
const candidateQueuePressure = average(
  scenarios.map(item => sumQueuePressure(item?.candidate?.queuePressure))
);

const lines = [];
lines.push('# TWCC A/B 有效性评估结果');
lines.push('');
lines.push(`生成时间：\`${safe(report.generatedAt, '<unknown>')}\``);
lines.push('');

lines.push('## 1. 实验信息');
lines.push('');
lines.push('| 字段 | 内容 |');
lines.push('|---|---|');
lines.push(`| Baseline 版本 | \`${safe(report?.baseline?.commit, '-')}\` |`);
lines.push(`| Candidate 版本 | \`${safe(report?.candidate?.commit, '-')}\` |`);
lines.push(`| Host | \`${safe(report?.environment?.host, '-')}\` |`);
lines.push(`| OS / Kernel | \`${safe(report?.environment?.os, '-')}\` / \`${safe(report?.environment?.kernel, '-')}\` |`);
lines.push(`| CPU / Memory | \`${safe(report?.environment?.cpu, '-')}\` / \`${safe(report?.environment?.memoryGb, '-')}\` GB |`);
lines.push(`| 网络注入 | ${safe(report?.environment?.networkSetup, '-')} |`);
lines.push(`| 输入素材 | ${safe(report?.environment?.inputMedia, '-')} |`);
lines.push(`| 每场景重复次数 | ${safe(report?.runConfig?.repetitionsPerScenario, '-')} |`);
lines.push(`| 场景数量 | ${scenarios.length}（PASS=${passedScenarios}, FAIL=${failedScenarios}） |`);
lines.push('');

lines.push('## 2. 汇总');
lines.push('');
lines.push('| 指标 | Baseline | Candidate | 变化 |');
lines.push('|---|---:|---:|---:|');
lines.push(`| 总体 Goodput (kbps) | ${fmtNum(baselineGoodput)} | ${fmtNum(candidateGoodput)} | ${fmtPercent(percentDelta(baselineGoodput, candidateGoodput))} |`);
lines.push(`| 总体 Loss (%) | ${fmtNum(baselineLoss)} | ${fmtNum(candidateLoss)} | ${fmtPercent(percentDelta(baselineLoss, candidateLoss))} |`);
lines.push(`| 恢复时间 (ms) | ${fmtNum(baselineRecovery)} | ${fmtNum(candidateRecovery)} | ${fmtPercent(percentDelta(baselineRecovery, candidateRecovery))} |`);
lines.push(`| 稳态抖动 (P90-P50, kbps) | ${fmtNum(baselineStability)} | ${fmtNum(candidateStability)} | ${fmtPercent(percentDelta(baselineStability, candidateStability))} |`);
lines.push(`| 队列压力（规范化总和） | ${fmtNum(baselineQueuePressure)} | ${fmtNum(candidateQueuePressure)} | ${fmtPercent(percentDelta(baselineQueuePressure, candidateQueuePressure))} |`);
lines.push(`| 多路权重偏差（越小越好） | ${fmtNum(baselineFairness)} | ${fmtNum(candidateFairness)} | ${fmtPercent(percentDelta(baselineFairness, candidateFairness))} |`);
lines.push('');
lines.push(`结论：\`${report?.overall?.pass === true ? 'PASS' : report?.overall?.pass === false ? 'FAIL' : safe(report?.overall?.summary, '-')}\``);
if (report?.overall?.summary) {
  lines.push('');
  lines.push(`说明：${report.overall.summary}`);
}
lines.push('');

lines.push('## 3. 判定门槛检查');
lines.push('');
lines.push('| 规则 | 目标 | 实际 | 结论 |');
lines.push('|---|---|---|---|');
if (gates.length === 0) {
  lines.push('| - | - | - | - |');
} else {
  for (const gate of gates) {
    lines.push(
      `| ${safe(gate.description, gate.id)} | ${safe(gate.target)} | ${safe(gate.actual)} | \`${gate.pass === true ? 'PASS' : gate.pass === false ? 'FAIL' : '-'}\` |`
    );
  }
}
lines.push('');

lines.push('## 4. 场景级结果');
lines.push('');
lines.push('| 场景 | 网络形态 | Baseline | Candidate | 变化 | 结论 |');
lines.push('|---|---|---|---|---|---|');
for (const scenario of scenarios) {
  const baselineSummary =
    `gp=${fmtNum(scenario?.baseline?.goodputKbps?.mean)}, loss=${fmtNum(scenario?.baseline?.lossPercent?.mean)}, rec=${fmtInt(scenario?.baseline?.recoveryTimeMs?.mean)}ms`;
  const candidateSummary =
    `gp=${fmtNum(scenario?.candidate?.goodputKbps?.mean)}, loss=${fmtNum(scenario?.candidate?.lossPercent?.mean)}, rec=${fmtInt(scenario?.candidate?.recoveryTimeMs?.mean)}ms`;
  const deltaGoodput = scenario?.delta?.goodputPercent;
  lines.push(
    `| ${safe(scenario?.id, scenario?.name)} | ${formatNetwork(scenario?.network)} | ${baselineSummary} | ${candidateSummary} | ${fmtPercent(deltaGoodput)} | \`${scenario?.pass === true ? 'PASS' : scenario?.pass === false ? 'FAIL' : '-'}\` |`
  );
}
if (scenarios.length === 0) {
  lines.push('| - | - | - | - | - | - |');
}
lines.push('');

lines.push('## 5. 逐场景明细');
lines.push('');
for (const scenario of scenarios) {
  lines.push(`### ${safe(scenario?.id, scenario?.name)}`);
  lines.push('');
  lines.push('| 字段 | 内容 |');
  lines.push('|---|---|');
  lines.push(`| 场景名 | ${safe(scenario?.name, '-')} |`);
  lines.push(`| 网络形态 | ${formatNetwork(scenario?.network)} |`);
  lines.push(`| Baseline Goodput (mean/p50/p90) | ${fmtNum(scenario?.baseline?.goodputKbps?.mean)} / ${fmtNum(scenario?.baseline?.goodputKbps?.p50)} / ${fmtNum(scenario?.baseline?.goodputKbps?.p90)} kbps |`);
  lines.push(`| Candidate Goodput (mean/p50/p90) | ${fmtNum(scenario?.candidate?.goodputKbps?.mean)} / ${fmtNum(scenario?.candidate?.goodputKbps?.p50)} / ${fmtNum(scenario?.candidate?.goodputKbps?.p90)} kbps |`);
  lines.push(`| Baseline Loss (mean/p50/p90) | ${fmtNum(scenario?.baseline?.lossPercent?.mean)} / ${fmtNum(scenario?.baseline?.lossPercent?.p50)} / ${fmtNum(scenario?.baseline?.lossPercent?.p90)} % |`);
  lines.push(`| Candidate Loss (mean/p50/p90) | ${fmtNum(scenario?.candidate?.lossPercent?.mean)} / ${fmtNum(scenario?.candidate?.lossPercent?.p50)} / ${fmtNum(scenario?.candidate?.lossPercent?.p90)} % |`);
  lines.push(`| Baseline Recovery (mean/p50/p90) | ${fmtInt(scenario?.baseline?.recoveryTimeMs?.mean)} / ${fmtInt(scenario?.baseline?.recoveryTimeMs?.p50)} / ${fmtInt(scenario?.baseline?.recoveryTimeMs?.p90)} ms |`);
  lines.push(`| Candidate Recovery (mean/p50/p90) | ${fmtInt(scenario?.candidate?.recoveryTimeMs?.mean)} / ${fmtInt(scenario?.candidate?.recoveryTimeMs?.p50)} / ${fmtInt(scenario?.candidate?.recoveryTimeMs?.p90)} ms |`);
  lines.push(`| 队列压力 Baseline/Candidate | ${fmtNum(sumQueuePressure(scenario?.baseline?.queuePressure), 0)} / ${fmtNum(sumQueuePressure(scenario?.candidate?.queuePressure), 0)} |`);
  lines.push(`| 白盒 Baseline | ${formatWhiteBox(scenario?.baseline?.whiteBox)} |`);
  lines.push(`| 白盒 Candidate | ${formatWhiteBox(scenario?.candidate?.whiteBox)} |`);
  lines.push(`| 权重偏差 Baseline/Candidate | ${fmtNum(scenario?.baseline?.fairnessDeviation)} / ${fmtNum(scenario?.candidate?.fairnessDeviation)} |`);
  lines.push(`| 变化（goodput/loss/recovery/queue/fairness） | ${fmtPercent(scenario?.delta?.goodputPercent)} / ${fmtPercent(scenario?.delta?.lossPercent)} / ${fmtPercent(scenario?.delta?.recoveryTimePercent)} / ${fmtPercent(scenario?.delta?.queuePressurePercent)} / ${fmtPercent(scenario?.delta?.fairnessDeviationPercent)} |`);
  lines.push(`| 结论 | \`${scenario?.pass === true ? 'PASS' : scenario?.pass === false ? 'FAIL' : '-'}\` |`);
  lines.push(`| 分析 | ${safe(scenario?.notes)} |`);
  lines.push('');
}

lines.push('## 6. 产物链接');
lines.push('');
lines.push(`- 原始指标 JSON：\`${safe(report?.artifacts?.rawMetricsPath, '-')}\``);
lines.push(`- 聚合对比 JSON：\`${safe(report?.artifacts?.comparisonReportPath, '-')}\``);
lines.push(`- 日志与 trace：\`${safe(report?.artifacts?.logsPath, '-')}\``);
lines.push('');

fs.mkdirSync(path.dirname(outputPath), { recursive: true });
fs.writeFileSync(outputPath, `${lines.join('\n')}\n`);

console.log(`twcc ab report written to ${outputPath}`);
