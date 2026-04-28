## Tasks

- [x] Update the six GCC-misaligned expectations in `tests/qos_harness/scenarios/sweep_cases.json`
  - Verification: inspect the changed case definitions

- [x] Update any affected documentation that describes the synthetic suite's interpretation of these cases
  - Verification: doc review against the new expectations

- [x] Re-run the 43 targeted `cpp-client` QoS matrix cases
  - Verification: `node tests/qos_harness/run_cpp_client_matrix.mjs --cases=...`

- [x] Confirm the targeted report shows all 43 cases passing
  - Verification: inspect `docs/generated/uplink-qos-cpp-client-matrix-report.targeted.json`
