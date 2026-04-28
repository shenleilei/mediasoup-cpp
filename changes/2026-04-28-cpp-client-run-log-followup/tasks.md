# Tasks

## 1. Harness Fix

- [x] 1.1 Parse `QOS_TRACE` from spdlog-prefixed lines and merge stdout/stderr trace sources.
  Files: `tests/qos_harness/cpp_client_runner.mjs`
  Verification: targeted Node tests

## 2. Regression Coverage

- [x] 2.1 Add a Node regression test for stderr-only and spdlog-prefixed trace parsing.
  Files: `tests/qos_harness/test.cpp_client_runner_trace.mjs`
  Verification: targeted Node tests

## 3. Targeted Verification

- [x] 3.1 Run the new Node regression test.
  Result: `node --test tests/qos_harness/test.cpp_client_runner_trace.mjs` passed.

- [x] 3.2 Re-run the threaded integration subtests implicated by the historical `rc=143` log and record whether they now pass.
  Result: `./build/mediasoup_thread_integration_tests --gtest_filter=NetworkThreadIntegration.DisableTransportControllerUsesLegacyPacingFallback:NetworkThreadIntegration.LegacyPacingShutdownWithZeroTargetDoesNotSpin` passed.
