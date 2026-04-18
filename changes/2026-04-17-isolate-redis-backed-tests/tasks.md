# Tasks

## 1. Reproduce

- [x] 1.1 Capture the failing regression from `run_all_tests.log`
  Verification: `CountryIsolationTest.ChinaClientRoutedToChinaNode` failed because `/api/resolve` returned `ws://47.99.237.234:3000/ws`

## 2. Fix

- [x] 2.1 Add a reusable isolated Redis test helper
  Files: `tests/TestRedisServer.h`
  Verification: Redis-backed suites can start a private Redis and obtain its port

- [x] 2.2 Point Redis-backed integration suites to their private Redis instance
  Files: `tests/test_review_fixes_integration.cpp`, `tests/test_multinode_integration.cpp`, `tests/test_multi_sfu_topology.cpp`
  Verification: rerun the original failing suite and Redis-backed adjacent suites

## 3. Unit And Integration Coverage

- [x] 3.1 Reuse existing integration coverage for the corrected routing logic
  Verification: run `./build/mediasoup_review_fix_tests`

- [x] 3.2 Revalidate Redis-backed multi-node coverage
  Verification: run `./build/mediasoup_multinode_tests` and `./build/mediasoup_topology_tests`

## 4. Guard Against Regression

- [x] 4.1 Update test runner and docs so the execution contract matches the new isolation behavior
  Verification: review `scripts/run_all_tests.sh` and docs for stale Redis assumptions

## 5. Validate Adjacent Behavior

- [x] 5.1 Preserve explicit degraded-without-Redis tests
  Verification: confirm tests that use invalid Redis endpoints remain unchanged

## 6. Delivery Gates

- [x] 6.1 Run required build and targeted test suites
  Verification: record command results

- [x] 6.2 Review `DELIVERY_CHECKLIST.md`
  Verification: all applicable items are resolved

## 7. Review

- [x] 7.1 Run self-review using `REVIEW.md` from the whole-system perspective
  Verification: root cause, regression risk, and boundary impact have been reviewed

## 8. Knowledge Update

- [x] 8.1 Update affected workflow docs and record whether `specs/current/` changes are unnecessary
  Verification: docs match final test behavior
