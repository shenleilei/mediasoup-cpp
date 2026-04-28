# Tasks

## 1. Reproduce

- [x] 1.1 Audit the plain-client full/targeted artifact path definitions and identify all inconsistent consumers.
  Files: `scripts/run_qos_tests.sh`, `tests/qos_harness/cpp_client_report_artifacts.mjs`, relevant docs
  Verification: mismatches are listed before code changes

## 2. Fix

- [x] 2.1 Align the targeted plain-client markdown output path across shell and Node layers.
  Files: `scripts/run_qos_tests.sh`
  Verification: path literals match the helper-defined canonical path

- [x] 2.2 Add or extend regression coverage for plain-client artifact path helpers.
  Files: `tests/qos_harness/test.report_artifacts.mjs`
  Verification: Node tests pass

- [x] 2.3 Correct current artifact index docs that label targeted outputs but link full artifacts.
  Files: `docs/README.md`, other current index docs as needed
  Verification: targeted labels point to targeted paths

## 3. Validate Adjacent Behavior

- [x] 3.1 Re-check browser uplink and downlink path families for similar mismatches and keep them unchanged if already consistent.
  Verification: audit notes recorded; no unintended path churn

## 4. Delivery Gates

- [x] 4.1 Run targeted verification for artifact helper behavior and path usage.
  Verification: record command results
  Result: `node --test tests/qos_harness/test.report_artifacts.mjs` passed with the new plain-client path checks and shell-path alignment assertion.

- [x] 4.2 Review `DELIVERY_CHECKLIST.md` and record any residual gaps.
  Verification: applicable items satisfied or noted
  Result: no behavior/runtime contract changed; residual risk is limited to historical docs that intentionally discuss older archived runs and were left untouched unless they served as current artifact indexes.
