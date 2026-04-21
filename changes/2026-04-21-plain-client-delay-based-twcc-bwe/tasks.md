# Tasks

1. [x] Add structured TWCC feedback parsing
   Outcome:
   - detailed per-packet TWCC parse result exists
   - summary counters are derived from the detailed parse result
   Files:
   - `client/TransportCcHelpers.h`
   Verification:
   - `./build/mediasoup_qos_unit_tests --gtest_filter='TransportCcHelpers.*'`

2. [x] Introduce minimal delay-based BWE module and sent-packet history
   Outcome:
   - a dedicated estimator stores sent packet history and produces a bounded transport estimate from delay trend + acked bitrate
   Files:
   - `client/DelayBasedBwe.h`
   - `client/NetworkThread.h`
   Verification:
   - `./build/mediasoup_qos_unit_tests --gtest_filter='DelayBasedBwe*'`

3. [x] Integrate the estimator into `NetworkThread` pacing
   Outcome:
   - successful transport-cc sends are recorded
   - TWCC feedback updates the estimate through the new module
   Files:
   - `client/NetworkThread.h`
   - `tests/test_thread_integration.cpp`
   Verification:
   - `./build/mediasoup_thread_integration_tests --gtest_filter='NetworkThreadIntegration.TransportCc*'`

4. [x] Add deterministic regression coverage for phase-2 estimate behavior
   Outcome:
   - delay-growth decrease, stable-delay increase, clamp, malformed no-op, and target-grow bootstrap stay covered
   Files:
   - `tests/test_thread_model.cpp`
   - `tests/test_thread_integration.cpp`
   Verification:
   - `./build/mediasoup_qos_unit_tests --gtest_filter='TransportCcHelpers.*:DelayBasedBwe*'`
   - `./build/mediasoup_thread_integration_tests --gtest_filter='NetworkThreadIntegration.TransportCc*'`
