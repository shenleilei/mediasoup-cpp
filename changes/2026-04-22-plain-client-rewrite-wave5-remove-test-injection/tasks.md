# Tasks

## 1. Reproduce

- [x] 1.1 Enumerate remaining `QOS_TEST_*` runtime hooks under `client/`
  Verification: affected production files and harness dependencies are identified explicitly

## 2. Fix

- [x] 2.1 Remove `QOS_TEST_*` parsing and runtime state from plain-client
  Files: `client/PlainClientApp.*`, `client/PlainClientSupport.*`, `client/PlainClientThreaded.cpp`
  Verification: sender sampling and runtime signaling no longer depend on test env payloads

- [x] 2.2 Remove test-only failure injection from `SourceWorker`
  Files: `client/SourceWorker.h`
  Verification: no production `SourceWorker` path reads `QOS_TEST_*`

- [x] 2.3 Narrow plain-client harness/scripts to externally driven scenarios
  Files: `tests/qos_harness/*`, `scripts/run_qos_tests.sh`
  Verification: remaining plain-client harness scenarios do not rely on in-process synthetic injection

- [x] 2.4 Refresh docs/specs for the narrowed plain-client regression surface
  Files: `docs/qos/plain-client/*`, `specs/current/*`
  Verification: docs no longer claim removed synthetic scenarios as plain-client runtime coverage

## 3. Validate

- [x] 3.1 Build affected native targets
  Verification: `plain-client`, `mediasoup_qos_unit_tests`, and `mediasoup_thread_integration_tests` compile

- [x] 3.2 Run targeted sender/runtime tests
  Verification: targeted QoS unit tests still pass after hook removal

- [x] 3.3 Run script syntax and residue checks
  Verification: edited JS scripts parse and `rg "QOS_TEST_" client` returns no matches
