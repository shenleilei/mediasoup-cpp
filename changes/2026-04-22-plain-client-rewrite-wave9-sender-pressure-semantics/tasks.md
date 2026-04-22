# Tasks

## 1. Define

- [x] 1.1 Define the sender-pressure semantic and where it belongs in the inner loop
  Verification: semantic and ownership are documented before code changes

## 2. Implement

- [x] 2.1 Add sender-pressure state to transport/runtime snapshot types
  Files: `client/ThreadTypes.h`, `client/qos/QosTypes.h`
  Verification: snapshot pipeline can carry the semantic end to end

- [x] 2.2 Compute sender-pressure inside the transport/runtime layer
  Files: `client/NetworkThread.h`, related helpers
  Verification: sender-pressure is derived from inner-loop facts, not outer-loop heuristics

- [x] 2.3 Consume sender-pressure in outer-loop derivation
  Files: `client/PlainClientSupport.cpp`, `client/qos/QosSignals.h`, tests
  Verification: outer-loop `bandwidthLimited` uses the new semantic

- [x] 2.4 Stop dropping queued fresh video on generation switch
  Files: `client/NetworkThread.h`, `tests/test_thread_integration.cpp`
  Verification: downgrade actions no longer create artificial loss via fresh-queue drops

- [x] 2.5 Align async `confirmAction()` probe startup with the JS controller path
  Files: `client/qos/QosController.h`, `tests/test_thread_integration.cpp`
  Verification: async confirm starts and completes recovery probes with the intended parity timing

- [x] 2.6 Remove implicit default warmup from the plain-client controller runtime
  Files: `client/qos/QosController.h`, `tests/test_client_qos.cpp`
  Verification: default controller reacts without the old hidden startup delay

- [x] 2.7 Remove probe-driven local sample suppression from threaded runtime
  Files: `client/ThreadedControlHelpers.h`, `tests/test_thread_model.cpp`
  Verification: probe activity no longer suppresses local sampling windows

## 3. Validate

- [x] 3.1 Build and test targeted logic
  Verification: builds and targeted signal tests pass

- [x] 3.2 Re-run default-timing representative cases
  Verification: `B2`, `BW1`, `R4`, and `J3` are rechecked

- [x] 3.3 Re-run full QoS unit coverage after the controller/runtime parity changes
  Verification: `./build/mediasoup_qos_unit_tests`
