# Requirements: Synthetic Model Calibration Improvements

## Context
The QoS test harness uses synthetic values to simulate WebRTC congestion control
behavior. Analysis against published empirical data (SIGCOMM 2018, TMA 2021,
RFC 3550) revealed systematic biases in the current model.

## Requirements

1. **Loss impact calibration**: Synthetic utilization under packet loss must
   match empirical GCC data (5% loss → util ~0.5, not ~0.9).

2. **RTT amplification**: Reported RTT must follow a logarithmic model where
   low RTT has higher amplification (protocol overhead) and high RTT converges
   toward 1.0x, matching TMA 2021 measurements.

3. **Jitter smoothing**: Synthetic jitter must apply RFC 3550 EWMA smoothing
   (≈0.75× raw value) since WebRTC reports smoothed jitter, not raw.

4. **CC convergence simulation**: Phase transitions in the C++ matrix test
   profile must not jump network-derived metrics instantly; use exponential
   convergence with empirical time constants (degrade ~1.5s, recover ~6s) for
   sendCeiling, RTT, jitter, and lossRate. Note: `qualityLimitationReason` is
   excluded from convergence because it is a discrete encoder flag in real
   WebRTC, not a smoothed network metric.

5. **Calibration validation tests**: Test suite must validate that synthetic
   values stay within empirically measured bounds.

## Acceptance Criteria
- `node --test tests/qos_harness/test.synthetic_sweep.mjs` passes
- Calibration tests validate utilization against empirical ranges
- Existing sweep ordering and evaluation tests remain green
