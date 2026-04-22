# Tasks

## 1. Define The Change

- [x] 1.1 Capture the intent for a durable worker-upgrade assessment document
  Files: `changes/2026-04-22-mediasoup-worker-upgrade-assessment/requirements.md`
  Verification: requirements describe baseline, bugfix, patch, and upgrade-risk coverage

## 2. Design The Documentation Change

- [x] 2.1 Define the assessment document structure and scope boundaries
  Files: `changes/2026-04-22-mediasoup-worker-upgrade-assessment/design.md`
  Verification: design explains document location, sections, and source basis

## 3. Write The Assessment

- [x] 3.1 Add a platform document summarizing current worker baseline, upstream fixes, upgrade blockers, and local patch carry-forward decisions
  Files: `docs/platform/mediasoup-worker-upgrade-assessment_cn.md`
  Verification: document covers the discussed `3.19.9`, `3.19.15`, protocol/FBS drift, build/tooling changes, and patch recommendations

- [x] 3.2 Add a discoverability link from the platform docs index
  Files: `docs/platform/README.md`
  Verification: the new assessment document is reachable from the existing platform docs entrypoint

## 4. Delivery Gates

- [ ] 4.1 Run `git diff --check`
  Verification: full-repo command currently fails on pre-existing EOF blank-line issues in `docs/generated/twcc-ab-g0-vs-g2.md`, `docs/generated/twcc-ab-g1-vs-g2.md`, and `docs/generated/twcc-ab-report.md`; targeted `git diff --check -- <changed files>` passed for this change

- [x] 4.2 Manual read-through
  Verification: the document reads as a maintainer guide rather than a raw chat transcript
