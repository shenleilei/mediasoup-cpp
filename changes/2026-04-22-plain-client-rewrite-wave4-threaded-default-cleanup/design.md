# Bugfix Design

## Context

Wave 4 is the cleanup step after the runtime cutover.

The architectural direction is already fixed:

- sender QoS runs only on the threaded runtime
- non-copy plain-client execution is the default threaded execution mode
- legacy fallback signaling for sender QoS is not part of the target architecture

## Root Cause

Wave 3 removed the behavioral dependency on a threaded opt-in flag, but several transitional artifacts survived:

- redundant `threadedMode_` state inside `PlainClientApp`
- setup and harness env examples still exporting `PLAIN_CLIENT_THREADED=1`
- a fallback scenario still encoding the old "threaded request but legacy execution" model
- session bootstrap still instantiating transitional controllers before the real threaded runtime controller set exists

These leftovers create instruction drift and encourage future code to preserve an obsolete split-brain model.

## Fix Strategy

### 1. Remove redundant runtime state

Simplify `PlainClientApp` so the remaining runtime decision is expressed directly in terms of `copyMode_` and explicit input availability, without storing a second derived threaded-mode flag.

### 2. Remove bootstrap-only controller setup

Do not create sender QoS controllers during session bootstrap. Keep notification delivery configured, but delay notification dispatch until the final threaded runtime controller set exists so initial `qosPolicy` / `qosOverride` notifications land on the authoritative controller instances.

### 3. Remove obsolete threaded opt-in references

Update setup examples, harnesses, and integration tests so the default non-copy runtime is used without `PLAIN_CLIENT_THREADED=1`.

### 4. Replace fallback-oriented regression coverage

Delete or rewrite coverage that still expects a legacy fallback warning. Keep the same multi-track intent, but validate the final contract:

- explicit per-track sources run directly on the threaded path
- multi-track default non-copy startup uses the threaded sender QoS path

## Risk Assessment

- Any external scripts outside this repo that still set `PLAIN_CLIENT_THREADED=1` will keep working only as a no-op assumption; this wave does not preserve that flag as a documented contract.
- Renaming/removing fallback scenarios can break local habits if docs are not updated together.

## Test Strategy

- build `plain-client`
- build `mediasoup_qos_unit_tests`
- run targeted sender QoS unit tests
- run `node --check` on edited harness scripts
- run targeted search to confirm repo-local removal of `PLAIN_CLIENT_THREADED` references from supported docs/scripts

## Rollout Notes

- This wave intentionally removes transitional wording and tests that described the old split runtime model.
- The remaining legacy runtime is only the copy-mode/plain send path, not a sender QoS fallback architecture.
