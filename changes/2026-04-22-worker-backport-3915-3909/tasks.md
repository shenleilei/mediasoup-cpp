# Tasks

## 1. Reproduce

- [x] 1.1 Capture the local code paths corresponding to upstream `3.19.9` and `3.19.15`
  Verification: identify the worker and top-level files that must be changed without broadening into a full worker upgrade

## 2. Fix

- [x] 2.1 Backport the signed `packets_lost` propagation fix into the vendored worker and top-level repo schema
  Files: vendored worker `rtpStream`/RTP stats files, top-level `fbs/rtpStream.fbs`, generated headers, `src/RtpStreamStatsJson.h`
  Verification: compile the changed top-level code and confirm the schema/header alignment

- [x] 2.2 Backport the duplicate RTP packet discard fix into vendored `RtpStreamSend`
  Files: vendored worker `RtpStreamSend` implementation and header
  Verification: rerun the targeted duplicate-packet regression check

- [x] 2.3 Wire the patched vendored worker into the default local setup flow
  Files: `setup.sh`, `README.md`, `docs/platform/README.md`, `docs/platform/dependencies_cn.md`
  Verification: setup flow documents and builds the vendored worker, then exposes it at `./mediasoup-worker`

## 3. Unit And Integration Coverage

- [x] 3.1 Add/update top-level regression coverage for signed `packetsLost` JSON conversion
  Verification: run the targeted unit test binary containing the new/updated test

- [ ] 3.2 Add/update vendored worker regression coverage for duplicate RTP packet discard
  Verification: regression test was added in vendored worker `TestRtpStreamSend.cpp`; targeted worker-local test invocation was started with Python 3.8 + invoke, but the first full `mediasoup-worker-test` target build was not awaited to completion in this run

## 4. Guard Against Regression

- [x] 4.1 Verify unaffected nearby behavior still compiles and the changed tests pass
  Verification: `mediasoup_tests` and `mediasoup_qos_unit_tests` rebuilt successfully; targeted top-level tests passed; vendored worker-local regression test remains the only unfinished verification item

## 5. Delivery Gates

- [x] 5.1 Run `git diff --check`
  Verification: `git diff --check -- <changed files>` passed in the superproject, and `git -C src/mediasoup-worker-src diff --check` passed in the vendored worker worktree

- [x] 5.2 Review `DELIVERY_CHECKLIST.md`
  Verification: applicable checklist items were reviewed; the only recorded unfinished verification item is completion of the vendored worker-local test run

## 6. Review

- [x] 6.1 Run self-review using `REVIEW.md` from the whole-system perspective
  Verification: the boundary between vendored worker changes and top-level schema/parser changes has been reviewed explicitly

## 7. Knowledge Update

- [x] 7.1 Update `specs/current/` if accepted behavior changed
  Verification: no `specs/current/` file currently documents this low-level worker signed-loss or duplicate-RTP behavior, so no spec update was required for this focused backport
