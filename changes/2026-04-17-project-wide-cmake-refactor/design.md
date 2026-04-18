# Design

## Context

The repository's CMake configuration currently repeats the same include directories, FFmpeg libraries, pthread linkage, and `add_test()` boilerplate across many targets. The duplication spans the main project and the standalone plain client build, which increases the cost and risk of routine maintenance.

## Proposed Approach

Introduce a shared helper module under `cmake/` that provides:

- interface libraries for common repository includes and common FFmpeg/runtime link groups
- small helper functions for configuring project-owned executables
- a helper for registering gtest-based test targets with a consistent working directory

Refactor the top-level `CMakeLists.txt` to consume those helpers for:

- `mediasoup_lib`
- `mediasoup-sfu`
- unit tests and integration tests
- the benchmark target

Refactor `client/CMakeLists.txt` to reuse the same helper module for the plain client target where applicable.

## Alternatives Considered

- Alternative: keep everything in one large `CMakeLists.txt` and only add local variables
- Why it was rejected: it reduces some repetition but still leaves the project without reusable conventions across build entrypoints

- Alternative: move every target into separate subdirectories immediately
- Why it was rejected: too broad for a first repository-wide refactor and would increase risk without improving behavior proportionally

## Module Boundaries

- `cmake/ResolveFFmpeg.cmake`: remains focused on FFmpeg header discovery
- new shared helper module: owns common project target wiring and reusable link groups
- root `CMakeLists.txt`: declares repository targets using the shared helpers
- `client/CMakeLists.txt`: declares the plain client target using the shared helpers

## Data And Interfaces

New CMake interfaces are internal to the build system only. They are expected to expose:

- common include directories for project-owned code
- common runtime library sets for FFmpeg and threading
- helper functions for executable and gtest target registration

No runtime or user-facing API changes are introduced.

## Control Flow

1. Configure project-level variables and third-party targets
2. Include the shared helper module
3. Declare reusable interface libraries from resolved paths and external libraries
4. Define project targets through the helper functions
5. Register tests through the helper function so `ctest` behavior remains unchanged

## Failure Handling

- helper functions should require an explicit target name and source list
- targets with non-standard dependencies should keep adding those dependencies explicitly at the callsite
- FFmpeg discovery failure behavior remains owned by `ResolveFFmpeg.cmake`

## Security Considerations

- none; build-system-only refactor

## Observability

- target setup becomes easier to audit because common dependencies are named and centralized
- reduced duplication lowers the chance of silent drift between similar targets

## Testing Strategy

- configure the top-level project in a fresh build directory with tests enabled
- build representative top-level targets including the main binary, unit tests, and at least one integration-style test binary
- list registered `ctest` tests to verify registration survived the refactor
- configure and build the standalone plain client target
- run at least the core unit test suite if the environment supports it

## Rollout Notes

- no migration needed; rollback is limited to the new helper module and the touched CMake files
