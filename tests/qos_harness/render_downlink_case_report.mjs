/**
 * Render downlink QoS case report from matrix JSON.
 *
 * Usage:
 *   node tests/qos_harness/render_downlink_case_report.mjs
 *   node tests/qos_harness/render_downlink_case_report.mjs --input=path/to/report.json --output=path/to/output.md
 */
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const args = process.argv.slice(2);

const DOWNLINK_PATHS = {
  archiveRoot: 'docs/archive/downlink-qos-runs',
  fullMatrixJson: 'docs/generated/downlink-qos-matrix-report.json',
  fullCaseMarkdown: 'docs/downlink-qos-case-results.md',
  targetedMatrixJson: 'docs/generated/downlink-qos-matrix-report.targeted.json',
  targetedCaseMarkdown: 'docs/generated/downlink-qos-case-results.targeted.md',
};

function resolveArg(value) {
  return path.isAbsolute(value) ? value : path.resolve(repoRoot, value);
}

const inputArg = args.find(a => a.startsWith('--input='));
const outputArg = args.find(a => a.startsWith('--output='));

function inferRunType(inputPath) {
  const resolved = path.resolve(inputPath);
  if (resolved === path.join(repoRoot, DOWNLINK_PATHS.targetedMatrixJson)) return 'targeted';
  return 'full';
}

const matrixReportPath = inputArg
  ? resolveArg(inputArg.slice('--input='.length))
  : path.join(repoRoot, DOWNLINK_PATHS.fullMatrixJson);

const runType = inferRunType(matrixReportPath);
const defaultOutput = runType === 'targeted'
  ? path.join(repoRoot, DOWNLINK_PATHS.targetedCaseMarkdown)
  : path.join(repoRoot, DOWNLINK_PATHS.fullCaseMarkdown);
const outputPath = outputArg ? resolveArg(outputArg.slice('--output='.length)) : defaultOutput;

const matrixReport = JSON.parse(fs.readFileSync(matrixReportPath, 'utf8'));
const caseDefs = JSON.parse(
  fs.readFileSync(path.join(__dirname, 'scenarios', 'downlink_cases.json'), 'utf8'),
);
const caseDefById = new Map(caseDefs.map(c => [c.caseId, c]));
const resultById = new Map((matrixReport.cases ?? []).map(c => [c.caseId, c]));
const includedIds = matrixReport.includedCaseIds ?? caseDefs.map(c => c.caseId);
const scenarios = includedIds.map(id => caseDefById.get(id)).filter(Boolean);
const groupOrder = [...new Set(scenarios.map(s => s.group))];

function caseAnchor(id) { return `#${String(id).trim().toLowerCase()}`; }
function caseLink(id) { return id ? `[${id}](${caseAnchor(id)})` : '-'; }

function fmtNetem(net) {
  if (!net) return '-';
  return `${net.bandwidth ?? '-'}kbps / RTT ${net.rtt ?? '-'}ms / loss ${net.loss ?? 0}% / jitter ${net.jitter ?? 0}ms`;
}

function verdictText(result) {
  if (!result) return '未产出结果';
  if (result.error) return `ERROR：${result.error}`;
  return result.verdict?.passed ? `PASS（${result.verdict.reason}）` : `FAIL（${result.verdict?.reason ?? 'unknown'}）`;
}

function expectText(expect) {
  if (!expect) return '-';
  const parts = [];
  if (expect.pauseUpstream) parts.push('pauseUpstream=true');
  if (expect.resumeUpstream) parts.push('resumeUpstream=true');
  if (expect.consumerPaused === false) parts.push('consumerPaused=false');
  if (typeof expect.preferredSpatialLayer === 'number') parts.push(`preferredSpatialLayer=${expect.preferredSpatialLayer}`);
  if (typeof expect.maxSpatialLayer === 'number') parts.push(`maxSpatialLayer≤${expect.maxSpatialLayer}`);
  if (expect.highPriorityBetterLayer) parts.push('highPriority gets better layer');
  if (expect.recovers) parts.push('recovers after impairment');
  return parts.join('；') || '-';
}

// Build report
const lines = [];
const summary = matrixReport.summary ?? {};
const failedCases = scenarios.filter(s => {
  const r = resultById.get(s.caseId);
  return r?.error || (r && !r.verdict?.passed);
});

lines.push('# 下行 QoS 逐 Case 最终结果');
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
lines.push('');

if (failedCases.length > 0) {
  lines.push('### 1.1 失败 / 错误 Case');
  lines.push('');
  lines.push('| Case ID | 结果 | 说明 |');
  lines.push('|---|---|---|');
  for (const s of failedCases) {
    const r = resultById.get(s.caseId);
    if (r?.error) {
      lines.push(`| ${caseLink(s.caseId)} | \`ERROR\` | ${r.error} |`);
    } else {
      lines.push(`| ${caseLink(s.caseId)} | \`FAIL\` | ${r?.verdict?.reason ?? 'unknown'} |`);
    }
  }
  lines.push('');
}

lines.push('## 2. 快速跳转');
lines.push('');
lines.push(
  failedCases.length > 0
    ? `- 失败 / 错误：${failedCases.map(s => caseLink(s.caseId)).join('、')}`
    : '- 失败 / 错误：无',
);
for (const group of groupOrder) {
  const ids = scenarios.filter(s => s.group === group).map(s => caseLink(s.caseId)).join('、');
  lines.push(`- ${group}：${ids}`);
}
lines.push('');

lines.push('## 3. 逐 Case 结果');
lines.push('');

for (const scenario of scenarios) {
  const result = resultById.get(scenario.caseId);
  const phases = scenario.phases || {};
  lines.push(`### ${scenario.caseId}`);
  lines.push('');
  lines.push('| 字段 | 内容 |');
  lines.push('|---|---|');
  lines.push(`| Case ID | \`${scenario.caseId}\` |`);
  lines.push(`| 类型 | \`${scenario.group}\` / priority \`${scenario.priority}\` |`);
  lines.push(`| 说明 | ${scenario.description ?? '-'} |`);
  lines.push(`| baseline 网络 | ${fmtNetem(phases.baseline)} |`);
  lines.push(`| impairment 网络 | ${fmtNetem(phases.impaired)} |`);
  lines.push(`| recovery 网络 | ${fmtNetem(phases.recovery)} |`);
  lines.push(`| 持续时间 | baseline ${scenario.baselineMs}ms / impairment ${scenario.impairmentMs}ms / recovery ${scenario.recoveryMs}ms |`);
  lines.push(`| 预期 | ${expectText(scenario.expect)} |`);
  lines.push(`| 实际结果 | ${verdictText(result)} |`);
  lines.push('');
}

fs.mkdirSync(path.dirname(outputPath), { recursive: true });
fs.writeFileSync(outputPath, `${lines.join('\n')}\n`);

// Archive
const archiveRoot = path.join(repoRoot, DOWNLINK_PATHS.archiveRoot);
const ts = matrixReport.generatedAt.replace(/:/g, '-');
const archiveDir = path.join(archiveRoot, ts);
if (fs.existsSync(archiveDir)) {
  const relOut = path.relative(repoRoot, outputPath);
  const archiveDest = path.join(archiveDir, relOut);
  fs.mkdirSync(path.dirname(archiveDest), { recursive: true });
  fs.copyFileSync(outputPath, archiveDest);
}

console.log(`downlink case report written to ${outputPath}`);
