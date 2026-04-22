# Bugfix Analysis

## Summary

After wave 6:

- threaded sender QoS runtime has a dedicated runtime object
- `PlainClientApp` is reduced to bootstrap/session ownership

The remaining architectural baggage is the old copy-mode entry still living in the main `plain-client` binary:

- `--copy` still selects a separate legacy runtime
- `PlainClientLegacy.cpp` remains part of the main build
- bootstrap code still carries fallback-to-copy behavior and copy-mode-only state

This wave removes the legacy copy-mode entry from the main `plain-client` binary so the product surface matches the final threaded-only architecture.

## Reproduction

1. Inspect `PlainClientApp::ParseArguments()` and observe `--copy`.
2. Inspect `PlainClientApp::Run()` and observe `RunCopyMode()`.
3. Inspect `client/CMakeLists.txt` and observe `PlainClientLegacy.cpp` still linked into `plain-client`.

## Observed Behavior

- main `plain-client` binary still exposes a second non-QoS runtime mode
- bootstrap code still carries copy-mode fallback behavior
- architecture docs still describe `--copy` as a supported main-path mode

## Expected Behavior

- main `plain-client` SHALL only expose the threaded runtime entry
- startup failures in threaded prerequisites SHALL fail fast instead of falling back to copy mode
- legacy copy-mode runtime SHALL no longer be part of the main `plain-client` build

## Known Scope

- `client/PlainClientApp.*`
- `client/PlainClientThreaded.cpp`
- `client/ThreadedQosRuntime.*`
- `client/CMakeLists.txt`
- delete `client/PlainClientLegacy.cpp`
- setup / runtime docs and harness launcher

## Must Not Regress

- threaded publish startup
- explicit source startup without MP4 bootstrap
- audio worker precondition handling
- threaded harness / unit / integration buildability

## Acceptance Criteria

### Requirement 1

Main `plain-client` SHALL become threaded-only.

#### Scenario: CLI startup

- WHEN `plain-client` starts
- THEN it does not accept or route to `--copy`
- AND `Run()` always delegates to the threaded runtime

### Requirement 2

Legacy copy-mode code SHALL exit the main build surface.

#### Scenario: build wiring

- WHEN `plain-client` is built
- THEN `PlainClientLegacy.cpp` is not linked into the binary

## Regression Expectations

- Required automated verification:
  - build affected native targets
  - run targeted sender QoS unit tests
  - run representative threaded harness scenarios
  - run a smoke check that `PLAIN_CLIENT_VIDEO_SOURCES` no longer creates copy-mode semantics in the main binary
