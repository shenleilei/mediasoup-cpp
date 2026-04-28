# Tasks

1. [x] Recalibrate loss stress normalization and utilization weight.
   - Files: `tests/qos_harness/synthetic_sweep_shared.mjs`
   - Change: `lossPct/10` → `lossPct/6`, weight `0.15` → `0.40`
   - Verify: calibration validation test bounds

2. [x] Replace linear RTT amplification with logarithmic model.
   - Files: `tests/qos_harness/synthetic_sweep_shared.mjs`
   - Change: `baseRtt + min(baseRtt, 100)` → `baseRtt + 5 + 15 * log2(1 + baseRtt/50)`
   - Verify: amplification ratio tests

3. [x] Apply RFC 3550 jitter smoothing to synthetic jitter output.
   - Files: `tests/qos_harness/synthetic_sweep_shared.mjs`
   - Change: raw jitter → `jitter * 0.75`
   - Verify: smoothing factor test

4. [x] Add CC convergence temporal simulation to C++ applyMatrixTestProfile.
   - Files: `client/PlainClientSupport.h`, `client/PlainClientSupport.cpp`
   - Change: instant phase values → exponential convergence (τ_down=1.5s, τ_up=6s)
   - Verify: JS behavioral tests for time constant properties (63% at 1τ, asymmetric degrade/recover)
   - Note: C++ unit test deferred; JS reimplementation validates algorithm specification

5. [x] Add empirical calibration validation tests.
   - Files: `tests/qos_harness/test.synthetic_sweep.mjs`
   - Added: RTT amplification bounds, jitter smoothing, loss utilization ranges, severe conditions
   - Verify: `node --test tests/qos_harness/test.synthetic_sweep.mjs`

6. [x] Add legacy override compatibility tests.
   - Files: `tests/qos_harness/test.synthetic_sweep.mjs`
   - Added: bw<=1000 sendCeiling override range, burst bw<=300 consistency, jitter floor range
   - Verify: `node --test tests/qos_harness/test.synthetic_sweep.mjs`

7. [x] Add CC convergence behavioral tests.
   - Files: `tests/qos_harness/test.synthetic_sweep.mjs`
   - Added: degrade 1τ, recover 1τ, asymmetry, 3τ convergence, loss rate convergence
   - Verify: `node --test tests/qos_harness/test.synthetic_sweep.mjs`
