# Tasks

## 1. Reproduce

- [x] 1.1 Identify remaining runtime uses of `RawSenderSnapshot`
  Verification: affected code paths are enumerated before rename

## 2. Fix

- [x] 2.1 Replace `RawSenderSnapshot` with `CanonicalTransportSnapshot` in sender QoS runtime types and interfaces
  Files: `client/qos/*`, `client/PlainClient*`, related type declarations
  Verification: runtime control code no longer references `RawSenderSnapshot`

- [x] 2.2 Centralize local-only transport snapshot construction helpers
  Files: `client/PlainClientSupport.*`, `client/PlainClientLegacy.cpp`, `client/PlainClientThreaded.cpp`
  Verification: legacy and threaded sender QoS paths build control snapshots through shared local-transport helper logic

## 3. Validate

- [x] 3.1 Build affected targets
  Verification: `plain-client` and `mediasoup_qos_unit_tests` compile

- [x] 3.2 Run targeted QoS unit tests
  Verification: sender QoS-related tests still pass
