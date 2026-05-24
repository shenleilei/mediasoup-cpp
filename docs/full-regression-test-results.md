# Full Regression Test Results

Generated at: `2026-04-28 12:45:10 CST`

## Summary

- Script: `scripts/run_all_tests.sh`
- Selected groups: `unit`, `integration`, `qos`, `topology`
- Overall status: `FAIL`
- Attempted tasks: `8`
- Passed tasks: `6`
- Failed tasks: `2`
- Failed groups: `integration`, `qos`

## Failed Task Summary

| Task | Group | Duration |
|---|---|---|
| `integration:mediasoup_review_fix_tests` | `integration` | `60s` |
| `qos:qos-regression` | `qos` | `9134s` |

## Task Results

| Task | Group | Status | Duration |
|---|---|---|---|
| `unit` | `unit` | `PASS` | `2s` |
| `integration:mediasoup_integration_tests` | `integration` | `PASS` | `18s` |
| `integration:mediasoup_e2e_tests` | `integration` | `PASS` | `5s` |
| `integration:mediasoup_stability_integration_tests` | `integration` | `PASS` | `6s` |
| `integration:mediasoup_review_fix_tests` | `integration` | `FAIL` | `60s` |
| `qos:qos-regression` | `qos` | `FAIL` | `9134s` |
| `topology:mediasoup_topology_tests` | `topology` | `PASS` | `13s` |
| `topology:mediasoup_multinode_tests` | `topology` | `PASS` | `42s` |

## Task Duration View

| Task | Duration | Visual |
|---|---:|---|
| `unit` | `2s` | # |
| `integration:mediasoup_integration_tests` | `18s` | # |
| `integration:mediasoup_e2e_tests` | `5s` | # |
| `integration:mediasoup_stability_integration_tests` | `6s` | # |
| `integration:mediasoup_review_fix_tests` | `60s` | # |
| `qos:qos-regression` | `9134s` | #################### |
| `topology:mediasoup_topology_tests` | `13s` | # |
| `topology:mediasoup_multinode_tests` | `42s` | # |

## Detailed Reports

| Report | Scope | Link | Updated |
|---|---|---|---|
| Uplink Summary | Uplink QoS summary | [uplink-qos-test-results-summary.md](uplink-qos-test-results-summary.md) | 2026-04-28 06:54:24 |
| Uplink Cases | Browser uplink per-case report | [uplink-qos-case-results.md](uplink-qos-case-results.md) | 2026-04-28 12:44:11 |
| Downlink Summary | Downlink QoS summary | [downlink-qos-test-results-summary.md](downlink-qos-test-results-summary.md) | 2026-04-28 12:44:11 |
| Downlink Cases | Downlink per-case report | [downlink-qos-case-results.md](downlink-qos-case-results.md) | 2026-04-28 12:44:11 |
| Uplink Matrix JSON | Latest browser uplink matrix artifact | [uplink-qos-matrix-report.json](generated/uplink-qos-matrix-report.json) | 2026-04-28 12:38:31 |
| Downlink Matrix JSON | Latest downlink matrix artifact | `generated/downlink-qos-matrix-report.json` is generated on demand and is not retained in the current tree | 2026-04-28 12:44:10 |
