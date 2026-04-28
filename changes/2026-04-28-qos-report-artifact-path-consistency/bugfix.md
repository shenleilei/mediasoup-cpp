# Bugfix Analysis

## Summary

QoS report artifact paths are not fully consistent across the plain-client C++ matrix toolchain:

- `scripts/run_qos_tests.sh` writes the targeted plain-client case markdown to a different filename than the Node artifact-path helper expects.
- entry documentation links labeled as "targeted rerun" still point to full-report artifacts in a few high-traffic docs.

## Reproduction

1. Inspect `tests/qos_harness/cpp_client_report_artifacts.mjs`.
2. Inspect the plain-client case report output section in `scripts/run_qos_tests.sh`.
3. Compare the generated files under `docs/` and `docs/generated/` after a targeted `cpp-client-matrix` run.
4. Inspect `docs/README.md` and nearby plain-client QoS index docs for targeted artifact links.

## Observed Behavior

- The shell runner emits targeted plain-client markdown as `docs/generated/uplink-qos-cpp-client-case-results.targeted.md`.
- The helper module and render entrypoint expect `docs/generated/plain-client-qos-case-results.targeted.md`.
- As a result, targeted markdown generation, default path inference, archive helpers, and human-facing docs are not using one canonical path.
- Some docs say "targeted rerun" but still link to the full artifact path.

## Expected Behavior

- Plain-client report artifact paths SHALL be defined once and consumed consistently by shell scripts, Node helpers, renderers, archives, and docs.
- Targeted artifact links SHALL point to targeted files, not the full-report files.

## Known Scope

- `scripts/run_qos_tests.sh`
- `tests/qos_harness/cpp_client_report_artifacts.mjs`
- `tests/qos_harness/render_cpp_client_case_report.mjs`
- `tests/qos_harness/test.report_artifacts.mjs`
- selected docs under `docs/` that index current artifact locations

## Must Not Regress

- Full plain-client matrix report paths.
- Browser uplink and downlink report path isolation.
- Existing archive layout for already-generated runs.

## Suspected Root Cause

- Path definitions were introduced in more than one place, and the shell script kept an older targeted markdown filename after the Node helper had moved to the `plain-client-qos-case-results.targeted.md` naming convention.
- Documentation was only partially updated when targeted/full report isolation was introduced.

## Acceptance Criteria

### Requirement 1

The system SHALL use one canonical targeted plain-client case-report markdown path.

#### Scenario: Targeted plain-client matrix run

- WHEN `run_qos_tests.sh` generates a targeted plain-client case report
- THEN the output path matches `cpp_client_report_artifacts.mjs`
- AND renderers and archive helpers infer the same path by default

### Requirement 2

The system SHALL keep full and targeted report paths isolated in tests.

#### Scenario: Artifact path helper test

- WHEN the artifact helper resolves full and targeted plain-client paths
- THEN the full markdown path is `docs/plain-client-qos-case-results.md`
- AND the targeted markdown path is `docs/generated/plain-client-qos-case-results.targeted.md`

### Requirement 3

Human-facing entry docs SHALL link targeted labels to targeted artifacts.

#### Scenario: README artifact index

- WHEN a line is labeled as a targeted rerun artifact
- THEN its markdown link points to a targeted JSON or targeted markdown file

## Regression Expectations

- Existing unaffected behavior: browser uplink full/targeted paths, downlink full/targeted paths, archive root handling.
- Required automated regression coverage: artifact-path helper tests and targeted render default-path checks.
- Required manual smoke checks: inspect current plain-client full and targeted files under `docs/` and `docs/generated/`.
