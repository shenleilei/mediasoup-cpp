# Tasks

## 1. Reproduce

- [x] 1.1 Enumerate remaining copy-mode and legacy runtime references in the main binary path
  Verification: affected runtime/build/docs references are identified explicitly

## 2. Fix

- [x] 2.1 Remove copy-mode state and runtime entry from `PlainClientApp`
  Files: `client/PlainClientApp.*`
  Verification: main binary no longer routes to `RunCopyMode()`

- [x] 2.2 Delete copy-mode-only bootstrap helpers/state
  Files: `client/PlainClientApp.*`, `client/ThreadedQosRuntime.*`
  Verification: startup no longer falls back to copy mode and legacy-only fields are removed

- [x] 2.3 Remove legacy runtime file from the main build surface
  Files: `client/CMakeLists.txt`, delete `client/PlainClientLegacy.cpp`
  Verification: `plain-client` links without the legacy source file

- [x] 2.4 Refresh setup/docs/harness launcher expectations
  Files: setup/runtime docs and harness launcher
  Verification: docs no longer advertise `--copy` on the main binary

## 3. Validate

- [x] 3.1 Build affected native targets
  Verification: `plain-client`, `mediasoup_qos_unit_tests`, and `mediasoup_thread_integration_tests` compile

- [x] 3.2 Run targeted threaded runtime checks
  Verification: representative threaded harness scenarios still pass after copy-mode removal
