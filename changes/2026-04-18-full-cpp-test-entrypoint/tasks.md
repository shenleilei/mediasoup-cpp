# Tasks

## 1. Define The Change

- [x] 1.1 Capture the new `run_all_tests.sh` scope as full C++ regression rather than non-QoS-only
  Files: `changes/2026-04-18-full-cpp-test-entrypoint/requirements.md`, `changes/2026-04-18-full-cpp-test-entrypoint/design.md`
  Verification: change docs explicitly describe full C++ coverage and report rename

- [x] 1.2 Record the current coverage gap and the intended grouping model
  Files: `changes/2026-04-18-full-cpp-test-entrypoint/design.md`
  Verification: design explains why tests currently fall through the gap

## 2. Implement Test Target And Script Changes

- [x] 2.1 Split QoS unit coverage into a dedicated gtest target
  Files: `CMakeLists.txt`
  Verification: `cmake --build build --target mediasoup_qos_unit_tests`

- [x] 2.2 Update `scripts/run_all_tests.sh` groups, build targets, and execution so it covers all C++ gtest binaries
  Files: `scripts/run_all_tests.sh`
  Verification: `./scripts/run_all_tests.sh --skip-build unit` and `./scripts/run_all_tests.sh --skip-build qos`

- [x] 2.3 Remove manual C++ case whitelists from `scripts/run_qos_tests.sh`
  Files: `scripts/run_qos_tests.sh`
  Verification: `./scripts/run_qos_tests.sh --skip-browser cpp-unit` and `./scripts/run_qos_tests.sh --skip-browser cpp-integration`

- [x] 2.4 Rename the generated report artifact to match the new scope
  Files: `scripts/run_all_tests.sh`, `docs/full-cpp-test-results.md`
  Verification: actual run rewrites `docs/full-cpp-test-results.md`

- [x] 2.5 Split downlink QoS summary output from downlink case output
  Files: `scripts/run_qos_tests.sh`, `docs/downlink-qos-test-results-summary.md`
  Verification: regular QoS runs update the summary file without touching `docs/downlink-qos-case-results.md`

## 3. Update Accepted Docs

- [x] 3.1 Update entrypoint spec and workflow docs to match the new full-C++ semantics, dedicated QoS unit target, and split downlink outputs
  Files: `specs/current/test-entrypoints.md`, `README.md`, `docs/README.md`, `docs/DEVELOPMENT.md`, `docs/qos-test-coverage_cn.md`, `docs/PRODUCTION_CHECKLIST.md`, `docs/qos-status.md`, `docs/downlink-qos-status.md`
  Verification: docs stop describing `run_all_tests.sh` as non-QoS-only

## 4. Verify

- [x] 4.1 Run targeted script checks
  Verification:
  - `bash -n scripts/run_all_tests.sh`
  - `bash -n scripts/run_qos_tests.sh`
  - `./scripts/run_all_tests.sh --list`
  - `./scripts/run_all_tests.sh --skip-build unit`
  - `./scripts/run_all_tests.sh --skip-build qos`
  - `./scripts/run_qos_tests.sh --skip-browser cpp-unit`
  - `./scripts/run_qos_tests.sh --skip-browser cpp-integration`

- [x] 4.2 Confirm report rewrite behavior
  Verification:
  - compare `stat -c '%Y' docs/full-cpp-test-results.md` before and after a real run
  - confirm `--help` and `--list` do not rewrite the report

## 5. Review

- [x] 5.1 Self-review against `REVIEW.md` and `DELIVERY_CHECKLIST.md`
  Verification: correctness, docs, and verification coverage reviewed together
