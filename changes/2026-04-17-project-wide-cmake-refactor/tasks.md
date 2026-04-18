# Tasks

## 1. Planning

- [x] 1.1 Define the repository-wide build refactor scope and non-goals
  Files: `changes/2026-04-17-project-wide-cmake-refactor/*`
  Verification: requirements and design clearly preserve runtime behavior

## 2. Shared Build Helpers

- [x] 2.1 Add a reusable CMake helper module for common target configuration
  Files: `cmake/*`
  Verification: root and client builds can consume the helper without duplicated setup blocks

## 3. Root Project Refactor

- [x] 3.1 Refactor top-level target declarations to use centralized include and link definitions
  Files: `CMakeLists.txt`
  Verification: fresh configure succeeds and target names remain unchanged

- [x] 3.2 Refactor gtest target registration to use a consistent helper path
  Files: `CMakeLists.txt`
  Verification: `ctest -N` shows the expected registered tests

## 4. Client Build Refactor

- [x] 4.1 Reuse the shared helper path for the standalone plain client build
  Files: `client/CMakeLists.txt`
  Verification: fresh client configure and build succeed

## 5. Delivery Gates

- [x] 5.1 Build representative top-level targets from a fresh build directory
  Verification: `cmake -S . -B ...`, `cmake --build ... --target mediasoup-sfu mediasoup_tests mediasoup_qos_integration_tests`

- [x] 5.2 Verify test registration and run the core unit suite
  Verification: `ctest -N` shows 11 registered tests; `ctest --output-on-failure -R '^mediasoup_tests$' -j1` still hits a current-source segfault in `OwnedNotificationTest.GarbageDataReturnsNull`

- [x] 5.3 Build the standalone plain client target
  Verification: `cmake -S client -B ...`, `cmake --build ... --target plain-client`

## 6. Review

- [x] 6.1 Self-review the change against project review guidance
  Verification: diff checked for compatibility, duplication removal, delivery checklist, and doc drift
