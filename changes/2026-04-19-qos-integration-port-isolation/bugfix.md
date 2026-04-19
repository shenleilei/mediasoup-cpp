# Bugfix

## Symptom

`tests/test_qos_integration.cpp` can pass when specific cases are run alone, but fail when the larger QoS regression set runs end to end. The observed failures include:

- `QosIntegrationTest.GetStatsForOtherPeer`
- `QosIntegrationTest.StatsReportIncludesDownlinkConsumerState`
- `QosIntegrationTest.ClientStatsQosStoredAndAggregated`

## Reproduction

Run the QoS group through the full C++ regression entrypoint:

```bash
./scripts/run_all_tests.sh --skip-build qos
```

Before the bugfix, the integration binary could fail even though the same failing cases passed when rerun directly:

```bash
./build/mediasoup_qos_integration_tests \
  --gtest_filter='QosIntegrationTest.GetStatsForOtherPeer:QosIntegrationTest.StatsReportIncludesDownlinkConsumerState:QosIntegrationTest.ClientStatsQosStoredAndAggregated'
```

## Observed Behavior

- tests share the fixed SFU port `14011`
- fixture setup only proves that some listener accepts connections on the port
- teardown does not own a deterministic child-process lifecycle for the SFU main process
- sequential tests can observe stale port/process state and connect to the wrong runtime window

## Expected Behavior

- each QoS integration fixture should start its own isolated SFU instance on a unique test port
- fixture teardown should deterministically stop the owned SFU process and confirm that the allocated port is released before the next fixture starts
- the QoS integration binary should be stable under full-suite sequential execution, not only under isolated reruns

## Suspected Scope

- `tests/test_qos_integration.cpp`
- `tests/TestProcessUtils.h`
- any helper introduced for starting/stopping SFU test processes

## Known Non-Affected Behavior

- the affected QoS integration cases pass reliably when rerun alone
- QoS unit / accuracy / recording-accuracy binaries are not the primary source of the failure
- the issue is test-fixture lifecycle instability, not a proven runtime regression in the QoS business logic

## Acceptance Criteria

- QoS integration fixtures no longer share a fixed SFU port
- fixture teardown verifies that the allocated port is released
- repeated targeted reruns of the previously failing cases pass
- `./scripts/run_all_tests.sh --skip-build qos` passes without the previous QoS integration failures

## Regression Expectations

- the new helpers must not require unrelated test files to migrate immediately
- process cleanup must remain safe for existing tests that still use the older spawn style
