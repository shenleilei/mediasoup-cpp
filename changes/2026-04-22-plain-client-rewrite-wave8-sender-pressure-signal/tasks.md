# Tasks

## 1. Reproduce

- [ ] 1.1 Confirm remaining weak-network misses are due to missing sender-side pressure in the outer loop
  Verification: targeted traces show queue/pacing pressure before outer-loop transitions

## 2. Fix

- [ ] 2.1 Add explicit sender-side bandwidth pressure to snapshot types
  Files: `client/ThreadTypes.h`, `client/qos/QosTypes.h`, related declarations
  Verification: snapshot pipeline carries sender pressure without overloading browser-only fields

- [ ] 2.2 Populate sender-side pressure from the transport inner loop
  Files: `client/NetworkThread.h`, `client/PlainClientSupport.cpp`
  Verification: threaded runtime snapshots preserve the sender pressure signal

- [ ] 2.3 Consume sender-side pressure in outer-loop signal derivation
  Files: `client/qos/QosSignals.h`, tests
  Verification: `bandwidthLimited` becomes true when explicit sender pressure is present

## 3. Validate

- [ ] 3.1 Build and unit-test the changed logic
  Verification: native builds and targeted signal tests pass

- [ ] 3.2 Re-run default-timing targeted matrix cases
  Verification: `B2`, `BW1`, `R4`, and `J3` are rechecked
