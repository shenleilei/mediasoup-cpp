# Design

## Context

`tests/qos_harness/run.mjs` currently carries its own copy of:

- `WsJsonClient`
- `defaultRtpCapabilities()`
- `joinRoom()`

Those same concepts already exist in `tests/qos_harness/ws_json_client.mjs`, which creates maintenance drift and raises the chance of inconsistent signaling setup.

## Proposed Approach

Make `ws_json_client.mjs` the single source of truth for:

- `WsJsonClient`
- `defaultRtpCapabilities()`
- `joinRoom()`
- `connectAndJoinRoom()`

Then remove the duplicated implementations from `run.mjs` and import the shared exports instead. Use the same module for the common connect-plus-join flow in other harness runners as well.

Add a separate shared module for local SFU process orchestration that provides:

- port allocation
- TCP readiness wait
- local SFU spawn with the common worker/IP/Redis defaults
- child shutdown

Add a separate shared module for browser harness bootstrap that provides:

- esbuild bundle creation
- local static server startup
- server URL discovery
- Puppeteer launch with overridable protocol timeout and args
- higher-level browser harness session startup
- inline-bundle page creation with shared diagnostics capture

## Alternatives Considered

- Keep both implementations and manually sync them
  Rejected: high drift risk and no real simplification

- Move all harness helpers into `run.mjs`
  Rejected: worse module boundaries and breaks existing reuse in other harness scripts

## Testing Strategy

- syntax check the migrated scripts
- import the shared module from `run.mjs` successfully
- run a lightweight harness scenario or script-level verification if the environment supports it
