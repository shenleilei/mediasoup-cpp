# Tasks

## 1. Reproduce

- [x] 1.1 Confirm `non-qos` still triggers QoS/threaded-only prebuild targets
  Verification: inspect the existing `build_targets()` hard-coded list

## 2. Fix

- [x] 2.1 Make `build_targets()` derive its target list from `SELECTED_GROUPS`
  Files: `scripts/run_all_tests.sh`
  Verification: target list excludes `mediasoup_thread_integration_tests` and `plain-client` for `non-qos`

- [x] 2.2 Build `mediasoup_thread_integration_tests` with `-j1`
  Files: `scripts/run_qos_tests.sh`
  Verification: the scripted build command for `mediasoup_thread_integration_tests` uses `cmake --build ... -j1`

- [x] 2.3 Include vendored worker tests in the default full regression entrypoint
  Files: `scripts/run_all_tests.sh`, `specs/current/test-entrypoints.md`
  Verification: `worker` becomes a first-class group, `non-qos` expands to include it, and the worker group runs a stable default subset

## 3. Validate

- [x] 3.1 Run `bash -n scripts/run_all_tests.sh`
  Verification: syntax check passes

- [x] 3.2 Verify selected-group target resolution
  Verification: inspect output for `non-qos` and default group selection

- [x] 3.3 Verify threaded integration build command
  Verification: inspect the resolved build command for `mediasoup_thread_integration_tests`

- [ ] 3.4 Verify vendored worker command selection
  Verification: inspect the resolved worker command, including tag subset and constrained parallelism
