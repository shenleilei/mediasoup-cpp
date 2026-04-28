# Design

## Context

The QoS sweep harness uses `sweep_cases.json` as the shared scenario catalog.
Cases without `extended:true` enter the default browser and cpp-client matrix
gates. The current default gate has 43 cases across groups: baseline, bw_sweep,
loss_sweep, rtt_sweep, jitter_sweep, transition, burst, traffic_model, and
oscillation.

The cpp-client matrix runner (`run_cpp_client_matrix.mjs`) applies
group-specific synthetic shaping to simulate real WebRTC behavior (e.g.,
`qualityLimitationReason: 'bandwidth'` for low-bandwidth bw_sweep cases).

The synthetic sweep unit test (`test.synthetic_sweep.mjs`) validates that
within each single-variable sweep group, expectations grow monotonically as
impairment gets harsher.

## Proposed Approach

1. **Add 12 `gcc_degrade` cases** to `sweep_cases.json`, all with
   `extended:true`. This ensures the default gate stays at 43 cases.

2. **Group-specific shaping** in `buildMatrixTestProfile`:
   - Loss ≥5%: set `qualityLimitationReason = 'bandwidth'` (WebRTC reports
     bandwidth limitation when GCC reduces estimate due to loss).
   - Bandwidth ≤2000 kbps: apply 0.75× send ceiling (same pattern as
     bw_sweep/transition).

3. **Monotonicity validation**: add `gcc_degrade` to the sweep monotonicity
   configs using `loss` as the primary harsher-first sort key. Cases with
   mixed dimensions (RTT + loss) are acceptable because higher loss implies
   equal-or-worse expectations in all GD cases.

## Alternatives Considered

- **Inline into existing `loss_sweep` group**: rejected because GD cases
  combine loss with RTT/BW/jitter transitions, which breaks the single-variable
  sweep contract.
- **Separate JSON file**: rejected because `scenario_catalog.mjs` loads a
  single file and the `extended` flag already provides the needed isolation.
- **Enter default gate immediately**: rejected because no targeted rerun
  evidence exists yet and it would silently change the 43-case gate contract.

## Modules And Responsibilities

- `sweep_cases.json`: case definitions (extended).
- `run_cpp_client_matrix.mjs`: group-specific synthetic shaping.
- `test.synthetic_sweep.mjs`: monotonicity validation.

## Testing Strategy

- Synthetic sweep unit tests validate monotonicity and calibration.
- JSON syntax validated by `node -e "JSON.parse(...)"`.
- Default gate count validated by filtering without `includeExtended`.
- Targeted rerun via `--cases=GD1,...,GD12` or `--include-extended` (future).

## Rollout Notes

- No migration needed.
- Backward compatible: default gate unchanged.
- To promote specific GD cases into the default gate later, remove
  `extended:true` and update docs/gate counts.
