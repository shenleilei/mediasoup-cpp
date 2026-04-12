import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import {
  deriveCaseEvaluation,
  extractTiming,
  getImpairedStateForEvaluation,
} from './synthetic_sweep_shared.mjs';
import {
  archiveCurrentReportSet,
  archiveKnownArtifacts,
  defaultCaseReportOutputPath,
  getReportSetPaths,
  getRunTypeForMatrixInput,
  getRunTypeFromSelectedCaseIds,
} from './report_artifacts.mjs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const scenariosPath = path.join(__dirname, 'scenarios', 'sweep_cases.json');
const args = process.argv.slice(2);

function resolvePathArg(value) {
  return path.isAbsolute(value) ? value : path.resolve(repoRoot, value);
}

const inputArg = args.find(arg => arg.startsWith('--input='));
const outputArg = args.find(arg => arg.startsWith('--output='));
const matrixReportPath = inputArg
  ? resolvePathArg(inputArg.slice('--input='.length))
  : getReportSetPaths(repoRoot, 'full').matrixJsonPath;
const outputPath = outputArg
  ? resolvePathArg(outputArg.slice('--output='.length))
  : defaultCaseReportOutputPath(repoRoot, matrixReportPath);

const scenarios = JSON.parse(fs.readFileSync(scenariosPath, 'utf8'));
const matrixReport = JSON.parse(fs.readFileSync(matrixReportPath, 'utf8'));
const resultByCaseId = new Map((matrixReport.cases ?? []).map(entry => [entry.caseId, entry]));
const inferredRunType =
  getRunTypeForMatrixInput(repoRoot, matrixReportPath) ??
  getRunTypeFromSelectedCaseIds(matrixReport.selectedCaseIds);
const inferredReportPaths = getReportSetPaths(repoRoot, inferredRunType);
const updatesKnownLatest =
  path.resolve(matrixReportPath) === inferredReportPaths.matrixJsonPath &&
  path.resolve(outputPath) === inferredReportPaths.caseMarkdownPath;

const baselineCases = scenarios.filter(item => item.group === 'baseline');
const groupOrder = [...new Set(scenarios.map(item => item.group))];

function groupLabel(group) {
  return {
    baseline: 'baseline',
    bw_sweep: 'bw_sweep',
    loss_sweep: 'loss_sweep',
    rtt_sweep: 'rtt_sweep',
    jitter_sweep: 'jitter_sweep',
    transition: 'transition',
    burst: 'burst',
  }[group] ?? group;
}

function fmtMs(value) {
  return typeof value === 'number' ? `${value}ms` : '-';
}

function fmtState(state) {
  if (!state) return '-';
  return `${state.state}/L${state.level}`;
}

function fmtNetem(netem) {
  if (!netem) return '-';
  return `${netem.bandwidth}kbps / RTT ${netem.rtt}ms / loss ${netem.loss}% / jitter ${netem.jitter}ms`;
}

function caseAnchor(caseId) {
  return `#${String(caseId).trim().toLowerCase()}`;
}

function caseLink(caseId) {
  if (!caseId || caseId === '-') {
    return '-';
  }
  return `[${caseId}](${caseAnchor(caseId)})`;
}

function findPrereqCase(result) {
  const scenario = scenarios.find(item => item.caseId === result?.caseId);
  if (scenario?.group === 'baseline') return '-';
  if (!result?.baseline?.netem) return '-';
  const match = baselineCases.find(item =>
    item.bandwidth === result.baseline.netem.bandwidth &&
    item.rtt === result.baseline.netem.rtt &&
    item.loss === result.baseline.netem.loss &&
    item.jitter === result.baseline.netem.jitter
  );
  return match ? match.caseId : '-';
}

function expectedStateText(expect = {}) {
  if (Array.isArray(expect.states) && expect.states.length > 0) {
    return expect.states.join(' / ');
  }
  if (typeof expect.state === 'string') {
    return expect.state;
  }
  return 'stable';
}

function expectedActionText(expect = {}) {
  const stateText = expectedStateText(expect);
  const hasCongested = stateText.includes('congested');
  if (expect.recovery === false) {
    return `允许持续降级，不要求恢复；最高不超过 L${expect.maxLevel ?? 4}`;
  }
  if (hasCongested) {
    return `应触发本地降级动作（以 setEncodingParameters 为主），允许进入 ${stateText}，最高不超过 L${expect.maxLevel ?? 4}`;
  }
  if (stateText === 'stable') {
    return `应保持稳定，动作为 noop 或极轻微保护，最高不超过 L${expect.maxLevel ?? 0}`;
  }
  return `应保持 stable 或轻度降级到 ${stateText}，动作以 noop / setEncodingParameters 为主，最高不超过 L${expect.maxLevel ?? '-'}`;
}

function expectedServerActionText() {
  return '无。matrix 为浏览器 loopback 弱网矩阵，仅验证客户端本地 QoS，不验证服务端 override 下发。';
}

