# Full Regression Test Results

Generated at: `2026-04-27 19:13:57 CST`

## Summary

- Script: `scripts/run_all_tests.sh`
- Selected groups: `unit`, `integration`, `qos`, `topology`
- Overall status: `FAIL`
- Attempted tasks: `8`
- Passed tasks: `7`
- Failed tasks: `1`
- Failed groups: `qos`

## Failed Task Summary

| Task | Group | Duration |
|---|---|---|
| `qos:qos-regression` | `qos` | `7631s` |

## Task Results

| Task | Group | Status | Duration |
|---|---|---|---|
| `unit` | `unit` | `PASS` | `2s` |
| `integration:mediasoup_integration_tests` | `integration` | `PASS` | `18s` |
| `integration:mediasoup_e2e_tests` | `integration` | `PASS` | `5s` |
| `integration:mediasoup_stability_integration_tests` | `integration` | `PASS` | `6s` |
| `integration:mediasoup_review_fix_tests` | `integration` | `PASS` | `52s` |
| `qos:qos-regression` | `qos` | `FAIL` | `7631s` |
| `topology:mediasoup_topology_tests` | `topology` | `PASS` | `13s` |
| `topology:mediasoup_multinode_tests` | `topology` | `PASS` | `42s` |

## Task Duration View

| Task | Duration | Visual |
|---|---:|---|
| `unit` | `2s` | # |
| `integration:mediasoup_integration_tests` | `18s` | # |
| `integration:mediasoup_e2e_tests` | `5s` | # |
| `integration:mediasoup_stability_integration_tests` | `6s` | # |
| `integration:mediasoup_review_fix_tests` | `52s` | # |
| `qos:qos-regression` | `7631s` | #################### |
| `topology:mediasoup_topology_tests` | `13s` | # |
| `topology:mediasoup_multinode_tests` | `42s` | # |

## Detailed Reports

| Report | Scope | Link | Updated |
|---|---|---|---|
| Uplink Summary | Uplink QoS summary | [uplink-qos-test-results-summary.md](uplink-qos-test-results-summary.md) | 2026-04-27 10:27:18 |
| Uplink Cases | Browser uplink per-case report | [uplink-qos-case-results.md](uplink-qos-case-results.md) | 2026-04-27 19:12:57 |
| Plain Client Cases | PlainTransport C++ client per-case report | [plain-client-qos-case-results.md](plain-client-qos-case-results.md) | 2026-04-27 19:12:58 |
| Downlink Summary | Downlink QoS summary | [downlink-qos-test-results-summary.md](downlink-qos-test-results-summary.md) | 2026-04-27 19:12:58 |
| Downlink Cases | Downlink per-case report | [downlink-qos-case-results.md](downlink-qos-case-results.md) | 2026-04-27 19:12:58 |
| Uplink Matrix JSON | Latest browser uplink matrix artifact | [uplink-qos-matrix-report.json](generated/uplink-qos-matrix-report.json) | 2026-04-27 06:54:40 |
| Plain Client Matrix JSON | Latest C++ client matrix artifact | [uplink-qos-cpp-client-matrix-report.json](generated/uplink-qos-cpp-client-matrix-report.json) | 2026-04-27 18:07:36 |
| Downlink Matrix JSON | Latest downlink matrix artifact | [downlink-qos-matrix-report.json](generated/downlink-qos-matrix-report.json) | 2026-04-27 19:12:57 |
| LiveKit 48-Case Compare | Latest livekit-aligned 43-case comparison | - | - |
| TWCC A/B Eval | Latest TWCC A/B effectiveness report | [g0-vs-g2.md](../changes/2026-04-21-plain-client-sender-transport-control/artifacts/twcc-ab-eval/2026-04-21T06-39-19.638Z/g0-vs-g2.md) | 2026-04-21 14:47:21 |
