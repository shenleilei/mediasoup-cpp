# Bugfix Design

## Context

Wave 5 is the final cleanup of production test injection.

The architectural direction is already fixed:

- sender QoS runtime uses real local transport truth
- server observation stays out of the sender QoS control chain
- the production binary should not contain synthetic runtime mutation hooks

## Root Cause

Earlier harness development embedded test orchestration directly into the plain-client process because it was the fastest way to cover:

- synthetic sender snapshots
- synthetic signaling requests
- forced stale stats
- synthetic encoding-command rejection

That was useful during bring-up, but it leaves the production runtime carrying test-only control paths that are now below the quality bar.

## Fix Strategy

### 1. Delete production test env parsing

Remove `QOS_TEST_*` parsing and state from:

- `PlainClientApp`
- `PlainClientSupport`
- `PlainClientThreaded`
- `SourceWorker`

### 2. Keep only externally driven plain-client runtime coverage

For the plain-client harness surface:

- keep scenarios that use real runtime behavior
- drive weak-network conditions with external loopback netem
- drop scenarios that only existed to inject synthetic `clientStats` or self-issued signaling from inside the production client process

### 3. Move responsibility boundaries back to the right layer

- server-side `clientStats` storage / stale-seq / auto-override semantics remain covered by server integration tests and JS/node harnesses
- plain-client harness focuses on real threaded runtime behavior and transport-driven adaptation

## Risk Assessment

- The plain-client harness surface becomes narrower; docs and scripts must be updated together so removed scenarios are not still advertised as active coverage.
- Real netem-driven adaptation can be noisier than synthetic profile injection; only externally meaningful scenarios should remain on the default plain-client surface.

## Test Strategy

- build `plain-client`
- build `mediasoup_qos_unit_tests`
- build `mediasoup_thread_integration_tests`
- run targeted sender QoS unit tests
- run `node --check` on edited harness scripts
- confirm `rg "QOS_TEST_" client` is empty

## Rollout Notes

- This wave intentionally removes synthetic plain-client harness coverage that depended on in-process env mutation.
- Server-side QoS semantics remain covered elsewhere; they are no longer claimed as plain-client runtime coverage unless exercised through real runtime paths.
