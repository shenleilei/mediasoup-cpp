import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

import { createCppClientHarness, ensureHarnessMp4, sleep } from './cpp_client_runner.mjs';
import { loadScenarioCatalog } from './scenario_catalog.mjs';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..', '..');
const changeDir = path.join(repoRoot, 'changes', '2026-04-21-plain-client-sender-transport-control');
const artifactRoot = path.join(changeDir, 'artifacts', 'twcc-ab-eval');

const args = process.argv.slice(2);
const repetitionsArg = args.find(arg => arg.startsWith('--repetitions='));
const matrixSpeedArg = args.find(arg => arg.startsWith('--matrix-speed='));
const casesArg = args.find(arg => arg.startsWith('--cases='));

const repetitions = repetitionsArg ? Number(repetitionsArg.split('=')[1]) : 3;
const matrixSpeed = matrixSpeedArg ? Number(matrixSpeedArg.split('=')[1]) : 0.1;
const selectedAbIds = casesArg
  ? casesArg.split('=')[1].split(',').map(item => item.trim()).filter(Boolean)
  : ['AB-001', 'AB-002', 'AB-003', 'AB-004', 'AB-005'];

const workerBin = process.env.QOS_CPP_CLIENT_WORKER_BIN || './mediasoup-worker';
const generatedAt = new Date().toISOString();
const runId = generatedAt.replace(/:/g, '-');
const runDir = path.join(artifactRoot, runId);
const rawDir = path.join(runDir, 'raw');
fs.mkdirSync(rawDir, { recursive: true });

const scenarioCatalog = loadScenarioCatalog();
const caseById = new Map(scenarioCatalog.map(caseDef => [caseDef.caseId, caseDef]));
const abScenarioConfig = {
  'AB-001': { caseId: 'B1', phase: 'baseline', name: 'steady_state' },
  'AB-002': { caseId: 'T3', phase: 'impairment', name: 'step_down_congestion' },
  'AB-003': { caseId: 'T3', phase: 'recovery', name: 'step_up_recovery' },
  'AB-004': { caseId: 'R4', phase: 'impairment', name: 'high_rtt_stability' },
};

const groups = [
  {
    id: 'G0',
    label: 'G0 Legacy Baseline',
    env: {
      PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER: '0',
    },
  },
  {
    id: 'G1',
    label: 'G1 Controller-Only Baseline',
    env: {
      PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER: '1',
      PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE: '0',
    },
  },
  {
    id: 'G2',
    label: 'G2 Candidate',
    env: {
      PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER: '1',
      PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE: '1',
    },
  },
];

function runCommand(cmd, commandArgs, envOverrides = {}) {
  return execFileSync(cmd, commandArgs, {
    cwd: repoRoot,
    env: {
      ...process.env,
      ...envOverrides,
    },
    stdio: 'pipe',
    encoding: 'utf8',
  });
}

function runCommandAllowFailure(cmd, commandArgs, envOverrides = {}) {
  try {
    return {
      status: 0,
      stdout: runCommand(cmd, commandArgs, envOverrides),
      stderr: '',
    };
  } catch (error) {
    return {
      status: typeof error.status === 'number' ? error.status : 1,
      stdout: error.stdout ?? '',
      stderr: error.stderr ?? error.message,
    };
  }
}

function percentile(values, ratio) {
  const valid = values.filter(value => Number.isFinite(value)).sort((a, b) => a - b);
  if (valid.length === 0) return null;
  const index = Math.min(valid.length - 1, Math.max(0, Math.round((valid.length - 1) * ratio)));
  return valid[index];
}

function summarizeValues(values) {
  const valid = values.filter(value => Number.isFinite(value));
  if (valid.length === 0) {
    return { mean: null, p50: null, p90: null };
  }
  const mean = valid.reduce((sum, value) => sum + value, 0) / valid.length;
  return {
    mean,
    p50: percentile(valid, 0.5),
    p90: percentile(valid, 0.9),
  };
}

function toPercentDelta(baselineValue, candidateValue) {
  if (!Number.isFinite(baselineValue) || !Number.isFinite(candidateValue) || baselineValue === 0) {
    return null;
  }
  return ((candidateValue - baselineValue) / baselineValue) * 100;
}

