## Tasks

- [x] Add `GD1-GD12` to `tests/qos_harness/scenarios/sweep_cases.json` on the current branch
  - Verification: inspect the scenario catalog diff

- [x] Update the gate-facing uplink QoS docs that currently describe a 43-case default gate
  - Verification: doc review against the new scenario count and coverage

- [x] Run targeted `cpp-client` verification for `GD1-GD12`
  - Verification: `node tests/qos_harness/run_cpp_client_matrix.mjs --cases=GD1,...,GD12`

- [x] Run targeted browser verification for `GD1-GD12`
  - Verification: browser matrix targeted rerun for the same cases

- [x] Record the resulting expanded gate status and any residual failures
  - Verification: generated reports and updated docs remain consistent
