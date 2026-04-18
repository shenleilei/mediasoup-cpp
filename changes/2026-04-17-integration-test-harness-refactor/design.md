# Design

## Context

Multiple integration-oriented test files duplicate the same SFU control flow:

- build a `./build/mediasoup-sfu --nodaemon` command line
- choose Redis and worker settings
- launch with `popen()`
- wait for the TCP port to accept connections
- terminate the process and wait for the port to become free
- generate unique room prefixes with `getpid()` and `steady_clock`

Several files also repeat the same default audio/video RTP capability JSON fixture.
Several suites also repeat the same single-encoding Opus and VP8 producer RTP parameter JSON.

## Proposed Approach

Add a shared helper header under `tests/` that provides:

- `SfuLaunchOptions` for configurable startup arguments
- `startSfuProcess()`, `waitForTcpPort()`, and `waitForTcpPortFree()` helpers
- `stopSfuProcessAndWait()` to preserve existing termination behavior
- `makeUniqueTestName()` for suite-local room prefixes
- `makeDefaultRtpCapabilities()` for the standard Opus + VP8 fixture
- `makeAudioProducerRtpParameters()` and `makeVideoProducerRtpParameters()` for the standard single-encoding producer fixtures

Add a shared HTTP test helper header under `tests/` that provides:

- full HTTP response reads with timeout handling
- simple HTTP GET helpers for raw response or body extraction

Extend the shared SFU harness helpers to provide:

- default join request helpers for standard RTP capability fixtures
- connect-plus-join helpers for standard RTP capability fixtures
- common send and recv transport request helpers
- common standard audio/video produce helpers
- common publisher/subscriber auto-subscribe setup helpers

Add a shared media test helper header under `tests/` that provides:

- common Opus codec fixtures
- loopback UDP socket setup helpers
- reusable audio-opus producer RTP parameter builders for non-WebSocket media tests

Refactor representative suites to call the shared helper rather than inlining their own process logic. The migration should include both:

- single-node suites
- multi-node suites with Redis-backed startup

## Alternatives Considered

- Alternative: introduce a complex inheritance-heavy base fixture hierarchy
- Why it was rejected: many suites have different helper methods and state; forcing them under one class would create tight coupling

- Alternative: leave helpers local to each file and only copy a newer pattern around
- Why it was rejected: it does not actually reduce drift or maintenance cost

## Module Boundaries

- `tests/TestProcessUtils.h`: can continue to own low-level process liveness and termination helpers
- new helper header: owns higher-level SFU launch, wait, and common fixture data
- migrated test files: keep scenario-specific logic and only delegate shared setup pieces

## Data And Interfaces

`SfuLaunchOptions` will expose:

- port
- workers
- worker binary path
- announced and listen IPs
- Redis host and port
- optional `maxRoutersPerWorker`
- optional extra CLI args
- optional shell redirection target

Shared JSON helpers remain internal to test code only.

## Control Flow

1. Test fixture builds `SfuLaunchOptions`
2. Shared helper launches `mediasoup-sfu`
3. Shared helper waits for the configured TCP port
4. Test fixture runs its existing scenario logic
5. Shared helper terminates the process and waits for the port to become reusable

## Failure Handling

- launch helper returns `-1` on startup failure so fixtures can assert explicitly
- wait helpers return `false` on timeout so fixtures can fail with suite-specific messages
- shutdown helper remains tolerant of already-dead processes

## Security Considerations

- none; test-only process-control refactor

## Observability

- centralizing startup/shutdown makes launch failures easier to diagnose consistently
- test files become shorter and clearer about scenario logic versus infrastructure logic

## Testing Strategy

- build migrated test targets
- run representative single-node and multi-node integration test binaries
- verify that startup and teardown still succeed with their expected ports and Redis settings

## Rollout Notes

- no migration outside the test tree
- rollback is limited to the new helper header and migrated test files
