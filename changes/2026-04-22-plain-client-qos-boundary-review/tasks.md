# Tasks

## 1. Reproduce

- [x] 1.1 Capture the main plain-client QoS code/principle findings from the current investigation
  Verification: findings are mapped to concrete local files

## 2. Fix

- [x] 2.1 Write a durable plain-client QoS boundary review document
  Files: `docs/qos/plain-client/plain-client-qos-boundary-review_cn.md`
  Verification: document covers the highest-confidence risks first and distinguishes them from lower-confidence audit targets

- [x] 2.2 Add a discoverability link from the plain-client QoS status page
  Files: `docs/qos/plain-client/README.md`
  Verification: the new review document is reachable from the plain-client QoS doc entrypoint

## 3. Validate

- [x] 3.1 Run `git diff --check`
  Verification: changed docs are clean

- [x] 3.2 Manual read-through
  Verification: the new document reads as a maintainer review memo rather than chat notes
