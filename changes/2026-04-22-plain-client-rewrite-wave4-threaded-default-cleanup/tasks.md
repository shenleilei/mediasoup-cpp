# Tasks

## 1. Reproduce

- [x] 1.1 Enumerate remaining threaded opt-in and fallback references
  Verification: affected code, scripts, tests, and docs are identified explicitly

## 2. Fix

- [x] 2.1 Remove redundant threaded opt-in state from `PlainClientApp`
  Files: `client/PlainClientApp.*`
  Verification: runtime decisions no longer depend on a stored `threadedMode_` flag

- [x] 2.2 Ensure queued notifications land on the final threaded controller set
  Files: `client/PlainClientApp.cpp`
  Verification: session bootstrap no longer creates transitional sender QoS controllers or dispatches queued notifications before `RunThreadedMode()`

- [x] 2.3 Update harnesses, tests, and setup examples to the default threaded runtime contract
  Files: `setup.sh`, `tests/qos_harness/*`, `tests/test_thread_integration.cpp`
  Verification: edited entrypoints stop exporting `PLAIN_CLIENT_THREADED=1` and no scenario expects legacy fallback

- [x] 2.4 Refresh plain-client architecture/checklist docs
  Files: `docs/qos/plain-client/*`
  Verification: docs no longer describe `PLAIN_CLIENT_THREADED=1` as the threaded runtime entry switch

## 3. Validate

- [x] 3.1 Build affected targets
  Verification: `plain-client` and `mediasoup_qos_unit_tests` compile

- [x] 3.2 Run targeted sender QoS unit tests
  Verification: shared sender QoS logic still passes after the cleanup

- [x] 3.3 Run edited script syntax checks and residue search
  Verification: JS harness scripts parse and repo-local `PLAIN_CLIENT_THREADED` references are removed from supported paths
