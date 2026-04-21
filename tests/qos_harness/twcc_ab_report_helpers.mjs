export function isFiniteNumber(value) {
  return typeof value === 'number' && Number.isFinite(value);
}

export function safe(value, fallback = '-') {
  if (value == null) return fallback;
  if (typeof value === 'string' && value.trim() === '') return fallback;
  return String(value);
}

export function fmtNum(value, digits = 2) {
  return isFiniteNumber(value) ? value.toFixed(digits) : '-';
}

export function fmtInt(value) {
  return isFiniteNumber(value) ? `${Math.round(value)}` : '-';
}

export function fmtPercent(value, digits = 2) {
  return isFiniteNumber(value) ? `${value.toFixed(digits)}%` : '-';
}

export function average(values) {
  const valid = values.filter(isFiniteNumber);
  if (valid.length === 0) return null;
  return valid.reduce((sum, value) => sum + value, 0) / valid.length;
}

export function percentDelta(baselineValue, candidateValue) {
  if (!isFiniteNumber(baselineValue) || !isFiniteNumber(candidateValue) || baselineValue === 0) {
    return null;
  }
  return ((candidateValue - baselineValue) / baselineValue) * 100;
}

export function sumQueuePressure(queuePressure = {}) {
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

export function formatNetwork(network = {}) {
  return [
    `bw ${safe(network.bandwidthKbps, '-')}kbps`,
    `rtt ${safe(network.rttMs, '-')}ms`,
    `loss ${safe(network.lossRate, '-')}`,
    `jitter ${safe(network.jitterMs, '-')}ms`,
  ].join(' / ');
}

export function formatWhiteBox(whiteBox = {}) {
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

const GROUP_PROFILES = [
  {
    match: /^G0\b/,
    groupId: 'G0',
    modeName: '旧发送路径基线',
    purpose: '作为 legacy path 基线，看整条新发送路径相对旧路径的整体效果。',
    toggles: [
      'PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=0',
    ],
  },
  {
    match: /^G1\b/,
    groupId: 'G1',
    modeName: '新发送路径，但关闭 transport estimate',
    purpose: '作为 controller-only 基线，用来隔离 transport estimate / TWCC 主路径本身的净收益。',
    toggles: [
      'PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=1',
      'PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE=0',
    ],
  },
  {
    match: /^G2\b/,
    groupId: 'G2',
    modeName: '新发送路径，并打开 transport estimate',
    purpose: '当前 candidate 路径，包含 transport controller 与 send-side estimate 主路径。',
    toggles: [
      'PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=1',
      'PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE=1',
    ],
  },
];

export function groupProfile(label) {
  return GROUP_PROFILES.find(item => item.match.test(label ?? '')) ?? {
    groupId: safe(label, '-'),
    modeName: safe(label, '-'),
    purpose: '-',
    toggles: [],
  };
}

export function pairPurpose(baselineLabel, candidateLabel) {
  const baseline = groupProfile(baselineLabel);
  const candidate = groupProfile(candidateLabel);

  if (baseline.groupId === 'G1' && candidate.groupId === 'G2') {
    return {
      short: '这组对比用来隔离 transport estimate / TWCC 主路径本身的净收益。',
      focus: '重点看 recovery、拥塞期 loss proxy、以及 estimate/pacing 白盒字段是否形成正向变化。',
    };
  }
  if (baseline.groupId === 'G0' && candidate.groupId === 'G2') {
    return {
      short: '这组对比用来看新整条发送路径相对旧路径的整体效果。',
      focus: '重点看整条 transport-control 路径是否带来整体收益，或至少没有引入明显退化。',
    };
  }
  return {
    short: `${safe(candidateLabel)} 相对 ${safe(baselineLabel)} 的 pairwise 对比。`,
    focus: '重点先看 overall、gate、再看场景级结果。',
  };
}

export const AB_SCENARIO_GUIDE = {
  'AB-001': {
    label: '稳态',
    why: '看 steady-state goodput 与抖动有没有明显退化。',
    focus: 'goodput / stability',
  },
  'AB-002': {
    label: '拥塞阶段',
    why: '看带宽骤降时 loss proxy 和 queue pressure 是否改善。',
    focus: 'loss / queue pressure / feedback',
  },
  'AB-003': {
    label: '恢复阶段',
    why: '看恢复到可用发送状态的速度是否更快，或至少不变差。',
    focus: 'recovery time',
  },
  'AB-004': {
    label: '高 RTT 稳定性',
    why: '看高时延下是否保持稳定，不引入额外退化。',
    focus: 'goodput / stability',
  },
  'AB-005': {
    label: '多轨公平性',
    why: '看多视频轨预算分配是否更偏，还是保持原有公平性。',
    focus: 'fairness deviation',
  },
};

function describeGoodput(delta, baselineValue, candidateValue) {
  if (!isFiniteNumber(delta)) return null;
  if (Math.abs(delta) < 0.5) {
    return `总体 goodput 基本持平（${fmtPercent(delta)}）。`;
  }
  if (delta > 0) {
    return `总体 goodput 提升 ${fmtPercent(delta)}（${fmtNum(baselineValue)} -> ${fmtNum(candidateValue)} kbps）。`;
  }
  return `总体 goodput 下降 ${fmtPercent(Math.abs(delta))}（${fmtNum(baselineValue)} -> ${fmtNum(candidateValue)} kbps）。`;
}

function describeRecovery(delta, baselineValue, candidateValue) {
  if (!isFiniteNumber(delta)) return null;
  if (Math.abs(delta) < 0.5) {
    return `恢复时间基本持平（${fmtNum(baselineValue)}ms -> ${fmtNum(candidateValue)}ms）。`;
  }
  if (delta < 0) {
    return `恢复时间更快，改善 ${fmtPercent(Math.abs(delta))}（${fmtNum(baselineValue)}ms -> ${fmtNum(candidateValue)}ms）。`;
  }
  return `恢复时间变慢 ${fmtPercent(delta)}（${fmtNum(baselineValue)}ms -> ${fmtNum(candidateValue)}ms）。`;
}

function describeLoss(delta) {
  if (!isFiniteNumber(delta)) return null;
  if (Math.abs(delta) < 0.5) {
    return `总体 loss 基本持平（${fmtPercent(delta)}）。`;
  }
  if (delta < 0) {
    return `总体 loss 下降 ${fmtPercent(Math.abs(delta))}。`;
  }
  return `总体 loss 上升 ${fmtPercent(delta)}。`;
}

function describeFairness(delta) {
  if (!isFiniteNumber(delta)) return null;
  if (Math.abs(delta) < 0.5) {
    return `多轨公平性基本持平（权重偏差变化 ${fmtPercent(delta)}）。`;
  }
  if (delta < 0) {
    return `多轨公平性改善（权重偏差下降 ${fmtPercent(Math.abs(delta))}）。`;
  }
  return `多轨公平性变差（权重偏差上升 ${fmtPercent(delta)}）。`;
}

export function buildPairTakeaways({
  report,
  baselineGoodput,
  candidateGoodput,
  baselineLoss,
  candidateLoss,
  baselineRecovery,
  candidateRecovery,
  baselineFairness,
  candidateFairness,
}) {
  const lines = [];
  const purpose = pairPurpose(report?.baseline?.label, report?.candidate?.label);

  if (report?.overall?.pass === true) {
    lines.push('这次 pairwise 对比通过了全部预设 gate。');
  } else if (report?.overall?.pass === false) {
    lines.push('这次 pairwise 对比没有通过全部预设 gate；这表示 candidate 还没有在当前门槛下“全面胜出”，不自动等于发送路径损坏。');
  }

  if (
    report?.baseline?.commit &&
    report?.candidate?.commit &&
    report.baseline.commit === report.candidate.commit
  ) {
    lines.push('Baseline 与 Candidate 使用同一提交，比较的是运行模式，不是两份不同代码。');
  }

  lines.push(purpose.short);

  const goodputDelta = percentDelta(baselineGoodput, candidateGoodput);
  const lossDelta = percentDelta(baselineLoss, candidateLoss);
  const recoveryDelta = percentDelta(baselineRecovery, candidateRecovery);
  const fairnessDelta = percentDelta(baselineFairness, candidateFairness);

  const metricFindings = [
    describeGoodput(goodputDelta, baselineGoodput, candidateGoodput),
    describeLoss(lossDelta),
    describeRecovery(recoveryDelta, baselineRecovery, candidateRecovery),
    describeFairness(fairnessDelta),
  ].filter(Boolean);

  if (metricFindings.length > 0) {
    lines.push(metricFindings.join(' '));
  }

  const failedGates = (Array.isArray(report?.gates) ? report.gates : []).filter(item => item?.pass === false);
  if (failedGates.length > 0) {
    lines.push(
      `阻塞项：${failedGates.map(item => `${safe(item.description, item.id)}（目标 ${safe(item.target)}，实际 ${safe(item.actual)}）`).join('；')}。`
    );
  }

  return lines;
}
