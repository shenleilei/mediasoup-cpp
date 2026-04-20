# Tasks

1. [completed] Add RTP delta helper and switch recorder audio/video PTS calculation to widened arithmetic.
   - Files: `src/Recorder.h`, `src/Recorder.cpp`
   - Verify: build recorder-linked test targets

2. [completed] Add regression tests for signed-boundary crossing and wraparound.
   - Files: `tests/test_qos_unit.cpp`
   - Verify: `./build/mediasoup_qos_unit_tests`

3. [completed] Run required suites and record outcomes.
   - Verify: `./build/mediasoup_qos_unit_tests`, `./build/mediasoup_tests`
