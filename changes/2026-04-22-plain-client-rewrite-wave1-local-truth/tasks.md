# Tasks

## 1. Reproduce

- [x] 1.1 Capture the server-stats-as-control-input paths in legacy and threaded sender QoS sampling
  Verification: affected files and state are identified explicitly

## 2. Fix

- [x] 2.1 Remove server `getStats` from sender QoS control sampling
  Files: `client/PlainClientApp.*`, `client/PlainClientLegacy.cpp`, `client/PlainClientThreaded.cpp`
  Verification: sender QoS snapshots are built without server stats

- [x] 2.2 Delete server-stats-specific compatibility state and helpers
  Files: `client/PlainClientSupport.*`, `client/PlainClientApp.h`, related helper headers
  Verification: `lossBase`, `LossCounterSource`, and cached server-stats control state are removed

- [x] 2.3 Source peer coordination decisions from local sender signals only
  Files: `client/PlainClientLegacy.cpp`, `client/PlainClientThreaded.cpp`
  Verification: budget/coordination paths no longer depend on server-derived observed bitrate

## 3. Validate

- [x] 3.1 Build affected targets
  Verification: changed native targets compile cleanly

- [x] 3.2 Run targeted tests
  Verification: targeted sender QoS-related tests pass

- [x] 3.3 Review trace source labels
  Verification: no plain-client runtime code path still formats sender control traces as `sample=server`
