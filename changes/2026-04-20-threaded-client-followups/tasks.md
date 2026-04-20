# Tasks

## 1. Fix Threaded Probe Coverage

- [x] 1.1 Make `sendComediaProbe()` emit probe RTP for every registered video track
  Files: `client/NetworkThread.h`, `tests/test_thread_integration.cpp`
  Verification: `ctest --test-dir build --output-on-failure -R '^mediasoup_thread_integration_tests$'`

## 2. Fix Signal Installation

- [x] 2.1 Replace `signal()` with `sigaction()` in the plain client
  Files: `client/PlainClientApp.cpp`
  Verification: `cmake --build build --target mediasoup_tests mediasoup_thread_integration_tests -- -j2` and `cmake --build client/build --target plain-client -- -j2`

## 3. Fix Per-Run Snapshot Sequencing

- [x] 3.1 Move threaded peer snapshot sequencing to `PlainClientApp` instance state
  Files: `client/PlainClientApp.h`, `client/PlainClientThreaded.cpp`
  Verification: `ctest --test-dir build --output-on-failure -R 'mediasoup_tests|mediasoup_thread_integration_tests'`

## 4. Delivery Gates

- [x] 4.1 Run the final targeted build/test set
  Verification:
  - `cmake --build build --target mediasoup_tests mediasoup_thread_integration_tests -- -j2`
  - `cmake --build client/build --target plain-client -- -j2`
  - `ctest --test-dir build --output-on-failure -R 'mediasoup_tests|mediasoup_thread_integration_tests'`

- [x] 4.2 Review `docs/aicoding/DELIVERY_CHECKLIST.md`
  Verification: all applicable items are resolved or recorded explicitly

## 5. Review

- [x] 5.1 Run self-review using `docs/aicoding/REVIEW.md`
  Verification: startup behavior, protocol coverage, and residual client debt are reviewed explicitly
