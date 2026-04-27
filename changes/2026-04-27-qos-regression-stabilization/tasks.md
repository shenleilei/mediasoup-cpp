# Tasks

1. [ ] Stabilize the downlink rate-limit integration test against the documented signaling contract.
   - Files: `tests/test_qos_integration.cpp`
   - Outcome: deterministic expectations for stored/rejected outcomes.
   - Verify: targeted gtest filter.

2. [ ] Repair and normalize the QoS accuracy zero-loss test.
   - Files: `tests/test_qos_accuracy.cpp`
   - Outcome: clean compile and stable localhost assertions.
   - Verify: targeted gtest filter.

3. [ ] Re-run the affected QoS suites and ensure interop black-box coverage stays green.
   - Files: none
   - Outcome: old failures removed without regressing the new gate.
   - Verify: targeted tests plus `scripts/run_qos_tests.sh browser-harness:public-interop`.
