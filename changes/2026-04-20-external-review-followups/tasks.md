# Tasks

## 1. Fix Shared Media And FFmpeg Correctness

- [x] 1.1 Correct H264 FU-A and RTP header parsing edge cases
  Files: `common/media/rtp/H264Packetizer.cpp`, `common/media/rtp/RtpHeader.h`, `tests/test_common_media.cpp`
  Verification: build `mediasoup_tests` and run `ctest --test-dir build --output-on-failure -R '^mediasoup_tests$'`

- [x] 1.2 Harden FFmpeg wrapper trailer, allocation, and backpressure handling
  Files: `common/ffmpeg/AvPtr.h`, `common/ffmpeg/OutputFormat.*`, `common/ffmpeg/Decoder.*`, `common/ffmpeg/Encoder.*`, `common/ffmpeg/BitstreamFilter.*`, affected tests
  Verification: build the affected test targets and run `ctest --test-dir build --output-on-failure -R 'mediasoup_tests|mediasoup_qos_recording_accuracy_tests|mediasoup_review_fix_tests'`

## 2. Fix Room/QoS Lifecycle And Registry Correctness

- [x] 2.1 Clear both peer and room-pressure override records on reconnect and leave
  Files: `src/RoomServiceLifecycle.cpp`, `src/RoomServiceQos.cpp`, `src/RoomServiceDownlink.cpp`, `src/RoomStatsQosHelpers.h`, affected tests
  Verification: run `ctest --test-dir build --output-on-failure -R 'mediasoup_tests|mediasoup_qos_unit_tests|mediasoup_review_fix_tests'`

- [x] 2.2 Harden stale cached remote redirects and normalize startup locking
  Files: `src/RoomRegistry.cpp`, related registry tests
  Verification: run `ctest --test-dir build --output-on-failure -R 'mediasoup_tests|mediasoup_multinode_tests|mediasoup_review_fix_tests'`

## 3. Clean Touched Boundary Debt

- [x] 3.1 Remove dead registry wrapper and keep the stats-store response helper single-sourced
  Files: `src/RoomService.h`, `src/RoomServiceRegistryView.cpp`, `src/RoomServiceQos.cpp`, `src/RoomServiceDownlink.cpp`, `src/RoomStatsQosHelpers.h`
  Verification: rerun the affected unit/integration targets from Tasks 1 and 2

## 4. Guard Against Regression

- [x] 4.1 Add or extend regression coverage for every confirmed fixed path
  Verification:
  - H264 FU-A F-bit preservation is covered
  - RTP version/padding parsing is covered
  - FFmpeg trailer/backpressure/allocation failure paths are covered
  - reconnect/leave room-pressure override cleanup is covered
  - stale cached remote redirect validation is covered

## 5. Delivery Gates

- [x] 5.1 Run the final targeted build/test set
  Verification:
  - `cmake --build build --target mediasoup-sfu mediasoup_tests mediasoup_qos_unit_tests mediasoup_multinode_tests mediasoup_review_fix_tests mediasoup_qos_recording_accuracy_tests`
  - `ctest --test-dir build --output-on-failure -R 'mediasoup_tests|mediasoup_qos_unit_tests|mediasoup_multinode_tests|mediasoup_review_fix_tests|mediasoup_qos_recording_accuracy_tests'`

- [x] 5.2 Review `docs/aicoding/DELIVERY_CHECKLIST.md`
  Verification: all applicable items are resolved or recorded explicitly

## 6. Review

- [x] 6.1 Run self-review using `docs/aicoding/REVIEW.md`
  Verification: scope remains limited to confirmed items, regression risk is covered, and deferred external-review debt is still explicit

## 7. Knowledge Update

- [x] 7.1 Update `specs/current/` only if implementation proves an accepted-behavior gap
  Verification: no doc drift remains between final code and accepted behavior
