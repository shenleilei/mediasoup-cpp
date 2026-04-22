# Bugfix Design

## Context

This change intentionally backports two specific upstream worker fixes into the current repository baseline instead of attempting a full worker upgrade.

That choice is driven by the current upgrade cost:

- `3.19.18+` changes FBS/IP​​C contracts such as `WORKER_CLOSE`
- newer worker releases raise build/toolchain requirements
- the repository still carries local worker mirror/build patches

The goal here is to take the high-value fixes without triggering the larger worker-upgrade migration.

## Root Cause

### `3.19.9`

At the current baseline, RTCP cumulative loss handling is internally signed at the `ReceiverReport` boundary, but the value is still propagated through unsigned storage/schema fields in downstream worker stats and the top-level repo `rtpStream` schema.

That mismatch can corrupt the meaning of `packetsLost`, especially when cumulative loss decreases or would otherwise require signed representation.

### `3.19.15`

At the current baseline, `RtpStreamSend::ReceivePacket()` stores the packet into the retransmission buffer and updates transmission counters without first checking whether the same sequence is already present in the retransmission buffer.

That allows a duplicate original RTP packet to re-enter the send path.

## Fix Strategy

### 1. Backport the minimal signed-loss propagation changes

Apply the smallest compatible subset of the upstream `3.19.9` fix:

- change worker-side `packetsLost` / `reportedPacketLost` state to signed where needed
- update worker `rtpStream.fbs` to use `int32` for `packets_lost`
- update the top-level repo `fbs/rtpStream.fbs` to match
- regenerate top-level FlatBuffers headers
- add top-level regression coverage for JSON conversion of negative `packetsLost`

This change does not attempt a broad FBS upgrade. It only aligns the `rtpStream` schema element needed for this backport.

### 2. Backport the minimal duplicate-RTP suppression fix

Apply the focused `RtpStreamSend` hot-path fix:

- before accepting the packet into the normal send/retransmission path, query the retransmission buffer by sequence
- if the packet is already present, discard it immediately
- otherwise insert it and continue normal accounting

Also backport the focused upstream worker regression test for the duplicate-packet case.

### 3. Keep the change narrow

This backport does NOT:

- upgrade the worker tag
- adopt the `3.19.18+` FBS protocol migration
- change build scripts or worker mirror policy beyond what is required for these two fixes

## Risk Assessment

- The `rtpStream` schema change crosses the worker/control-plane boundary, so generated headers and local stats parsing must stay aligned.
- Vendored worker tests may require a worker-local build environment that is not fully available in this repo runtime.
- The duplicate-packet fix is in a hot RTP path, so it must avoid altering the normal retransmission path for non-duplicate packets.

## Test Strategy

- Add top-level unit coverage for signed `packetsLost` JSON conversion.
- Add vendored worker regression coverage for duplicate packet discard in `TestRtpStreamSend.cpp`.
- Run targeted top-level unit tests that exercise the changed JSON/statistics path.
- Attempt targeted vendored worker build/test execution if the local environment supports it; if blocked, record the blocker explicitly.

## Observability

- No new logs or metrics are required for the top-level repo.
- The worker duplicate-packet path already has debug-level context in the upstream fix pattern; avoid broad logging changes.

## Rollout Notes

- The runtime benefit of the worker-side changes only applies when using a worker binary built from the patched vendored source, or when equivalent patches are carried into the deployed worker artifact.
- To make the patched worker the default local runtime artifact, the repository setup flow should build the vendored worker and expose it at `./mediasoup-worker`.
- Rollback is straightforward:
  - revert the vendored worker source changes
  - revert the top-level `rtpStream` schema/header/test changes together