function scaleDuration(ms) {
  return Math.max(1000, Math.round(ms * matrixSpeed));
}

function phaseWindow(result, phaseName) {
  const phase = result?.[phaseName];
  if (!phase) return { startMs: null, endMs: null };
  return { startMs: phase.startMs, endMs: phase.endMs };
}

function phaseSamples(result, phaseName) {
  const { startMs, endMs } = phaseWindow(result, phaseName);
  const samples = Array.isArray(result?.samples) ? result.samples : [];
  return samples.filter(sample => {
    if (typeof sample?.tsMs !== 'number') return false;
    if (typeof startMs === 'number' && sample.tsMs <= startMs) return false;
    if (typeof endMs === 'number' && sample.tsMs > endMs) return false;
    return true;
  });
}

function phaseGoodputStats(result, phaseName) {
  const samples = phaseSamples(result, phaseName);
  return summarizeValues(samples.map(sample => (Number.isFinite(sample.sendBps) ? sample.sendBps / 1000 : null)));
}

function phaseLossStats(result, phaseName) {
  const samples = phaseSamples(result, phaseName);
  return summarizeValues(samples.map(sample => (Number.isFinite(sample.lossRate) ? sample.lossRate * 100 : null)));
}

function phaseStability(result, phaseName) {
  const samples = phaseSamples(result, phaseName)
    .map(sample => (Number.isFinite(sample.sendBps) ? sample.sendBps / 1000 : null))
    .filter(value => Number.isFinite(value));
  const p50 = percentile(samples, 0.5);
  const p90 = percentile(samples, 0.9);
  if (!Number.isFinite(p50) || !Number.isFinite(p90)) return null;
  return p90 - p50;
}

function recoveryTimingValue(result) {
  const stable = result?.timing?.recovery?.t_detect_stable;
  if (Number.isFinite(stable)) return stable;
  const recovering = result?.timing?.recovery?.t_detect_recovering;
  if (Number.isFinite(recovering)) return recovering;
  return result?.recovery?.durationMs ?? null;
}

function buildAbScenarioMetrics(result, abId) {
  const config = abScenarioConfig[abId];
  if (!config) return null;
  return {
    id: abId,
    sourceCaseId: config.caseId,
    phase: config.phase,
    goodputKbps: phaseGoodputStats(result, config.phase),
    lossPercent: phaseLossStats(result, config.phase),
    recoveryTimeMs: summarizeValues([abId === 'AB-003' ? recoveryTimingValue(result) : null]),
    stabilityKbpsP90MinusP50: phaseStability(result, config.phase),
  };
}

function buildFairnessDeviation(samplesByTrack, weights, trackIds) {
  const expectedShares = weights.map(weight => weight / weights.reduce((sum, value) => sum + value, 0));
  const alignedLength = Math.min(...trackIds.map(trackId => (samplesByTrack[trackId] ?? []).length));
  const startIndex = Math.max(0, alignedLength - 10);
  const averages = trackIds.map(trackId => {
    const samples = (samplesByTrack[trackId] ?? []).slice(startIndex, alignedLength);
    const valid = samples
      .map(sample => sample?.bitrateBps)
      .filter(value => Number.isFinite(value));
    if (valid.length === 0) return 0;
    return valid.reduce((sum, value) => sum + value, 0) / valid.length;
  });
  const totalAverage = averages.reduce((sum, value) => sum + value, 0);
  if (totalAverage <= 0) return null;
  const observedShares = averages.map(value => value / totalAverage);
  return observedShares.reduce((sum, observedShare, index) => {
    return sum + Math.abs(observedShare - expectedShares[index]);
  }, 0) / 2;
}

