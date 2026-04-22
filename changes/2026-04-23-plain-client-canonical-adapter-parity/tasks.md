# Tasks

## 1. Define

- [x] 1.1 Define the plain-client transport parity fields and their ownership
  Verification: browser-only fields and plain-client local-equivalent fields are explicitly separated

## 2. Implement

- [x] 2.1 Export local sender-transport parity metrics from the threaded runtime
  Files: `client/sendsidebwe/*`, `client/NetworkThread.h`, `client/ThreadTypes.h`
  Verification: runtime stats carry explicit sender transport delay/jitter equivalents

- [x] 2.2 Add local sender limitation reason to the canonical snapshot path
  Files: `client/qos/QosTypes.h`, `client/PlainClientSupport.cpp`
  Verification: canonical snapshot can represent plain-client limitation semantics without overloading browser-only fields

- [x] 2.3 Update QoS signal derivation to consume canonical adapter fields
  Files: `client/qos/QosSignals.h`, related tests
  Verification: QoS derivation consumes sender limitation reason while keeping explicit sender transport timing fields available for observability

- [x] 2.4 Reject the unsafe direct timing merge experiment
  Files: `client/qos/QosSignals.h`, targeted matrix verification
  Verification: explicit sender transport timing fields remain exported, but they do not regress `J3` by directly overdriving the main QoS timing path

## 3. Validate

- [x] 3.1 Build and run targeted native tests
  Verification: native unit/integration coverage passes

- [x] 3.2 Re-run representative matrix cases
  Verification: `B2`, `BW1`, `R4`, and `J3` remain stable after the adapter change
