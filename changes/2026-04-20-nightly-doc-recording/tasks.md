# Tasks

## 1. Define The Change

- [x] 1.1 Document the nightly doc-recording and retention requirements
  Files: `changes/2026-04-20-nightly-doc-recording/requirements.md`, `changes/2026-04-20-nightly-doc-recording/design.md`, `changes/2026-04-20-nightly-doc-recording/tasks.md`
  Verification: change docs match the requested workflow

## 2. Implement Retention

- [x] 2.1 Add shared 100-entry retention for date-based report archive roots
  Files: `tests/qos_harness/report_artifacts.mjs`, `tests/qos_harness/run_downlink_matrix.mjs`
  Verification: targeted helper tests cover pruning behavior

- [x] 2.2 Add 100-entry retention for nightly artifact run directories
  Files: `scripts/nightly_full_regression.py`
  Verification: dry-run path or targeted helper logic shows only latest timestamped directories are kept

## 3. Implement Git Recording

- [x] 3.1 Stage and commit newly changed docs produced by nightly runs
  Files: `scripts/nightly_full_regression.py`
  Verification: dry-run style invocation records commit status metadata and stages only newly dirty docs

- [x] 3.2 Skip pre-existing dirty doc paths and record the skip result
  Files: `scripts/nightly_full_regression.py`
  Verification: summary metadata includes skipped doc paths when applicable

## 4. Update Accepted Docs

- [x] 4.1 Update spec and runbook docs
  Files: `specs/current/test-entrypoints.md`, `docs/nightly-full-regression.md`
  Verification: docs describe git recording and bounded retention

## 5. Verify

- [x] 5.1 Run targeted checks for changed scripts/helpers
  Verification:
  - `python3 -m py_compile scripts/nightly_full_regression.py`
  - `node tests/qos_harness/test.report_artifacts.mjs`