async function runFairnessScenario(group, repetition) {
  const scenarioPath = path.join(__dirname, 'scenarios', 'multi_video_budget.json');
  const scenario = JSON.parse(fs.readFileSync(scenarioPath, 'utf8'));
  const scaledProfile = {
    warmupMs: scaleDuration(scenario.matrixProfile.warmupMs),
    phases: scenario.matrixProfile.phases.map(phase => ({
      ...phase,
      durationMs: scaleDuration(phase.durationMs),
    })),
  };
  const totalDurationMs =
    scaledProfile.warmupMs +
    scaledProfile.phases.reduce((sum, phase) => sum + phase.durationMs, 0) +
    1500;
  const signalingPort = 15150 + groups.findIndex(item => item.id === group.id) * 20 + repetition;
  const roomId = `${scenario.roomId}_${group.id.toLowerCase()}_${repetition}`;
  const peerId = `${scenario.peerId}_${group.id.toLowerCase()}_${repetition}`;
  const videoSources = Array.from({ length: scenario.trackCount }, () => ensureHarnessMp4()).join(',');

  const harness = await createCppClientHarness({
    signalingPort,
    roomId,
    peerId,
    mp4Path: ensureHarnessMp4(),
    extraEnv: {
      PLAIN_CLIENT_THREADED: '1',
      PLAIN_CLIENT_VIDEO_TRACK_COUNT: String(scenario.trackCount),
      PLAIN_CLIENT_VIDEO_TRACK_WEIGHTS: scenario.weights.join(','),
      PLAIN_CLIENT_VIDEO_SOURCES: videoSources,
      QOS_TEST_MATRIX_PROFILE: JSON.stringify(scaledProfile),
      QOS_TEST_MATRIX_LOCAL_ONLY: '1',
      ...group.env,
    },
  });

  try {
    await sleep(totalDurationMs);
    const trace = harness.getTrace();
    const deviation = buildFairnessDeviation(
      trace.samplesByTrack ?? {},
      scenario.weights,
      scenario.expect.trackIds,
    );
    return {
      fairnessDeviation: deviation,
      tracePath: null,
    };
  } finally {
    await harness.stop();
  }
}

function writeJson(filePath, value) {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
  fs.writeFileSync(filePath, `${JSON.stringify(value, null, 2)}\n`);
}

function runHardRegressionSuite() {
  const result = {
    commands: [],
    pass: true,
  };
  const commands = [
    {
      label: 'qos-unit',
      cmd: './build/mediasoup_qos_unit_tests',
      args: [
        "--gtest_filter=TransportCcHelpers.*:SenderTransportControllerTest.BitrateAllocation*:SenderTransportControllerTest.PauseDropsQueuedRetransmissionsForTrack:SenderTransportControllerTest.FlushForShutdownDrainsQueuedVideoBeforeDiscardingRemainder:SenderTransportControllerTest.EffectivePacingBitrateUsesMinOfTargetEstimateAndCap",
      ],
      env: {},
    },
    {
      label: 'threaded-integration',
      cmd: './build/mediasoup_thread_integration_tests',
      args: [
        "--gtest_filter=PlainTransportDirect.*:NetworkThreadIntegration.TransportCcFeedback*:NetworkThreadIntegration.TransportEstimateRaisesWhenAggregateTargetIncreases:NetworkPause.PauseAckRequiresQuiescedTransport:NetworkPause.PauseAckDropsQueuedRetransmissionsAndPreventsLateRtp:ThreadedPlainPublishIntegrationTest.RealWorkerTWCCFeedbackObservedByPlainSender",
      ],
      env: {
        QOS_THREAD_WORKER_BIN: workerBin,
      },
    },
  ];

  for (const command of commands) {
    try {
      runCommand(command.cmd, command.args, command.env);
      result.commands.push({ label: command.label, pass: true });
    } catch (error) {
      result.pass = false;
      result.commands.push({
        label: command.label,
        pass: false,
        error: error.stdout || error.message,
      });
    }
  }

  return result;
}

