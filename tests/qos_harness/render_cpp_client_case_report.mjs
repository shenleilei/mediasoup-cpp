import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import {
  deriveCaseEvaluation,
  extractTiming,
  getCaseExpectation,
  getPhaseNetwork,
  getImpairedStateForEvaluation,
} from './synthetic_sweep_shared.mjs';
import {
  getCppClientReportSetPaths,
  inferCppClientRunType,
} from './cpp_client_report_artifacts.mjs';
import {
  filterScenarioCatalog,
  loadScenarioCatalog,
} from './scenario_catalog.mjs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const args = process.argv.slice(2);

function resolvePathArg(value) {
  return path.isAbsolute(value) ? value : path.resolve(repoRoot, value);
}

const inputArg = args.find(arg => arg.startsWith('--input='));
const outputArg = args.find(arg => arg.startsWith('--output='));
const matrixReportPath = inputArg
  ? resolvePathArg(inputArg.slice('--input='.length))
  : getCppClientReportSetPaths(repoRoot, 'full').matrixJsonPath;
const runType = inferCppClientRunType(repoRoot, matrixReportPath) ?? 'full';
const defaultOutput = getCppClientReportSetPaths(repoRoot, runType).caseMarkdownPath;
const outputPath = outputArg ? resolvePathArg(outputArg.slice('--output='.length)) : defaultOutput;

const matrixReport = JSON.parse(fs.readFileSync(matrixReportPath, 'utf8'));
const allScenarios = loadScenarioCatalog();
const scenarios = Array.isArray(matrixReport.includedCaseIds)
  ? allScenarios.filter(scenario => matrixReport.includedCaseIds.includes(scenario.caseId))
  : filterScenarioCatalog(allScenarios, {
      selectedCaseIds: Array.isArray(matrixReport.selectedCaseIds)
        ? new Set(matrixReport.selectedCaseIds)
        : null,
      includeExtended: matrixReport.includeExtended === true,
    });
const resultByCaseId = new Map((matrixReport.cases ?? []).map(entry => [entry.caseId, entry]));
const groupOrder = [...new Set(scenarios.map(item => item.group))];

function caseAnchor(caseId) {
  return `#${String(caseId).trim().toLowerCase()}`;
}

function caseLink(caseId) {
  if (!caseId || caseId === '-') {
    return '-';
  }
  return `[${caseId}](${caseAnchor(caseId)})`;
}

function fmtState(state) {
  if (!state) return '-';
  return `${state.state}/L${state.level}`;
}

function fmtNetem(netem) {
  if (!netem) return '-';
  return `${netem.bandwidth}kbps / RTT ${netem.rtt}ms / loss ${netem.loss}% / jitter ${netem.jitter}ms`;
}

function fmtMs(value) {
  return typeof value === 'number' ? `${value}ms` : '-';
}

function expectedStateText(expectation = {}) {
  if (Array.isArray(expectation.states) && expectation.states.length > 0) {
    return expectation.states.join(' / ');
  }
  if (typeof expectation.state === 'string') {
    return expectation.state;
  }
  return 'stable';
}

function expectedActionText(expectation = {}) {
  const stateText = expectedStateText(expectation);
  const parts = [
    `期望状态=${stateText}`,
  ];
  if (typeof expectation.minLevel === 'number') {
    parts.push(`minLevel=${expectation.minLevel}`);
  }
  if (typeof expectation.maxLevel === 'number') {
    parts.push(`maxLevel=${expectation.maxLevel}`);
  }
  if (expectation.recovery === false) {
    parts.push('recovery=disabled');
  }
  return parts.join('；');
}

function actualQosText(result) {
  if (!result) return '未产出结果';
  if (result.error) return `ERROR：${result.error}`;
  return [
    `baseline(current=${fmtState(result.phaseSummary?.baseline?.current)})`,
    `impairment(peak=${fmtState(result.phaseSummary?.impairment?.peak)}, current=${fmtState(result.phaseSummary?.impairment?.current)})`,
    `recovery(best=${fmtState(result.phaseSummary?.recovery?.best)}, current=${fmtState(result.phaseSummary?.recovery?.current)})`,
  ].join('；');
}

function actualResultText(result, scenario) {
  if (!result) return '未产出结果';
  if (result.error) return `ERROR：${result.error}`;

  const baselineState = result.phaseSummary?.baseline?.current ?? result.baseline?.state;
  const impairedState = getImpairedStateForEvaluation(
    scenario,
    result.phaseSummary?.impairment,
    result.phaseSummary?.baseline
  );
  const recoveredState = result.phaseSummary?.recovery?.best ?? result.recovery?.state;
  const evaluation = deriveCaseEvaluation(
    scenario,
    baselineState,
    impairedState,
    recoveredState,
    'cpp_client'
  );
  return evaluation.passed
    ? `PASS（${evaluation.analysis?.verdict ?? '符合'}）`
    : `FAIL（${evaluation.reason}）`;
}

function actualActionText(result) {
  if (!result) return '-';
  if (result.error) return '未完成';
  const actions = Array.from(new Set(result.actionTypes ?? []));
  if (actions.length === 0) return '无非 noop 动作';
  return `${actions.join(', ')}（共 ${result.actionCount ?? actions.length} 次非 noop）`;
}

function fmtTimingBlock(timing = {}) {
  const parts = Object.entries(timing)
    .filter(([, value]) => typeof value === 'number')
    .map(([key, value]) => `${key}=${fmtMs(value)}`);
  return parts.length > 0 ? parts.join('；') : '-';
}

