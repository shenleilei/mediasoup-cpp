# Tasks

1. [x] Reindent `SenderTransportController` queue-flush helpers and `SendPacket()`
   Outcome:
   - The real control flow is obvious from the code layout
   Main files:
   - `client/SenderTransportController.h`
   Verification:
   - `git diff --check`

2. [x] Add audio deadline regression coverage
   Outcome:
   - There is a direct regression test for expired queued audio and `audioDeadlineDrops`
   Main files:
   - `tests/test_thread_model.cpp`
   Verification:
   - `./build/mediasoup_qos_unit_tests --gtest_filter='SenderTransportControllerTest.Audio*'`

3. [x] Harden prober listener shutdown
   Outcome:
   - Prober teardown waits for in-flight listener callbacks before returning
   Main files:
   - `client/ccutils/Prober.h`
   - `client/NetworkThread.h`
   - `specs/current/runtime-safety.md`
   Verification:
   - `./build/mediasoup_qos_unit_tests --gtest_filter='*Probe*'`
   - `./build/mediasoup_thread_integration_tests --gtest_filter='NetworkThreadIntegration.Probe*'`

4. [x] Verify and land the review closeout
   Outcome:
   - Targeted checks pass and the final fix set is committed cleanly
   Verification:
   - `git diff --check`
   - `./build/mediasoup_qos_unit_tests --gtest_filter='SenderTransportControllerTest.*:*Probe*'`
   - `./build/mediasoup_thread_integration_tests --gtest_filter='NetworkThreadIntegration.Probe*'`
