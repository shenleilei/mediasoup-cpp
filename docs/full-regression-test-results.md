# Full Regression Test Results

Generated at: `2026-04-19 23:01:14 CST`

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
| `qos:qos-regression` | `qos` | `7329s` |

## Task Results

| Task | Group | Status | Duration |
|---|---|---|---|
| `unit` | `unit` | `PASS` | `2s` |
| `integration:mediasoup_integration_tests` | `integration` | `PASS` | `17s` |
| `integration:mediasoup_e2e_tests` | `integration` | `PASS` | `5s` |
| `integration:mediasoup_stability_integration_tests` | `integration` | `PASS` | `5s` |
| `integration:mediasoup_review_fix_tests` | `integration` | `PASS` | `46s` |
| `qos:qos-regression` | `qos` | `FAIL` | `7329s` |
| `topology:mediasoup_topology_tests` | `topology` | `PASS` | `12s` |
| `topology:mediasoup_multinode_tests` | `topology` | `PASS` | `39s` |
