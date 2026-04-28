# Tasks

## 1. Define Consistent Empty-State Semantics

- [x] 1.1 Guard operational FFmpeg adapter methods against empty or moved-from state
  Files: `common/ffmpeg/InputFormat.*`, `common/ffmpeg/OutputFormat.*`, `common/ffmpeg/Decoder.*`, `common/ffmpeg/Encoder.*`, `common/ffmpeg/BitstreamFilter.*`
  Verification: `cmake --build build --target mediasoup_tests -- -j2`

## 2. Add Regression Coverage

- [x] 2.1 Extend adapter unit tests for default and moved-from object misuse
  Files: `tests/test_common_ffmpeg.cpp`
  Verification: `ctest --test-dir build --output-on-failure -R '^mediasoup_tests$'`

## 3. Delivery Gates

- [x] 3.1 Run the final targeted build/test set
  Verification:
  - `cmake --build build --target mediasoup_tests -- -j2`
  - `ctest --test-dir build --output-on-failure -R '^mediasoup_tests$'`

- [x] 3.2 Review `docs/aicoding/DELIVERY_CHECKLIST.md`
  Verification: all applicable items are resolved or recorded explicitly

## 4. Review

- [x] 4.1 Run self-review using `docs/aicoding/REVIEW.md`
  Verification: empty-state semantics are consistent, scoped, and regression-covered
