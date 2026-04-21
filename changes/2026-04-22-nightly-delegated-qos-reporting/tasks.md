# Tasks

## 1. Parse Nested QoS Tasks

- [ ] 1.1 Extend nested nightly log parsing to keep delegated task status and duration
  Verification: raw log parsing returns usable delegated QoS rows

## 2. Render The Summary

- [ ] 2.1 Add delegated QoS task rendering to the nightly plain-text summary
  Verification: `--print-summary` output includes the section

- [ ] 2.2 Add delegated QoS task rendering to the nightly HTML summary and summary JSON
  Verification: the generated summary payload includes delegated task rows

## 3. Validate

- [ ] 3.1 Validate against an existing nightly run using `--skip-tests`
  Verification: delegated rows like `cpp-client-matrix` and `cpp-client-ab` appear