function deriveEvaluation(result, scenario) {
  if (!result || result.error) return null;

  const baselineState = result.phaseSummary?.baseline?.current ?? result.baseline?.state;
  const impairedState = getImpairedStateForEvaluation(
    scenario,
    result.phaseSummary?.impairment
  );
  const recoveredState = result.phaseSummary?.recovery?.best ?? result.recovery?.state;

  return deriveCaseEvaluation(
    scenario,
    baselineState ?? { state: 'stable', level: 0 },
    impairedState ?? { state: 'stable', level: 0 },
    recoveredState ?? baselineState ?? { state: 'stable', level: 0 }
  );
}

function actualQosText(result) {
  if (!result) return '未产出结果';
  if (result.error) return `执行错误：${result.error}`;
  return [
    `baseline(current=${fmtState(result.phaseSummary?.baseline?.current)})`,
    `impairment(评估取 peak=${fmtState(result.phaseSummary?.impairment?.peak)}, current=${fmtState(result.phaseSummary?.impairment?.current)})`,
    `recovery(评估取 best=${fmtState(result.phaseSummary?.recovery?.best)}, current=${fmtState(result.phaseSummary?.recovery?.current)})`,
  ].join('；');
}

function actualResultText(result, scenario) {
  if (!result) return '未产出结果';
  if (result.error) return `ERROR：${result.error}`;
  const evaluation = deriveEvaluation(result, scenario);
  return evaluation?.passed === true
    ? `PASS（${evaluation.analysis?.verdict ?? '符合'}）`
    : `FAIL（${evaluation?.reason ?? 'unknown'}）`;
}

function actualActionText(result) {
  if (!result) return '-';
  if (result.error) return '未完成';
  const actions = Array.from(new Set(result.actionTypes ?? []));
  if (actions.length === 0) return '无非 noop 动作';
  return `${actions.join(', ')}（共 ${result.actionCount ?? actions.length} 次非 noop）`;
}

function actualServerActionText() {
  return '无。matrix runner 未覆盖服务端 automatic override / room pressure / clear 链路。';
}

function keyAnalysisText(result, scenario) {
  if (!result) return '未找到 case 结果。';
  if (result.error) {
    return `执行失败。浏览器/runner 在该 case 中断，错误：${result.error}`;
  }
  const evaluation = deriveEvaluation(result, scenario);
  const verdict = evaluation?.analysis?.verdict ?? '未标记';
  const impaired = fmtState(result.phaseSummary?.impairment?.peak);
  const recovered = fmtState(result.phaseSummary?.recovery?.best);
  if (evaluation?.passed === true) {
    return `判定=${verdict}。重点看 impairment phase 的 peak 和 recovery phase 的 best；本 case 实测为 impaired=${impaired}，recovered=${recovered}。`;
  }
  return `判定=${verdict}。预期=${JSON.stringify(scenario.expect ?? {})}；实际 impairment 评估值=${impaired}，recovery 评估值=${recovered}；失败原因=${evaluation?.reason ?? 'unknown'}`;
}

function timingText(result) {
  const sections = [];
  const phaseConfigs = [
    {
      name: 'impairment',
      trace: result?.phaseSummary?.impairment?.entries,
      startMs: result?.impairment?.startMs,
      endMs: result?.recovery?.startMs,
    },
    {
      name: 'recovery',
      trace: result?.phaseSummary?.recovery?.entries,
      startMs: result?.recovery?.startMs,
    },
  ];
  for (const phase of phaseConfigs) {
    const timing = extractTiming(phase.trace ?? [], phase.startMs, phase.endMs);
    if (!timing || Object.keys(timing).length === 0) continue;
    const parts = [
      `warning=${fmtMs(timing.t_detect_warning)}`,
      `congested=${fmtMs(timing.t_detect_congested)}`,
      `firstAction=${fmtMs(timing.t_first_action)}`,
      `L1=${fmtMs(timing.t_level_1)}`,
      `L2=${fmtMs(timing.t_level_2)}`,
      `L3=${fmtMs(timing.t_level_3)}`,
      `L4=${fmtMs(timing.t_level_4)}`,
      `audioOnly=${fmtMs(timing.t_audio_only)}`,
    ];
    sections.push(`${phase.name}: ${parts.join(', ')}`);
  }
  return sections.length > 0 ? sections.join('；') : '-';
}

function durationText(scenario) {
  return `baseline ${scenario.baselineMs}ms / impairment ${scenario.impairmentMs}ms / recovery ${scenario.recoveryMs}ms`;
}

const casesWithEvaluation = scenarios.map(scenario => {
  const result = resultByCaseId.get(scenario.caseId);
  return {
    scenario,
    result,
    evaluation: deriveEvaluation(result, scenario),
  };
});

const summary = {
  total: scenarios.length,
  executed: casesWithEvaluation.filter(item => item.result && !item.result.error).length,
  passed: casesWithEvaluation.filter(item => item.evaluation?.passed === true).length,
  failed: casesWithEvaluation.filter(
    item => item.result && !item.result.error && item.evaluation?.passed !== true
  ).length,
  errors: casesWithEvaluation.filter(item => item.result?.error).length,
};

