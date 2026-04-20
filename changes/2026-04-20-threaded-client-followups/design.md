# Design

## Context

This wave intentionally fixes only three narrow client correctness items and avoids broader client refactors.

The affected code is already localized:

- `NetworkThread` owns threaded RTP send-side probing
- `PlainClientApp` owns signal installation
- `PlainClientThreaded` owns peer snapshot sequencing

## Root Cause

- `sendComediaProbe()` was simplified to use only `tracks_[0]`, diverging from the legacy all-track loop
- signal handling was left on the older `signal()` API
- peer snapshot sequencing used a function-local static for convenience, which becomes hidden process-global state

## Fix Strategy

### 1. Probe every registered video track

Keep the probe payload trivial, but loop across all registered tracks:

- emit the same small dummy RTP packet pattern
- preserve the current per-track sequence increment behavior
- keep the existing short inter-packet sleep so the transport still learns the remote tuple before normal media starts

No bandwidth/QoS logic changes are needed; this is only about transport/SSRC visibility.

### 2. Replace `signal()` with `sigaction()`

Install handlers explicitly with `sigaction()`:

- `SIGINT` should point to `PlainClientApp::OnSignal`
- `SIGPIPE` should be ignored

This is a narrow implementation cleanup with no intended behavior change beyond safer signal semantics.

### 3. Move peer snapshot sequence to per-instance state

Add a small `PlainClientApp` member for the threaded peer snapshot sequence counter and use it where the threaded path currently increments the local static.

This preserves monotonic sequencing within one run while resetting naturally for each app instance.

## Risk Assessment

- probing all tracks slightly increases the initial dummy packet count, but only during startup
- `sigaction()` wiring must preserve the previous observable signal behavior
- the new peer snapshot sequence member must only affect the threaded path and should not leak into legacy mode logic

## Test Strategy

- add a focused integration test in `tests/test_thread_integration.cpp` that captures UDP probe packets and verifies both SSRCs appear
- run the threaded integration target and the main unit target after the client changes

## Observability

- existing startup logs are sufficient
- no new production diagnostics are required

## Rollout Notes

- client-side only
- no server contract or accepted-spec change is expected
