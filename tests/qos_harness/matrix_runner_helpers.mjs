import { getPhaseNetwork } from './synthetic_sweep_shared.mjs';

export function isMildBaselineNetwork(network = {}) {
  return (
    (network.bandwidth ?? 0) >= 2000 &&
    (network.rtt ?? 0) <= 80 &&
    (network.loss ?? 0) <= 2 &&
    (network.jitter ?? 0) <= 20
  );
}

export function detectBaselineContamination(caseDef, baselineState) {
  if (!baselineState) {
    return null;
  }

  // Baseline-group cases intentionally validate weak baseline behavior and should
  // be judged by their declared oracle, not by the infrastructure contamination
  // shortcut.
  if (caseDef.group === 'baseline') {
    return null;
  }

  if (baselineState.state === 'recovering') {
    return 'baseline entered recovering before any impairment';
  }

  const baselineNetwork = getPhaseNetwork(caseDef, 'baseline');
  if (!isMildBaselineNetwork(baselineNetwork)) {
    return null;
  }

  if (baselineState.state === 'congested') {
    return `mild baseline network reported congested/L${baselineState.level}`;
  }

  if ((baselineState.level ?? 0) > 1) {
    return `mild baseline network reported level L${baselineState.level}`;
  }

  return null;
}
