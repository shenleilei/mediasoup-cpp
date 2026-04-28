# Design: Synthetic Model Calibration Improvements

## Context

The QoS test harness uses a synthetic value model to simulate WebRTC congestion
control (GCC) behavior for regression testing. Prior to this change, the model
had significant calibration gaps compared to published empirical data, making the
synthetic test suite less trustworthy as a regression gate.

This change keeps the synthetic suite as the primary regression mechanism (not
real WebRTC clients) but improves the model's fidelity against empirical data so
that failures in the synthetic suite are more likely to correspond to real-world
regressions.

## Approach

### JS Synthetic Model (`synthetic_sweep_shared.mjs`)

Three components of the synthetic value pipeline were recalibrated:

1. **Loss utilization weight**: Increased from 0.15 → 0.40 and normalization
   changed from `lossPct/10` → `lossPct/6`. Added utilization caps at 10%/20%
   loss thresholds. **Rationale**: SIGCOMM 2018 GCC measurements show that 5%
   packet loss drops utilization to ~0.5, not the ~0.9 produced by the old
   model. The multiplicative decrease behavior of GCC (0.85× per loss event)
   means loss impact is much more aggressive than the old linear stress model
   implied.

2. **RTT amplification**: Replaced linear doubling (`baseRtt + min(baseRtt, 100)`)
   with logarithmic model (`baseRtt + 5 + 15 * log2(1 + baseRtt/50)`).
   **Rationale**: TMA 2021 empirical measurements show that WebRTC reported RTT
   has higher amplification at low RTT (protocol overhead dominates at ~1.5×)
   and converges toward 1.0× at high RTT. The old 2× flat doubling overstated
   RTT for moderate values and understated protocol overhead for low values.

3. **Jitter smoothing**: Applied 0.75× factor to raw jitter. **Rationale**:
   WebRTC reports EWMA-smoothed jitter per RFC 3550 §6.4.1 (`J = J + (|D| - J)
   / 16`). The smoothing filter empirically reduces reported jitter to ~75% of
   raw network jitter.

### C++ CC Convergence Simulation (`PlainClientSupport.cpp`)

Phase transitions in the matrix test profile no longer jump values instantly.
All network-derived metrics (sendCeiling, RTT, jitter, lossRate) now use
exponential convergence with asymmetric time constants:

- **τ_degrade = 1500ms**: GCC detects congestion in ~1–2 seconds
- **τ_recover = 6000ms**: GCC additive-increase recovery takes ~5–7 seconds

**Design decision – qualityLimitationReason stays instantaneous**: The real
WebRTC encoder reports `qualityLimitationReason` as a discrete enum based on its
current state, not a smoothed network metric. Applying temporal smoothing to
this field would introduce a synthetic delay that does not exist in real
clients. This is intentionally documented in the code.

### Legacy Forcing Values

The following legacy overrides existed in `run_cpp_client_matrix.mjs` prior to
this calibration work:

| Override | Status | Rationale |
|---|---|---|
| B3 RTT floor 230ms | **Removed** | The calibrated logarithmic RTT model now produces an appropriate value (~76ms for base RTT 55ms). The old 230ms floor was a pre-calibration forcing value that contradicted the model. |
| BW/transition `sendCeilingBps × 0.75` for bw≤1000 | Retained | This reflects real GCC behavior where available bandwidth limits the send rate below the model's pure utilization estimate. Not a calibration issue. |
| Burst `qualityLimitationReason = 'bandwidth'` for bw≤300 | Retained | Real encoders would report bandwidth limitation at such low rates. Consistent with the model's own utilization caps. |
| Jitter sweep `jitterMs` floor for jitter≥40 | Retained | Ensures the C++ side sees enough jitter to trigger state transitions. Compensates for the smoothing factor, not a calibration bypass. |

### Plan for Remaining Legacy Overrides

The retained overrides listed above are not calibration bypasses but rather
domain-specific adjustments that compensate for limitations of the static
utilization model (which cannot simulate dynamic GCC probing, encoder feedback
loops, or multi-second convergence within the JS pipeline). They should be
reviewed when/if the synthetic model is replaced by recorded real-WebRTC traces.

## Alternatives Considered

1. **Replace synthetic suite with recorded traces**: Would provide higher
   fidelity but requires infrastructure for recording and replay. Deferred as a
   future improvement.

2. **Converge all fields including qualityLimitationReason**: Rejected because
   the real encoder flag is discrete, not smoothed. Converging it would make the
   synthetic world less realistic, not more.

3. **Remove all legacy overrides at once**: Rejected because some overrides
   compensate for structural model limitations, not calibration gaps. Removing
   them would cause false test failures without improving model accuracy.

## Testing Strategy

- 5 new calibration validation tests compare model output against empirical
  ranges from published literature
- All existing sweep ordering, expectation, and evaluation tests must remain
  green
- Verification: `node --test tests/qos_harness/test.synthetic_sweep.mjs`
