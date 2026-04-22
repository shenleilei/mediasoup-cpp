# Tasks

## 1. Reproduce

- [x] 1.1 Capture the threaded runtime responsibilities currently embedded in `RunThreadedMode()`
  Verification: queue/thread/controller/loop ownership is enumerated before extraction

## 2. Fix

- [x] 2.1 Introduce a dedicated threaded runtime object
  Files: new threaded runtime implementation files
  Verification: runtime object owns threaded queue/thread/controller lifecycle

- [x] 2.2 Reduce `PlainClientApp::RunThreadedMode()` to a thin delegation entrypoint
  Files: `client/PlainClientThreaded.cpp`, `client/PlainClientApp.*`
  Verification: app shell no longer contains the threaded control loop body

- [x] 2.3 Move runtime-only counters/state off `PlainClientApp`
  Files: `client/PlainClientApp.h`, new runtime files
  Verification: peer snapshot sequencing and threaded loop state live in the runtime object

- [x] 2.4 Refresh docs/build wiring
  Files: `client/CMakeLists.txt`, affected docs
  Verification: new runtime files build and docs reflect the extracted structure

## 3. Validate

- [x] 3.1 Build affected native targets
  Verification: `plain-client`, `mediasoup_qos_unit_tests`, and `mediasoup_thread_integration_tests` compile

- [x] 3.2 Run targeted sender/runtime checks
  Verification: unit tests and representative threaded harness scenarios still pass
