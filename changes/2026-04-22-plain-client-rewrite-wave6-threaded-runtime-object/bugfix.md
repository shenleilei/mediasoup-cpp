# Bugfix Analysis

## Summary

After wave 5:

- production plain-client runtime no longer reads `QOS_TEST_*`
- the remaining sender QoS path is the real threaded runtime

The next architectural mismatch is ownership and structure:

- `PlainClientThreaded.cpp` still contains the entire threaded runtime lifecycle in one large function
- queue plumbing, worker lifecycle, QoS controller orchestration, peer coordination, and `clientStats` upload all live inside `PlainClientApp::RunThreadedMode()`
- `PlainClientApp` therefore still owns runtime details that should belong to a dedicated threaded runtime object

This wave extracts a real threaded runtime object so `PlainClientApp` returns to bootstrap/session ownership only.

## Reproduction

1. Inspect `PlainClientApp::RunThreadedMode()`.
2. Observe that it directly:
   - creates transport/control queues
   - starts/stops network/audio/source threads
   - instantiates per-track QoS controllers
   - drives the sampling/coordination loop
   - uploads `clientStats`

## Observed Behavior

- threaded runtime behavior is correct, but it is structurally coupled to `PlainClientApp`
- runtime state has no dedicated owner separate from bootstrap/session state
- future changes to sampling, coordination, or lifecycle still require editing one large function

## Expected Behavior

- threaded sender QoS runtime SHALL have a dedicated runtime object
- `PlainClientApp` SHALL only bootstrap session state and hand execution to that runtime object
- runtime-owned counters, queues, and loop state SHALL move off `PlainClientApp`

## Known Scope

- `client/PlainClientApp.*`
- `client/PlainClientThreaded.cpp`
- new threaded runtime implementation files
- `client/CMakeLists.txt`
- affected architecture docs

## Must Not Regress

- threaded media publish pipeline
- audio worker startup behavior
- per-track controller actions and ack handling
- multi-track coordination and `clientStats` upload

## Acceptance Criteria

### Requirement 1

Threaded sender QoS runtime SHALL have a dedicated owner object.

#### Scenario: threaded startup

- WHEN non-copy plain-client enters threaded mode
- THEN `PlainClientApp::RunThreadedMode()` only delegates to a runtime object
- AND the runtime object owns queue/thread/controller lifecycle

### Requirement 2

Runtime loop state SHALL move out of `PlainClientApp`.

#### Scenario: runtime state ownership

- WHEN reviewing `PlainClientApp`
- THEN peer snapshot sequencing and threaded sampling loop state are not stored there

## Regression Expectations

- Required automated verification:
  - build affected native targets
  - run targeted sender QoS unit tests
  - run threaded harness smoke after extraction
