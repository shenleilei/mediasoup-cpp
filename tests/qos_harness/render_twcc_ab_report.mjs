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

import {
  AB_SCENARIO_GUIDE,
  average,
  buildPairTakeaways,
  fmtInt,
  fmtNum,
  fmtPercent,
  formatNetwork,
  formatWhiteBox,
  groupProfile,
  percentDelta,
  safe,
  sumQueuePressure,
} from './twcc_ab_report_helpers.mjs';

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

const baselineProfile = groupProfile(report?.baseline?.label);
const candidateProfile = groupProfile(report?.candidate?.label);
const takeaways = buildPairTakeaways({
  report,
  baselineGoodput,
  candidateGoodput,
  baselineLoss,
  candidateLoss,
  baselineRecovery,
  candidateRecovery,
  baselineFairness,
  candidateFairness,
});

const lines = [];
lines.push('# TWCC A/B 有效性评估结果');
lines.push('');
lines.push('> 文档性质');
lines.push('>');
lines.push('> 这是一份 pairwise TWCC A/B 报告。');
lines.push('> 它回答的是：在当前一组预设 gate 下，candidate runtime mode 相对 baseline runtime mode 是否足够好。');
lines.push('>');
lines.push('> 这里的 overall `FAIL` 只表示“至少一条 gate 没过”，');
lines.push('> 不自动等于 “TWCC 路径已经坏了” 或 “功能不可用”。');
lines.push('');

lines.push('## 1. 先看这里');
lines.push('');
for (const takeaway of takeaways) {
  lines.push(`- ${takeaway}`);
}
if (takeaways.length === 0) {
  lines.push('- 本次报告没有可自动提炼的 reader-facing 结论，请直接看 gate 与场景级结果。');
}
lines.push('');

lines.push('## 2. 比较对象');
lines.push('');
lines.push('| 角色 | 标签 | 运行模式 | 关键开关 |');
lines.push('|---|---|---|---|');
lines.push(
  `| Baseline | \`${safe(report?.baseline?.label, '-')}\` | ${safe(baselineProfile.modeName)} | ${baselineProfile.toggles.length > 0 ? baselineProfile.toggles.map(item => `\`${item}\``).join('<br>') : '-'} |`
);
lines.push(
  `| Candidate | \`${safe(report?.candidate?.label, '-')}\` | ${safe(candidateProfile.modeName)} | ${candidateProfile.toggles.length > 0 ? candidateProfile.toggles.map(item => `\`${item}\``).join('<br>') : '-'} |`
);
lines.push('');
lines.push(`- 这组对比回答：${safe(report?.baseline?.label)} -> ${safe(report?.candidate?.label)} 是否满足当前 TWCC A/B 门槛。`);
lines.push(`- Baseline 侧重点：${safe(baselineProfile.purpose)}`);
lines.push(`- Candidate 侧重点：${safe(candidateProfile.purpose)}`);
lines.push('');

lines.push('## 3. 怎么读这份报告');
lines.push('');
lines.push('1. 先看“先看这里”和“汇总”，判断这次是 gate 没过，还是出现了明显的吞吐/恢复退化。');
lines.push('2. 再看“判定门槛检查”，它直接解释 overall `PASS` / `FAIL` 的原因。');
lines.push('3. 然后看“场景级结果”，判断问题主要落在稳态、拥塞、恢复还是多轨公平性。');
lines.push('4. “逐场景明细”里的白盒字段只在定位原因时再看，不是第一次阅读的入口。');
lines.push('');

lines.push('## 4. 实验信息');
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
if (
  report?.baseline?.commit &&
  report?.candidate?.commit &&
  report.baseline.commit === report.candidate.commit
) {
  lines.push(`| 版本解读 | baseline 与 candidate 为同一提交；本次比较的是运行模式而不是两份不同代码 |`);
}
lines.push('');

lines.push('## 5. 场景在测什么');
lines.push('');
lines.push('| 场景 | 含义 | 重点看什么 |');
lines.push('|---|---|---|');
for (const scenario of scenarios) {
  const guide = AB_SCENARIO_GUIDE[scenario?.id] ?? {
    label: safe(scenario?.name, '-'),
    why: safe(scenario?.notes, '-'),
    focus: '-',
  };
  lines.push(
    `| ${safe(scenario?.id, scenario?.name)} ${guide.label} | ${guide.why} | ${guide.focus} |`
  );
}
if (scenarios.length === 0) {
  lines.push('| - | - | - |');
}
lines.push('');

lines.push('## 6. 汇总');
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

lines.push('## 7. 判定门槛检查');
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

lines.push('## 8. 场景级结果');
lines.push('');
lines.push('| 场景 | 网络形态 | Baseline | Candidate | 变化 | 结论 |');
lines.push('|---|---|---|---|---|---|');
for (const scenario of scenarios) {
  const baselineSummary =
    `gp=${fmtNum(scenario?.baseline?.goodputKbps?.mean)}, loss=${fmtNum(scenario?.baseline?.lossPercent?.mean)}, rec=${fmtInt(scenario?.baseline?.recoveryTimeMs?.mean)}ms`;
  const candidateSummary =
    `gp=${fmtNum(scenario?.candidate?.goodputKbps?.mean)}, loss=${fmtNum(scenario?.candidate?.lossPercent?.mean)}, rec=${fmtInt(scenario?.candidate?.recoveryTimeMs?.mean)}ms`;
  lines.push(
    `| ${safe(scenario?.id, scenario?.name)} | ${formatNetwork(scenario?.network)} | ${baselineSummary} | ${candidateSummary} | ${fmtPercent(scenario?.delta?.goodputPercent)} | \`${scenario?.pass === true ? 'PASS' : scenario?.pass === false ? 'FAIL' : '-'}\` |`
  );
}
if (scenarios.length === 0) {
  lines.push('| - | - | - | - | - | - |');
}
lines.push('');

lines.push('## 9. 逐场景明细');
lines.push('');
for (const scenario of scenarios) {
  lines.push(`### ${safe(scenario?.id, scenario?.name)}`);
  lines.push('');
  lines.push('| 字段 | 内容 |');
  lines.push('|---|---|');
  lines.push(`| 场景名 | ${safe(scenario?.name, '-')} |`);
  lines.push(`| 网络形态 | ${formatNetwork(scenario?.network)} |`);
  lines.push(`| 这场景在看什么 | ${safe(AB_SCENARIO_GUIDE[scenario?.id]?.why, '-')} |`);
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

lines.push('## 10. 产物链接');
lines.push('');
lines.push(`- 原始指标 JSON：\`${safe(report?.artifacts?.rawMetricsPath, '-')}\``);
lines.push(`- 聚合对比 JSON：\`${safe(report?.artifacts?.comparisonReportPath, '-')}\``);
lines.push(`- 日志与 trace：\`${safe(report?.artifacts?.logsPath, '-')}\``);
lines.push('');

fs.mkdirSync(path.dirname(outputPath), { recursive: true });
fs.writeFileSync(outputPath, `${lines.join('\n')}\n`);

console.log(`twcc ab report written to ${outputPath}`);
