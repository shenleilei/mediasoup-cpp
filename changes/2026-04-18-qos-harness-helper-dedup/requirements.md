# Requirements

## Summary

Refactor the Node-based QoS harness scripts so duplicated WebSocket client, RTP capability, and room-join helpers are centralized in the shared `ws_json_client.mjs` module.

## Business Goal

Developers need the Node QoS harness scripts to share one source of truth for connection and join behavior so changes to signaling setup do not drift between runner scripts.

## In Scope

- `tests/qos_harness/run.mjs`
- `tests/qos_harness/ws_json_client.mjs`
- `tests/qos_harness/sfu_process.mjs`
- `tests/qos_harness/browser_harness.mjs`
- light migration of other harness scripts that already import `ws_json_client.mjs` where needed

## Out Of Scope

- changing QoS scenario behavior
- browser-side harness logic
- SFU runtime behavior
- restructuring scenario JSON files

## Acceptance Criteria

### Requirement 1

The system SHALL centralize the shared Node WebSocket client and join helpers in `ws_json_client.mjs`.

#### Scenario: harness room join

- WHEN a harness script needs to connect and join a room with the default RTP capabilities
- THEN it SHALL be able to use the shared exported helper instead of duplicating that logic locally

### Requirement 2

The system SHALL centralize identical connect-plus-join flows in the Node QoS harness.

#### Scenario: runner session setup

- WHEN a harness script needs to create a WebSocket client, connect, and then join a room
- THEN it SHALL be able to use one shared helper for that sequence
### Requirement 3

The system SHALL preserve existing QoS harness behavior.

#### Scenario: migrated run script

- WHEN `run.mjs` and related runner scripts are migrated to the shared helpers
- THEN it SHALL preserve existing scenario execution semantics and error handling

### Requirement 4

The system SHALL centralize repeated SFU child-process startup helpers in the Node QoS harness where practical.

#### Scenario: browser harness SFU launch

- WHEN a harness script needs to allocate a port, launch a local SFU, wait for readiness, and stop it
- THEN it SHALL be able to use one shared helper module instead of duplicating the same process boilerplate

### Requirement 5

The system SHALL centralize repeated browser harness bootstrap helpers where practical.

#### Scenario: browser QoS runner setup

- WHEN a harness script needs to build a browser bundle, serve it over a local static server, and launch Puppeteer
- THEN it SHALL be able to use shared helper code instead of duplicating the same bootstrap steps

## Non-Functional Requirements

- Maintainability: one shared implementation for Node harness join behavior
- Compatibility: existing script entrypoints remain unchanged
- Reliability: signaling setup behavior remains deterministic
