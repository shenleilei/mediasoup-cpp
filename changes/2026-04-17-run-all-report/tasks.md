# Tasks

## 1. Planning Updates

- [x] 1.1 Define the generated non-QoS report contract and keep scope to one Markdown artifact
  Verification: requirements and design specify a single generated report file

- [x] 1.2 Map acceptance criteria to concrete script checks
  Verification: verification commands are listed below

## 2. Core Implementation

- [x] 2.1 Add task result tracking and report rendering to `scripts/run_all_tests.sh`
  Files: `scripts/run_all_tests.sh`
  Verification: targeted run rewrites `docs/non-qos-test-results.md`

- [x] 2.2 Add the generated non-QoS report artifact to docs
  Files: `docs/non-qos-test-results.md`
  Verification: report content matches the latest targeted run

## 3. Integration

- [x] 3.1 Update workflow docs to reference the generated non-QoS report
  Files: `README.md`, `docs/DEVELOPMENT.md`, `docs/README.md`
  Verification: docs describe where the latest non-QoS result is written

- [x] 3.2 Record accepted entrypoint behavior in `specs/current/`
  Files: `specs/current/test-entrypoints.md`
  Verification: accepted behavior matches the implemented contract

## 4. Delivery Gates

- [x] 4.1 Run syntax and targeted execution checks
  Verification:
  - `bash -n scripts/run_all_tests.sh`
  - `./scripts/run_all_tests.sh --skip-build unit`
  - `./scripts/run_all_tests.sh --skip-build integration`
  - `stat -c '%Y' docs/non-qos-test-results.md` before and after `./scripts/run_all_tests.sh --list`

- [x] 4.2 Review `DELIVERY_CHECKLIST.md`
  Verification: applicable checklist items reviewed and gaps noted

## 5. Review

- [x] 5.1 Run self-review using `REVIEW.md`
  Verification: correctness, docs, accepted spec, and verification coverage reviewed together
