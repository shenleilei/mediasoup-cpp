# Threaded Client Follow-ups

## Summary

The threaded plain-client path still has three confirmed behavior gaps:

- `NetworkThread::sendComediaProbe()` only emits probe RTP for the first registered video track
- `PlainClientApp::InstallSignalHandlers()` still uses `signal()` instead of `sigaction()`
- threaded peer QoS snapshots use a function-local `static` sequence counter, which leaks state across repeated runs in the same process

## Reproduction

1. Review `client/NetworkThread.h`, `client/PlainClientApp.cpp`, and `client/PlainClientThreaded.cpp`.
2. Compare `NetworkThread::sendComediaProbe()` against the legacy threaded-agnostic probe loop in `client/PlainClientLegacy.cpp`.
3. Observe that the threaded path sends probe RTP only for `tracks_[0]`.
4. Observe that `InstallSignalHandlers()` still calls `signal(SIGINT, ...)` and `signal(SIGPIPE, ...)`.
5. Observe that threaded `clientStats` snapshots increment a function-local `static int peerSnapshotSeq`.

## Observed Behavior

- multi-track threaded publishing probes only the first video SSRC before media starts flowing
- signal handler installation uses weaker `signal()` semantics
- repeated threaded runs in one process reuse the previous peer snapshot sequence baseline

## Expected Behavior

- threaded `comedia` probing SHOULD cover every registered video track SSRC, matching the legacy path's first-packet behavior more closely
- signal handlers SHOULD be installed with `sigaction()`
- threaded peer snapshot sequences SHOULD reset per `PlainClientApp` instance/run rather than leaking across process-local reruns

## Known Scope

- `client/NetworkThread.h`
- `client/PlainClientApp.cpp`
- `client/PlainClientApp.h`
- `client/PlainClientThreaded.cpp`
- `tests/test_thread_integration.cpp`

## Must Not Regress

- existing single-track threaded publishing behavior
- existing threaded QoS control and ack plumbing
- plain-client stop/shutdown behavior
- current legacy-mode behavior

## Suspected Root Cause

Confidence: high.

These are narrow follow-up defects left after the larger plain-client split:

- the threaded path kept only the first-track probe logic instead of the legacy all-track loop
- signal handling was carried forward unchanged from the original implementation
- the peer snapshot sequence counter stayed as a local static convenience variable instead of per-run state

## Acceptance Criteria

### Requirement 1

Threaded `comedia` probing SHALL emit probe RTP for every registered video track.

#### Scenario: two registered video tracks

- WHEN `NetworkThread::sendComediaProbe()` runs with multiple registered tracks
- THEN each registered track SSRC SHALL emit probe RTP packets

### Requirement 2

Plain-client signal handlers SHALL be installed with `sigaction()`.

#### Scenario: signal handler installation

- WHEN `PlainClientApp::InstallSignalHandlers()` runs
- THEN it SHALL configure `SIGINT` and `SIGPIPE` using `sigaction()`

### Requirement 3

Threaded peer snapshot sequence numbering SHALL reset per app run.

#### Scenario: repeated threaded runs in one process

- WHEN the threaded path starts a new `PlainClientApp` run
- THEN peer snapshot sequence numbering SHALL start from a fresh per-instance baseline

## Regression Expectations

- existing threaded plain-publish integration behavior remains working
- new regression coverage is added for multi-track probe SSRC coverage
- build/tests continue to pass for the threaded integration target and affected unit target
