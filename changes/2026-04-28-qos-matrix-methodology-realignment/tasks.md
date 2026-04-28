# Tasks

## 1. Methodology Freeze

- [ ] 1.1 Audit and document all current implicit transformations in browser/cpp matrix runners.
  Files: `tests/qos_harness/run_matrix.mjs`, `tests/qos_harness/run_cpp_client_matrix.mjs`, related helpers
  Verification: transformation inventory recorded in design notes or follow-up docs

- [ ] 1.2 Add explicit legacy labeling to current matrix entrypoints and reports.
  Outcome: current mixed-method runners are no longer mistaken for physical-signoff suites
  Verification: docs / report headers updated

- [ ] 1.3 Freeze the legacy mixed-method runners from further semantic expansion.
  Outcome: future changes stop adding new implicit shaping to legacy paths
  Verification: design/maintenance rule documented

## 2. Schema Design

- [ ] 2.1 Define separate physical-case and synthetic-case schemas.
  Outcome: parameter semantics are explicit and non-overlapping
  Verification: schema proposal documented

- [ ] 2.2 Define the Phase-2 physical baseline schema for `B1~B5`.
  Outcome: first physical suite has a concrete case contract
  Verification: field list and examples documented

## 3. Runner Split Plan

- [ ] 3.1 Define browser runner split plan, with Phase-2 focusing on a physical baseline entrypoint.
  Outcome: one physical E2E path and optional synthetic regression path
  Verification: design finalized

- [ ] 3.2 Define cpp runner split plan.
  Outcome: `run_cpp_client_matrix` responsibilities decomposed into physical and synthetic variants
  Verification: design finalized

- [ ] 3.3 Define Phase-2 browser physical runner rules.
  Outcome: no RTT amplification, no bandwidth folding, no synthetic stat shaping
  Verification: explicit rule list documented

## 4. Migration Plan

- [ ] 4.1 Plan phased migration starting with `B1~B5`.
  Outcome: baseline cases become the first physical-signoff family
  Verification: ordered migration tasks documented

- [ ] 4.2 Plan artifact/report family migration.
  Outcome: legacy, physical, and synthetic outputs are distinguishable
  Verification: artifact strategy documented

- [ ] 4.3 Define Phase-2 physical baseline report contract.
  Outcome: configured / applied / observed layers are explicit
  Verification: report field plan documented

## 5. Verification Strategy

- [ ] 5.1 Define how physical and synthetic suites will each be validated after migration.
  Outcome: acceptance checks are explicit before implementation starts
  Verification: commands / evidence expectations documented
