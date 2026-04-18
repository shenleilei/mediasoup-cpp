# Tasks

## 1. Planning

- [x] 1.1 Define the Node QoS harness helper dedup scope
  Verification: requirements and design match the intended script-only refactor

## 2. Shared Helper Consolidation

- [x] 2.1 Make `ws_json_client.mjs` the shared source of truth for client and join helpers
  Files: `tests/qos_harness/ws_json_client.mjs`
  Verification: exports remain usable by migrated scripts

- [x] 2.2 Add a shared connect-plus-join helper for Node harness runners
  Files: `tests/qos_harness/ws_json_client.mjs`
  Verification: migrated runners can replace repeated setup boilerplate with the shared helper

- [x] 2.3 Add a shared local-SFU process helper module for Node harness scripts
  Files: `tests/qos_harness/*`
  Verification: migrated scripts can replace repeated port allocation / start / wait / stop boilerplate with the shared helper

- [x] 2.4 Add a shared browser harness bootstrap helper module
  Files: `tests/qos_harness/*`
  Verification: migrated browser runners can replace repeated bundle/static server/browser launch boilerplate with the shared helper

- [x] 2.5 Add higher-level browser session and inline-page helpers
  Files: `tests/qos_harness/*`
  Verification: migrated runners can replace repeated browser/page bootstrap code with the shared helper

- [x] 2.4 Add a shared browser harness bootstrap helper module
  Files: `tests/qos_harness/*`
  Verification: migrated browser runners can replace repeated bundle/static server/browser launch boilerplate with the shared helper

## 3. Script Migration

- [x] 3.1 Remove duplicated client/join helpers from `run.mjs` and import the shared module
  Files: `tests/qos_harness/run.mjs`
  Verification: script syntax remains valid and imports resolve

- [x] 3.2 Migrate runner setup callsites to the shared connect-plus-join helper
  Files: `tests/qos_harness/run.mjs`, `tests/qos_harness/run_cpp_client_harness.mjs`
  Verification: script syntax remains valid and imports resolve

- [x] 3.3 Migrate representative harness scripts to the shared local-SFU process helper
  Files: `tests/qos_harness/run.mjs`, `tests/qos_harness/browser_downlink_v2.mjs`, `tests/qos_harness/browser_server_signal.mjs`, `tests/qos_harness/browser_downlink_e2e.mjs`, `tests/qos_harness/browser_downlink_controls.mjs`, `tests/qos_harness/browser_downlink_priority.mjs`, `tests/qos_harness/browser_downlink_v3.mjs`, `tests/qos_harness/browser_capacity_rooms.mjs`, `tests/qos_harness/run_downlink_matrix.mjs`, `tests/qos_harness/cpp_client_runner.mjs`
  Verification: script syntax remains valid and imports resolve

- [x] 3.4 Migrate representative browser runners to the shared browser harness bootstrap helper
  Files: `tests/qos_harness/browser_downlink_v2.mjs`, `tests/qos_harness/browser_downlink_e2e.mjs`, `tests/qos_harness/browser_downlink_controls.mjs`, `tests/qos_harness/browser_server_signal.mjs`, `tests/qos_harness/browser_downlink_v3.mjs`, `tests/qos_harness/browser_capacity_rooms.mjs`, `tests/qos_harness/run_downlink_matrix.mjs`, `tests/qos_harness/browser_downlink_priority.mjs`
  Verification: script syntax remains valid and imports resolve

- [x] 3.4 Migrate representative browser runners to the shared browser harness bootstrap helper
  Files: `tests/qos_harness/browser_downlink_v2.mjs`, `tests/qos_harness/browser_downlink_e2e.mjs`, `tests/qos_harness/browser_downlink_controls.mjs`, `tests/qos_harness/browser_server_signal.mjs`, `tests/qos_harness/browser_downlink_v3.mjs`, `tests/qos_harness/browser_downlink_priority.mjs`, `tests/qos_harness/browser_capacity_rooms.mjs`, `tests/qos_harness/run_downlink_matrix.mjs`, `tests/qos_harness/browser_loopback.mjs`, `tests/qos_harness/browser_synthetic_sweep.mjs`, `tests/qos_harness/loopback_runner.mjs`
  Verification: script syntax remains valid and imports resolve

- [x] 3.5 Migrate representative browser runners to the higher-level session/page helpers
  Files: `tests/qos_harness/browser_downlink_v2.mjs`, `tests/qos_harness/browser_downlink_e2e.mjs`, `tests/qos_harness/browser_downlink_controls.mjs`, `tests/qos_harness/browser_server_signal.mjs`, `tests/qos_harness/browser_downlink_v3.mjs`, `tests/qos_harness/browser_capacity_rooms.mjs`, `tests/qos_harness/browser_downlink_priority.mjs`
  Verification: script syntax remains valid and imports resolve

## 4. Delivery Gates

- [x] 4.1 Run script-level verification
  Verification: `node --check tests/qos_harness/run.mjs`, `node --check tests/qos_harness/ws_json_client.mjs`, `node --check tests/qos_harness/run_cpp_client_harness.mjs`, `node --check tests/qos_harness/browser_downlink_v2.mjs`, `node --check tests/qos_harness/browser_server_signal.mjs`, `node --check tests/qos_harness/browser_downlink_e2e.mjs`, `node --check tests/qos_harness/browser_downlink_controls.mjs`, `node --check tests/qos_harness/browser_downlink_priority.mjs`, `node --check tests/qos_harness/browser_downlink_v3.mjs`, `node --check tests/qos_harness/browser_capacity_rooms.mjs`, `node --check tests/qos_harness/run_downlink_matrix.mjs`, `node --check tests/qos_harness/browser_loopback.mjs`, `node --check tests/qos_harness/browser_synthetic_sweep.mjs`, `node --check tests/qos_harness/loopback_runner.mjs`, `node --check tests/qos_harness/sfu_process.mjs`, `node --check tests/qos_harness/browser_harness.mjs`
