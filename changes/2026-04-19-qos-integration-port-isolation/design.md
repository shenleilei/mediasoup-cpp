# Design

## Context

The current QoS integration fixtures spawn SFU instances on a fixed port using shell background commands. That leaves two structural problems:

1. consecutive fixtures reuse the same TCP port
2. the test process does not own a direct child lifecycle for the spawned SFU, so teardown cannot deterministically reap the process before the next fixture begins

The result is a classic sequential-suite flake: standalone reruns pass, but full-suite execution can connect to a stale listener or collide with the shutdown window of the previous fixture.

## Proposed Approach

Introduce reusable SFU test helpers in `tests/TestProcessUtils.h` and migrate `tests/test_qos_integration.cpp` to them.

### 1. Dynamic test-port allocation

- add a helper that allocates a unique port from a high test-only range
- verify that the chosen port is free before use
- assign a dedicated port to each fixture instance

This removes cross-fixture port reuse entirely.

### 2. Deterministic SFU process ownership

- start the SFU through `fork` + `exec` instead of shell background launch
- make the spawned SFU the direct child of the test process
- place the SFU in its own process group so teardown can signal the owned process tree cleanly

This gives the fixture direct control over process lifetime and allows reliable `waitpid`-based cleanup.

### 3. Strong shutdown and release confirmation

- enhance the SFU termination helper to:
  - send `SIGTERM`
  - poll for child exit / process disappearance
  - escalate to `SIGKILL` only if required
  - reap the direct child when possible
- after termination, explicitly wait for the fixture’s allocated port to become bindable again

This makes “port released” an enforced postcondition, not a best-effort sleep.

### 4. QoS integration fixture migration

Update `QosIntegrationTest` and `QosRecordingTest` to:

- store `port_` per fixture
- connect clients and HTTP helpers through `port_`
- drop the fixed `14011` assumption
- use the shared helper for startup/shutdown

## Why This Is Correct

- unique ports remove the largest source of inter-test interference
- direct child ownership eliminates ambiguity about which process instance the fixture started
- explicit release checks guarantee the previous fixture finished cleaning up before the next one starts
- the helper remains compatible with older tests because termination still tolerates non-child PIDs

## Alternatives Considered

### Alternative: keep the fixed port and only increase sleeps/timeouts

Rejected because it treats the symptom, not the cause. Fixed-port reuse and shell-spawned processes still leave race windows that will reappear under load or on slower machines.

### Alternative: keep fixed port but improve ready checks

Rejected because a stronger ready check still cannot eliminate reuse of the same port across fixture boundaries.

## Files

- `tests/TestProcessUtils.h`
- `tests/test_qos_integration.cpp`
- change docs under `changes/2026-04-19-qos-integration-port-isolation/`

## Verification

- `./build/mediasoup_qos_integration_tests --gtest_filter='QosIntegrationTest.GetStatsForOtherPeer:QosIntegrationTest.StatsReportIncludesDownlinkConsumerState:QosIntegrationTest.ClientStatsQosStoredAndAggregated' --gtest_repeat=20`
- `./scripts/run_all_tests.sh --skip-build qos`