function diagnosticsText(result) {
  if (!result?.diagnostics) return '-';
  const parts = [];
  if (result.diagnostics.clientStdoutTail) parts.push(`clientStdoutTail:\n\`\`\`\n${result.diagnostics.clientStdoutTail}\n\`\`\``);
  if (result.diagnostics.clientStderrTail) parts.push(`clientStderrTail:\n\`\`\`\n${result.diagnostics.clientStderrTail}\n\`\`\``);
  if (result.diagnostics.sfuStderrTail) parts.push(`sfuStderrTail:\n\`\`\`\n${result.diagnostics.sfuStderrTail}\n\`\`\``);
  return parts.length > 0 ? parts.join('\n\n') : '-';
}

const lines = [];
const summary = matrixReport.summary ?? {};
const failedCases = scenarios.filter(scenario => {
  const result = resultByCaseId.get(scenario.caseId);
  return result?.error || (result && !result.verdict?.passed);
});

lines.push('# PlainTransport C++ Client QoS Matrix 逐 Case 结果');
lines.push('');
lines.push(`生成时间：\`${matrixReport.generatedAt}\``);
lines.push('');
lines.push('## 1. 汇总');
lines.push('');
lines.push(`- 总 Case：\`${summary.total ?? scenarios.length}\``);
lines.push(`- 已执行：\`${summary.executed ?? 0}\``);
lines.push(`- 通过：\`${summary.passed ?? 0}\``);
lines.push(`- 失败：\`${summary.failed ?? 0}\``);
lines.push(`- 错误：\`${summary.errors ?? 0}\``);
lines.push(`- runner：\`${matrixReport.runner ?? 'cpp_client'}\``);
lines.push('');

if (failedCases.length > 0) {
  lines.push('### 1.1 失败 / 错误 Case');
  lines.push('');
  lines.push('| Case ID | 结果 | 说明 |');
  lines.push('|---|---|---|');
  for (const scenario of failedCases) {
    const result = resultByCaseId.get(scenario.caseId);
    if (result?.error) {
      lines.push(`| ${caseLink(scenario.caseId)} | \`ERROR\` | ${result.error} |`);
    } else {
      lines.push(`| ${caseLink(scenario.caseId)} | \`FAIL\` | ${result?.verdict?.reason ?? 'unknown'} |`);
    }
  }
  lines.push('');
}

lines.push('## 2. 快速跳转');
lines.push('');
lines.push(
  failedCases.length > 0
    ? `- 失败 / 错误：${failedCases.map(scenario => caseLink(scenario.caseId)).join('、')}`
    : '- 失败 / 错误：无'
);
for (const group of groupOrder) {
  const ids = scenarios
    .filter(scenario => scenario.group === group)
    .map(scenario => caseLink(scenario.caseId))
    .join('、');
  lines.push(`- ${group}：${ids}`);
}
lines.push('');

lines.push('## 3. 逐 Case 结果');
lines.push('');

for (const scenario of scenarios) {
  const result = resultByCaseId.get(scenario.caseId);
  const expectation = getCaseExpectation(scenario, 'cpp_client');
  const baselineNetem = result?.baseline?.netem ?? getPhaseNetwork(scenario, 'baseline');
  const impairmentNetem = result?.impairment?.netem ?? getPhaseNetwork(scenario, 'impaired');
  const recoveryNetem = result?.recovery?.netem ?? getPhaseNetwork(scenario, 'recovery');

  lines.push(`### ${scenario.caseId}`);
  lines.push('');
  lines.push('| 字段 | 内容 |');
  lines.push('|---|---|');
  lines.push(`| Case ID | \`${scenario.caseId}\` |`);
  lines.push(`| 类型 | \`${scenario.group}\` / priority \`${scenario.priority}\` |`);
  lines.push(`| baseline 网络 | ${fmtNetem(baselineNetem)} |`);
  lines.push(`| impairment 网络 | ${fmtNetem(impairmentNetem)} |`);
  lines.push(`| recovery 网络 | ${fmtNetem(recoveryNetem)} |`);
  lines.push(`| 预期 QoS | ${expectedActionText(expectation)} |`);
  lines.push(`| 实际 QoS | ${actualQosText(result)} |`);
  lines.push(`| 结果 | ${actualResultText(result, scenario)} |`);
  lines.push(`| 动作摘要 | ${actualActionText(result)} |`);
  lines.push(`| impairment timing | ${fmtTimingBlock(result?.timing?.impairment)} |`);
  lines.push(`| recovery timing | ${fmtTimingBlock(result?.timing?.recovery)} |`);
  lines.push(`| 诊断 | ${result?.error ? diagnosticsText(result) : '-'} |`);

  if (result?.trace?.length) {
    const impairmentStartMs = result?.impairment?.startMs;
    const recoveryStartMs = result?.recovery?.startMs;
    const recoveryEndMs = result?.recovery?.endMs;
    const impairmentTiming = extractTiming(result.trace, impairmentStartMs, recoveryStartMs);
    const recoveryTiming =
      expectation?.recovery === false
        ? {}
        : extractTiming(result.trace, recoveryStartMs, recoveryEndMs);
    lines.push(`| raw impairment timing | ${fmtTimingBlock(impairmentTiming)} |`);
    lines.push(`| raw recovery timing | ${fmtTimingBlock(recoveryTiming)} |`);
  }

  lines.push('');
}

fs.mkdirSync(path.dirname(outputPath), { recursive: true });
fs.writeFileSync(outputPath, `${lines.join('\n')}\n`);
