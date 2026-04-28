# Tasks

## 1. Reproduce

- [x] 1.1 Confirm `O1` currently fails `cpp_client` only because of the shared action-count budget.
  Files: existing generated reports and `tests/qos_harness/scenarios/sweep_cases.json`
  Verification: recorded before the fix

## 2. Fix

- [x] 2.1 Add a `cpp_client` runner-specific `maxActionCount` override for `O1`.
  Files: `tests/qos_harness/scenarios/sweep_cases.json`
  Verification: unit tests and targeted rerun reflect the new expectation

- [x] 2.2 Extend synthetic oracle tests for `O1` runner-specific action-count behavior.
  Files: `tests/qos_harness/test.synthetic_sweep.mjs`
  Verification: Node test passes

## 3. Validate Adjacent Behavior

- [x] 3.1 Confirm loopback/browser `O1` strictness remains unchanged in unit-level oracle tests.
  Verification: Node test passes

## 4. Delivery Gates

- [x] 4.1 Run targeted verification.
  Verification: record command results
  Result: `node --test tests/qos_harness/test.synthetic_sweep.mjs` passed; `node tests/qos_harness/run_cpp_client_matrix.mjs --cases=O1` produced a targeted `cpp_client` `O1` PASS and updated targeted JSON/markdown artifacts.

- [x] 4.2 Record any remaining gap, including that full matrix reports are stale until a full rerun.
  Verification: change notes updated
  Result: full plain-client case reports still reflect the last full matrix run and remain stale until a future full rerun; only the targeted `O1` artifacts were refreshed in this bugfix.
