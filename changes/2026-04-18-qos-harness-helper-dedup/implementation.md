# QoS Harness Helper Dedup Implementation

## Objective

This implementation document records how the Node QoS harness scripts are being refactored so duplicated client, SFU-process, and browser-bootstrap helpers are centralized without changing scenario behavior.

## Scope

### In Scope

- `tests/qos_harness/ws_json_client.mjs`
- `tests/qos_harness/sfu_process.mjs`
- `tests/qos_harness/browser_harness.mjs`
- migrated harness runners under `tests/qos_harness/*.mjs`

### Out Of Scope

- SFU runtime behavior
- browser bundle application logic
- scenario JSON definitions
- C++ runtime or test behavior

## Execution Strategy

1. Move one repeated helper family at a time into a dedicated shared module.
2. Migrate a representative subset of scripts first.
3. Run `node --check` after each migration wave.
4. Expand to the remaining scripts once the helper shape is proven.

## Shared Modules

### `ws_json_client.mjs`

Owns:

- `WsJsonClient`
- `defaultRtpCapabilities()`
- `joinRoom()`
- `connectAndJoinRoom()`

### `sfu_process.mjs`

Owns:

- `allocatePort()`
- `waitForPort()`
- `startLocalSfu()`
- `stopChild()`

### `browser_harness.mjs`

Owns:

- `buildBrowserBundle()`
- `startStaticServer()`
- `getServerUrl()`
- `launchBrowser()`

## Migration Waves

### Wave 1: WebSocket Client And Join Dedup

Status: completed

Migrated scripts:

- `tests/qos_harness/run.mjs`
- `tests/qos_harness/run_cpp_client_harness.mjs`

### Wave 2: SFU Process Helper Dedup

Status: completed

Migrated scripts:

- `tests/qos_harness/run.mjs`
- `tests/qos_harness/browser_downlink_v2.mjs`
- `tests/qos_harness/browser_server_signal.mjs`
- `tests/qos_harness/browser_downlink_e2e.mjs`
- `tests/qos_harness/browser_downlink_controls.mjs`
- `tests/qos_harness/browser_downlink_priority.mjs`
- `tests/qos_harness/browser_downlink_v3.mjs`
- `tests/qos_harness/browser_capacity_rooms.mjs`
- `tests/qos_harness/run_downlink_matrix.mjs`
- `tests/qos_harness/cpp_client_runner.mjs`

### Wave 3: Browser Bootstrap Helper Dedup

Status: completed

Migrated scripts:

- `tests/qos_harness/browser_downlink_v2.mjs`
- `tests/qos_harness/browser_downlink_e2e.mjs`
- `tests/qos_harness/browser_downlink_controls.mjs`
- `tests/qos_harness/browser_server_signal.mjs`
- `tests/qos_harness/browser_downlink_v3.mjs`
- `tests/qos_harness/browser_downlink_priority.mjs`
- `tests/qos_harness/browser_capacity_rooms.mjs`
- `tests/qos_harness/run_downlink_matrix.mjs`
- `tests/qos_harness/browser_loopback.mjs`
- `tests/qos_harness/browser_synthetic_sweep.mjs`
- `tests/qos_harness/loopback_runner.mjs`

## Verification

Completed checks:

- `node --check tests/qos_harness/ws_json_client.mjs`
- `node --check tests/qos_harness/sfu_process.mjs`
- `node --check tests/qos_harness/browser_harness.mjs`
- `node --check tests/qos_harness/run.mjs`
- `node --check tests/qos_harness/run_cpp_client_harness.mjs`
- `node --check tests/qos_harness/browser_downlink_v2.mjs`
- `node --check tests/qos_harness/browser_server_signal.mjs`
- `node --check tests/qos_harness/browser_downlink_e2e.mjs`
- `node --check tests/qos_harness/browser_downlink_controls.mjs`
- `node --check tests/qos_harness/browser_downlink_priority.mjs`
- `node --check tests/qos_harness/browser_downlink_v3.mjs`
- `node --check tests/qos_harness/browser_capacity_rooms.mjs`
- `node --check tests/qos_harness/run_downlink_matrix.mjs`
- `node --check tests/qos_harness/browser_loopback.mjs`
- `node --check tests/qos_harness/browser_synthetic_sweep.mjs`
- `node --check tests/qos_harness/loopback_runner.mjs`

## Current Result

The Node QoS harness now has shared sources of truth for:

- WebSocket signaling setup
- local SFU child-process orchestration
- browser bundle and local static-server bootstrap

This reduces script-local duplication and makes future harness changes much cheaper to apply consistently.