async function main() {
  const selectedConfigs = selectedAbIds.filter(id => abScenarioConfig[id]);
  const matrixCaseIds = [...new Set(selectedConfigs.map(id => abScenarioConfig[id].caseId))];
  const raw = {
    schema: 'mediasoup.twcc.abc.raw.v1',
    generatedAt,
    repetitions,
    matrixSpeed,
    selectedAbIds,
    matrixCaseIds,
    workerBin,
    environment: {
      host: os.hostname(),
      os: `${os.platform()} ${os.release()}`,
      cpu: os.cpus()[0]?.model ?? 'unknown',
      memoryGb: Math.round((os.totalmem() / (1024 ** 3)) * 10) / 10,
      commit: runCommand('git', ['rev-parse', 'HEAD']).trim(),
    },
    hardRegression: runHardRegressionSuite(),
    groups: {},
  };

  for (const group of groups) {
    const groupResults = {
      label: group.label,
      matrixRepetitions: [],
      fairnessRepetitions: [],
    };

    for (let repetition = 1; repetition <= repetitions; repetition += 1) {
      const signalingPort = 15100 + groups.findIndex(item => item.id === group.id) * 20 + repetition;
      const matrixRun = runCommandAllowFailure(
        'node',
        [
          path.join(repoRoot, 'tests', 'qos_harness', 'run_cpp_client_matrix.mjs'),
          `--cases=${matrixCaseIds.join(',')}`,
        ],
        {
          QOS_MATRIX_SPEED: String(matrixSpeed),
          QOS_CPP_CLIENT_MATRIX_PORT: String(signalingPort),
          QOS_CPP_CLIENT_WORKER_BIN: workerBin,
          ...group.env,
        },
      );

      const reportPath = path.join(repoRoot, 'docs', 'generated', 'uplink-qos-cpp-client-matrix-report.targeted.json');
      const report = JSON.parse(fs.readFileSync(reportPath, 'utf8'));
      report.exitStatus = matrixRun.status;
      report.stderr = matrixRun.stderr;
      writeJson(path.join(rawDir, `${group.id.toLowerCase()}-matrix-rep${repetition}.json`), report);
      groupResults.matrixRepetitions.push(report);
    }

    if (selectedAbIds.includes('AB-005')) {
      for (let repetition = 1; repetition <= repetitions; repetition += 1) {
        const fairness = await runFairnessScenario(group, repetition);
        groupResults.fairnessRepetitions.push(fairness);
      }
    }

    raw.groups[group.id] = groupResults;
  }

  writeJson(path.join(runDir, 'raw-groups.json'), raw);

  const pairings = [
    { baseline: 'G1', candidate: 'G2', name: 'g1-vs-g2', baselineLabel: 'G1 Controller-Only', candidateLabel: 'G2 Candidate' },
    { baseline: 'G0', candidate: 'G2', name: 'g0-vs-g2', baselineLabel: 'G0 Legacy', candidateLabel: 'G2 Candidate' },
  ];

  for (const pairing of pairings) {
    const baselineGroup = raw.groups[pairing.baseline];
    const candidateGroup = raw.groups[pairing.candidate];
    const metrics = {
      schema: 'mediasoup.twcc.ab.v1',
      generatedAt,
      baseline: { label: pairing.baselineLabel, commit: raw.environment.commit },
      candidate: { label: pairing.candidateLabel, commit: raw.environment.commit },
      environment: {
        host: raw.environment.host,
        os: os.platform(),
        kernel: os.release(),
        cpu: raw.environment.cpu,
        memoryGb: raw.environment.memoryGb,
        networkSetup: `tc/netem via cpp-client-matrix speed=${matrixSpeed}`,
        inputMedia: 'tests/fixtures/media/test_sweep_cpp_matrix.mp4 + threaded multi_video_budget',
      },
      runConfig: {
        repetitionsPerScenario: repetitions,
        scenarios: selectedAbIds,
      },
      scenarios: [],
      gates: [],
      overall: { pass: true, summary: '' },
      artifacts: {
        rawMetricsPath: path.relative(repoRoot, path.join(runDir, 'raw-groups.json')),
        comparisonReportPath: path.relative(repoRoot, path.join(runDir, `${pairing.name}.json`)),
        logsPath: path.relative(repoRoot, rawDir),
      },
    };

    for (const abId of selectedAbIds) {
      if (abId === 'AB-005') {
        const baselineDeviation = summarizeValues(
          baselineGroup.fairnessRepetitions.map(item => item.fairnessDeviation),
        );
        const candidateDeviation = summarizeValues(
          candidateGroup.fairnessRepetitions.map(item => item.fairnessDeviation),
        );
        metrics.scenarios.push({
          id: abId,
          name: 'multi_video_budget',
          network: {
            bandwidthKbps: 300,
            rttMs: 260,
            lossRate: 0.08,
            jitterMs: 24,
          },
          baseline: {
            goodputKbps: { mean: null, p50: null, p90: null },
            lossPercent: { mean: null, p50: null, p90: null },
            recoveryTimeMs: { mean: null, p50: null, p90: null },
            stabilityKbpsP90MinusP50: null,
            queuePressure: {
              wouldBlockTotal: null,
              queuedVideoRetentions: null,
              audioDeadlineDrops: null,
              retransmissionDrops: null,
            },
            fairnessDeviation: baselineDeviation.mean,
          },
          candidate: {
            goodputKbps: { mean: null, p50: null, p90: null },
            lossPercent: { mean: null, p50: null, p90: null },
            recoveryTimeMs: { mean: null, p50: null, p90: null },
            stabilityKbpsP90MinusP50: null,
            queuePressure: {
              wouldBlockTotal: null,
              queuedVideoRetentions: null,
              audioDeadlineDrops: null,
              retransmissionDrops: null,
            },
            fairnessDeviation: candidateDeviation.mean,
          },
          delta: {
            goodputPercent: null,
            lossPercent: null,
            recoveryTimePercent: null,
            queuePressurePercent: null,
            fairnessDeviationPercent: toPercentDelta(baselineDeviation.mean, candidateDeviation.mean),
          },
          pass:
            Number.isFinite(baselineDeviation.mean) &&
            Number.isFinite(candidateDeviation.mean) &&
            candidateDeviation.mean <= baselineDeviation.mean,
          notes: 'Fairness derived from weighted multi-track trace shares over the final 10 samples.',
        });
        continue;
      }

      const config = abScenarioConfig[abId];
      const baselineMetrics = [];
      const candidateMetrics = [];
      for (const report of baselineGroup.matrixRepetitions) {
        const result = report.cases.find(item => item.caseId === config.caseId);
        if (result) baselineMetrics.push(buildAbScenarioMetrics(result, abId));
      }
      for (const report of candidateGroup.matrixRepetitions) {
        const result = report.cases.find(item => item.caseId === config.caseId);
        if (result) candidateMetrics.push(buildAbScenarioMetrics(result, abId));
      }

      const aggregateMetric = (items, accessor) => summarizeValues(items.map(item => accessor(item)).filter(value => Number.isFinite(value)));
      const baselineGoodput = aggregateMetric(baselineMetrics, item => item?.goodputKbps?.mean);
      const candidateGoodput = aggregateMetric(candidateMetrics, item => item?.goodputKbps?.mean);
      const baselineLoss = aggregateMetric(baselineMetrics, item => item?.lossPercent?.mean);
      const candidateLoss = aggregateMetric(candidateMetrics, item => item?.lossPercent?.mean);
      const baselineRecovery = aggregateMetric(baselineMetrics, item => item?.recoveryTimeMs?.mean);
      const candidateRecovery = aggregateMetric(candidateMetrics, item => item?.recoveryTimeMs?.mean);
      const baselineStability = summarizeValues(baselineMetrics.map(item => item?.stabilityKbpsP90MinusP50));
      const candidateStability = summarizeValues(candidateMetrics.map(item => item?.stabilityKbpsP90MinusP50));

      const caseDef = caseById.get(config.caseId);
      metrics.scenarios.push({
        id: abId,
        name: config.name,
        network: {
          bandwidthKbps: caseDef?.bandwidth ?? caseDef?.baselineBw ?? 0,
          rttMs: caseDef?.rtt ?? caseDef?.baselineRtt ?? 0,
          lossRate: caseDef?.loss ?? caseDef?.baselineLoss ?? 0,
          jitterMs: caseDef?.jitter ?? caseDef?.baselineJitter ?? 0,
        },
        baseline: {
          goodputKbps: baselineGoodput,
          lossPercent: baselineLoss,
          recoveryTimeMs: baselineRecovery,
          stabilityKbpsP90MinusP50: baselineStability.mean,
          queuePressure: {
            wouldBlockTotal: null,
            queuedVideoRetentions: null,
            audioDeadlineDrops: null,
            retransmissionDrops: null,
          },
          fairnessDeviation: null,
        },
        candidate: {
          goodputKbps: candidateGoodput,
          lossPercent: candidateLoss,
          recoveryTimeMs: candidateRecovery,
          stabilityKbpsP90MinusP50: candidateStability.mean,
          queuePressure: {
            wouldBlockTotal: null,
            queuedVideoRetentions: null,
            audioDeadlineDrops: null,
            retransmissionDrops: null,
          },
          fairnessDeviation: null,
        },
        delta: {
          goodputPercent: toPercentDelta(baselineGoodput.mean, candidateGoodput.mean),
          lossPercent: toPercentDelta(baselineLoss.mean, candidateLoss.mean),
          recoveryTimePercent: toPercentDelta(baselineRecovery.mean, candidateRecovery.mean),
          queuePressurePercent: null,
          fairnessDeviationPercent: null,
        },
        pass: true,
        notes: `Phase=${config.phase}, case=${config.caseId}. Goodput/loss derived from QOS_TRACE sendBps/lossRate samples.`,
      });
    }

    const scenarioById = new Map(metrics.scenarios.map(item => [item.id, item]));
    const gateAb002 = scenarioById.get('AB-002');
    const gateAb003 = scenarioById.get('AB-003');
    const gateAb001 = scenarioById.get('AB-001');
    const gateAb005 = scenarioById.get('AB-005');

    const lossImprovement = gateAb002?.delta?.lossPercent != null
      ? -gateAb002.delta.lossPercent
      : null;
    const recoveryImprovement = gateAb003?.delta?.recoveryTimePercent != null
      ? -gateAb003.delta.recoveryTimePercent
      : null;
    const steadyGoodputRegression = gateAb001?.delta?.goodputPercent ?? null;
    const fairnessRegression = gateAb005?.delta?.fairnessDeviationPercent != null
      ? gateAb005.delta.fairnessDeviationPercent
      : null;

    metrics.gates = [
      {
        id: 'gate-AB-002-loss',
        description: 'AB-002 congestion loss proxy improves >=20%',
        target: '>=20%',
        actual: Number.isFinite(lossImprovement) ? `${lossImprovement.toFixed(2)}%` : 'n/a',
        pass: Number.isFinite(lossImprovement) ? lossImprovement >= 20 : false,
      },
      {
        id: 'gate-AB-003-recovery',
        description: 'AB-003 recovery time not worse, ideally >=15%',
        target: 'not worse, ideally >=15%',
        actual: Number.isFinite(recoveryImprovement) ? `${recoveryImprovement.toFixed(2)}%` : 'n/a',
        pass: Number.isFinite(recoveryImprovement) ? recoveryImprovement >= 0 : false,
      },
      {
        id: 'gate-AB-001-goodput',
        description: 'AB-001 steady-state goodput regression <=5%',
        target: '>=-5%',
        actual: Number.isFinite(steadyGoodputRegression) ? `${steadyGoodputRegression.toFixed(2)}%` : 'n/a',
        pass: Number.isFinite(steadyGoodputRegression) ? steadyGoodputRegression >= -5 : false,
      },
      {
        id: 'gate-AB-005-fairness',
        description: 'AB-005 fairness deviation not increased',
        target: '<=0%',
        actual: Number.isFinite(fairnessRegression) ? `${fairnessRegression.toFixed(2)}%` : 'n/a',
        pass: Number.isFinite(fairnessRegression) ? fairnessRegression <= 0 : false,
      },
      {
        id: 'gate-regression-safety',
        description: 'transport-control hard regression suite',
        target: 'all pass',
        actual: raw.hardRegression.pass ? 'pass' : 'fail',
        pass: raw.hardRegression.pass,
      },
    ];

    metrics.overall.pass = metrics.gates.every(item => item.pass === true);
    metrics.overall.summary = metrics.overall.pass
      ? `${pairing.candidateLabel} preserved regressions and improved targeted TWCC metrics over ${pairing.baselineLabel}.`
      : `${pairing.candidateLabel} did not meet every configured gate over ${pairing.baselineLabel}.`;

    const metricsPath = path.join(runDir, `${pairing.name}.json`);
    writeJson(metricsPath, metrics);
    runCommand('node', [
      path.join(repoRoot, 'tests', 'qos_harness', 'render_twcc_ab_report.mjs'),
      `--input=${metricsPath}`,
      `--output=${path.join(runDir, `${pairing.name}.md`)}`,
    ]);
  }

  console.log(`twcc ab evaluation written to ${runDir}`);
}

main().catch(error => {
  console.error(error);
  process.exitCode = 1;
});
