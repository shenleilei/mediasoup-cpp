# Tasks

## 1. Core Implementation

- [x] 1.1 Add 12 `gcc_degrade` cases (GD1–GD12) to `sweep_cases.json`
  Files: `tests/qos_harness/scenarios/sweep_cases.json`
  Verification: JSON parses without error

- [x] 1.2 Mark all GD cases with `extended:true`
  Files: `tests/qos_harness/scenarios/sweep_cases.json`
  Verification: `filterScenarioCatalog()` default returns 43 cases

## 2. Synthetic Shaping

- [x] 2.1 Add `gcc_degrade` group-specific shaping in `buildMatrixTestProfile`
  Files: `tests/qos_harness/run_cpp_client_matrix.mjs`
  Verification: code review — loss ≥5% sets `qualityLimitationReason`

## 3. Validation

- [x] 3.1 Add `gcc_degrade` to monotonicity configs in `test.synthetic_sweep.mjs`
  Files: `tests/qos_harness/test.synthetic_sweep.mjs`
  Verification: `node --test test.synthetic_sweep.mjs` passes

## 4. Change Documentation

- [x] 4.1 Create structured change folder with requirements, design, and tasks
  Files: `changes/2026-04-28-gcc-degrade-test-cases/`
  Verification: documents exist and are consistent

## 5. Delivery Gates

- [x] 5.1 JSON syntax validation
  Verification: `node -e "JSON.parse(...)"`

- [ ] 5.2 Run synthetic sweep unit tests
  Verification: `node --test tests/qos_harness/test.synthetic_sweep.mjs`

- [ ] 5.3 Verify default gate count is still 43
  Verification: inline script counting non-extended cases
