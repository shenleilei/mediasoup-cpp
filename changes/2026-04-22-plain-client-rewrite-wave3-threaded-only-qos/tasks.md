# Tasks

## 1. Reproduce

- [x] 1.1 Confirm legacy path still owns sender QoS runtime logic
  Verification: controller creation and runtime sampling paths are identified

## 2. Fix

- [x] 2.1 Restrict sender QoS controller creation to threaded mode
  Files: `client/PlainClientApp.cpp`
  Verification: legacy mode no longer instantiates sender QoS controllers

- [x] 2.2 Delete legacy sender QoS runtime logic
  Files: `client/PlainClientLegacy.cpp`
  Verification: sender QoS sampling/coordination/snapshot logic is removed from legacy mode

## 3. Validate

- [x] 3.1 Build affected targets
  Verification: `plain-client` and `mediasoup_qos_unit_tests` compile

- [x] 3.2 Run targeted QoS unit tests
  Verification: shared sender QoS logic still passes targeted tests
