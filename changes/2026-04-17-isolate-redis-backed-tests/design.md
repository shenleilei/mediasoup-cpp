# Bugfix Design

## Context

Three integration binaries exercise Redis-backed node discovery and room ownership:

- `mediasoup_review_fix_tests`
- `mediasoup_multinode_tests`
- `mediasoup_topology_tests`

They currently point test SFU processes and direct Redis cleanup calls at `127.0.0.1:6379`.

## Root Cause

The tests rely on deleting `sfu:node:*` and `sfu:room:*` keys from a shared Redis instance, but that does not prevent unrelated SFUs from republishing node heartbeats during the test run. The registry correctly consumes those keys, so test assumptions about the available node set become false.

## Fix Strategy

Add a small header-only Redis test helper that:

- selects a free local TCP port
- starts `redis-server` bound to `127.0.0.1` with persistence disabled
- waits until the private Redis is ready
- exposes the selected port to the test fixture
- shuts the server down and removes its temp directory during teardown

Wire each Redis-backed integration fixture to:

- start its own private Redis server in `SetUp`
- connect cleanup and verification calls to that Redis port
- pass `--redisHost=127.0.0.1 --redisPort=<private-port>` to every test-owned SFU process

Update `scripts/run_all_tests.sh` and workflow docs to reflect that these suites no longer require a pre-existing Redis on `127.0.0.1:6379`, but they do require the `redis-server` binary.

## Risk Assessment

- The new helper depends on `redis-server` being present in the environment.
- Process cleanup must be reliable so tests do not leak Redis instances.
- Tests that intentionally exercise Redis-unavailable behavior must stay pointed at an invalid endpoint.

## Test Strategy

- Reproduction test: rerun `CountryIsolationTest.ChinaClientRoutedToChinaNode` through `mediasoup_review_fix_tests`
- Regression test: rerun the full `mediasoup_review_fix_tests` binary
- Adjacent-path checks: rerun `mediasoup_multinode_tests` and `mediasoup_topology_tests`
- Integration test: the affected binaries already cover cache/pub-sub, geo routing, takeover, and routing selection through real SFU processes

## Observability

- On helper startup failure, surface the Redis log path in assertions so the failure is diagnosable from the test output

## Rollout Notes

- No production rollout impact; this is test infrastructure only
- Rollback is to restore the suites to the shared Redis configuration if the helper proves unstable