const failedCases = casesWithEvaluation.filter(
  item => item.result?.error || (item.result && item.evaluation?.passed !== true)
);
const caseLinksByGroup = new Map();

for (const group of groupOrder) {
  caseLinksByGroup.set(
    group,
    scenarios
      .filter(item => item.group === group)
      .map(item => caseLink(item.caseId))
      .join('、')
  );
}

const lines = [];

lines.push('# 上行 QoS 逐 Case 最终结果');
lines.push('');
lines.push(`生成时间：\`${matrixReport.generatedAt}\``);
lines.push('');
lines.push('## 1. 汇总');
lines.push('');
lines.push(`- 总 Case：\`${summary.total}\``);
lines.push(`- 已执行：\`${summary.executed}\``);
lines.push(`- 通过：\`${summary.passed}\``);
lines.push(`- 失败：\`${summary.failed}\``);
lines.push(`- 错误：\`${summary.errors}\``);
lines.push('');
if (failedCases.length > 0) {
  lines.push('### 1.1 失败 / 错误 Case');
  lines.push('');
  lines.push('| Case ID | 结果 | 说明 |');
  lines.push('|---|---|---|');
  for (const item of failedCases) {
    if (item.result?.error) {
      lines.push(`| ${caseLink(item.scenario.caseId)} | \`ERROR\` | ${item.result.error} |`);
    } else {
      lines.push(`| ${caseLink(item.scenario.caseId)} | \`FAIL\` | ${item.evaluation?.reason ?? 'unknown'} |`);
    }
  }
  lines.push('');
}

lines.push('## 2. 快速跳转');
lines.push('');
lines.push(
  failedCases.length > 0
    ? `- 失败 / 错误：${failedCases.map(item => caseLink(item.scenario.caseId)).join('、')}`
    : '- 失败 / 错误：无'
);
for (const group of groupOrder) {
  lines.push(`- ${groupLabel(group)}：${caseLinksByGroup.get(group)}`);
}
lines.push('');

lines.push('## 3. 逐 Case 结果');
lines.push('');

for (const scenario of scenarios) {
  const result = resultByCaseId.get(scenario.caseId);
  lines.push(`### ${scenario.caseId}`);
  lines.push('');
  lines.push('| 字段 | 内容 |');
  lines.push('|---|---|');
  lines.push(`| Case ID | \`${scenario.caseId}\` |`);
  lines.push(`| 前置 Case | ${caseLink(findPrereqCase(result))} |`);
  lines.push(`| 类型 | \`${scenario.group}\` / priority \`${scenario.priority}\` |`);
  lines.push(`| 上行带宽 | ${result?.impairment?.netem?.bandwidth ?? scenario.bandwidth} kbps |`);
  lines.push(`| RTT | ${result?.impairment?.netem?.rtt ?? scenario.rtt} ms |`);
  lines.push(`| 丢包率 | ${(result?.impairment?.netem?.loss ?? scenario.loss)}% |`);
  lines.push(`| Jitter | ${result?.impairment?.netem?.jitter ?? scenario.jitter} ms |`);
  lines.push(`| 持续时间 | ${durationText(scenario)} |`);
  lines.push(`| 预期 QoS 状态 | ${expectedStateText(scenario.expect ?? {})} |`);
  lines.push(`| 预期动作 | ${expectedActionText(scenario.expect ?? {})} |`);
  lines.push(`| 预期服务端动作 | ${expectedServerActionText()} |`);
  lines.push(`| 实际结果 | ${actualResultText(result, scenario)} |`);
  lines.push(`| 实际 QoS 状态 | ${actualQosText(result)} |`);
  lines.push(`| 实际动作 | ${actualActionText(result)} |`);
  lines.push(`| 实际服务端动作 | ${actualServerActionText()} |`);
  lines.push(`| 重点分析 | ${keyAnalysisText(result, scenario)} |`);
  lines.push(`| 关键时间指标 | ${timingText(result)} |`);
  lines.push('');
}

if (updatesKnownLatest) {
  archiveKnownArtifacts(
    repoRoot,
    [
      {
        path: outputPath,
        relativePath: inferredReportPaths.caseMarkdownRelativePath,
        kind: 'caseMarkdown',
        runType: inferredRunType,
      },
    ],
    {
      runType: inferredRunType,
      sourceScript: 'tests/qos_harness/render_case_report.mjs',
    }
  );
}

fs.mkdirSync(path.dirname(outputPath), { recursive: true });
fs.writeFileSync(outputPath, `${lines.join('\n')}\n`);

if (updatesKnownLatest) {
  archiveCurrentReportSet(repoRoot, {
    generatedAt: matrixReport.generatedAt,
    runType: inferredRunType,
    selectedCaseIds: matrixReport.selectedCaseIds,
    sourceScript: 'tests/qos_harness/render_case_report.mjs',
  });
}
console.log(`case report written to ${outputPath}`);
