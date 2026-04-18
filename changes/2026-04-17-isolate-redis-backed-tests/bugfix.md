# Bugfix Analysis

## Summary

Redis-backed integration tests can read live `sfu:node:*` advertisements from an unrelated SFU that is also connected to `127.0.0.1:6379`.

## Reproduction

1. Run an external SFU against Redis on `127.0.0.1:6379` so it continuously republishes `sfu:node:*`.
2. Run `./scripts/run_all_tests.sh` or `./build/mediasoup_review_fix_tests`.
3. Observe `CountryIsolationTest.ChinaClientRoutedToChinaNode` fail because `/api/resolve` selects the external China node instead of the test-owned China node.

## Observed Behavior

`mediasoup_review_fix_tests` clears Redis keys before startup, but a live external SFU can republish its node key during the test. The registry then sees an unexpected third node and returns `ws://47.99.237.234:3000/ws`, breaking assertions that expect only the locally started test nodes.

## Expected Behavior

Redis-backed integration tests should run against a private Redis instance so only test-owned node and room keys participate in routing decisions.

## Known Scope

- `tests/test_review_fixes_integration.cpp`
- `tests/test_multinode_integration.cpp`
- `tests/test_multi_sfu_topology.cpp`
- `scripts/run_all_tests.sh`
- test workflow documentation

## Must Not Regress

- Geo routing and country isolation behavior under Redis-backed multi-node tests
- Cache/pub-sub behavior across multiple SFU processes
- Existing single-node and degraded-without-Redis tests

## Suspected Root Cause

High confidence: the Redis-backed integration suites use the shared default Redis endpoint on `127.0.0.1:6379`, so they are not isolated from external node heartbeats.

## Acceptance Criteria

### Requirement 1

The Redis-backed integration suites SHALL run against an isolated Redis instance that is started and stopped by the test process.

#### Scenario: Regression path

- WHEN an unrelated SFU is publishing `sfu:node:*` to the system Redis
- THEN the Redis-backed integration suites still only see the nodes they started for the test run

### Requirement 2

The original failing country-isolation regression SHALL pass against the isolated test Redis.

#### Scenario: Country isolation

- WHEN `CountryIsolationTest.ChinaClientRoutedToChinaNode` runs
- THEN `/api/resolve` returns the test-owned China node rather than an external same-country node

## Regression Expectations

- Existing unaffected behavior: single-node suites that intentionally use invalid Redis continue to run unchanged
- Required automated regression coverage: rerun `mediasoup_review_fix_tests`, `mediasoup_multinode_tests`, and `mediasoup_topology_tests`
- Required manual smoke checks: none beyond automated suite execution
