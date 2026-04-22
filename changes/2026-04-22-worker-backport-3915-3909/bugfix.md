# Bugfix Analysis

## Summary

The current repository baseline still carries two upstream worker defects that are high-value for this project:

- the `3.19.9` RTCP `packets_lost` signedness/statistics fix is missing
- the `3.19.15` duplicate RTP packet suppression in `RtpStreamSend` is missing

These defects matter for this repository because it relies heavily on:

- worker stats correctness for QoS and reporting
- RTP retransmission / weak-network behavior in send-side media paths

## Reproduction

### `3.19.9` statistics defect

1. Inspect the local worker and top-level FBS definitions.
2. Observe that `RTCP::ReceiverReport::GetTotalLost()` is signed, but `packetsLost` / `reportedPacketLost` propagation and top-level `fbs/rtpStream.fbs` still use unsigned types.
3. Observe that negative or descending cumulative loss can be misrepresented across the worker -> control-plane stats boundary.

### `3.19.15` duplicate RTP defect

1. Inspect local `RtpStreamSend::ReceivePacket()`.
2. Observe that when the retransmission buffer exists, the packet is stored and transmission counters are updated without first checking whether the same sequence is already buffered.
3. Observe that the upstream fix explicitly discards already-buffered duplicate RTP packets before they re-enter the send path.

## Observed Behavior

- RTCP loss statistics can be propagated with the wrong signedness across the worker and top-level FlatBuffers schema boundary.
- The local `RtpStreamSend` path can continue handling a duplicate original RTP packet instead of short-circuiting it before it contaminates retransmission/bandwidth-estimation behavior.

## Expected Behavior

- Worker and control-plane stats SHALL preserve signed RTCP cumulative loss semantics where required.
- Duplicate RTP packets already present in the retransmission buffer SHALL be discarded before they are counted and resent as fresh packets.

## Known Scope

- `src/mediasoup-worker-src/worker/include/RTC/RtpStream.hpp`
- `src/mediasoup-worker-src/worker/include/RTC/RtpStreamRecv.hpp`
- `src/mediasoup-worker-src/worker/include/RTC/RtxStream.hpp`
- `src/mediasoup-worker-src/worker/src/RTC/RtpStreamRecv.cpp`
- `src/mediasoup-worker-src/worker/src/RTC/RtpStreamSend.cpp`
- `src/mediasoup-worker-src/worker/src/RTC/RtxStream.cpp`
- `src/mediasoup-worker-src/worker/fbs/rtpStream.fbs`
- `src/mediasoup-worker-src/worker/test/src/RTC/TestRtpStreamSend.cpp`
- top-level `fbs/rtpStream.fbs`
- generated top-level FlatBuffers headers under `generated/`
- `src/RtpStreamStatsJson.h`
- unit tests under `tests/`

## Must Not Regress

- current worker control-plane IPC outside the targeted `rtpStream` schema update
- unaffected RTP retransmission behavior
- existing stats JSON field names
- current build and test entrypoints outside this bugfix scope

## Suspected Root Cause

High confidence:

- `3.19.9`: signed RTCP cumulative loss semantics were fixed in the RTCP reader path, but local worker/storage/schema propagation still used unsigned representations at this baseline.
- `3.19.15`: local `RtpStreamSend` accepts packets into the retransmission path without checking whether the same sequence is already buffered.

## Acceptance Criteria

### Requirement 1

The repository SHALL preserve signed RTCP cumulative loss semantics across the worker and top-level stats schema boundary.

#### Scenario: negative or descending RTCP cumulative loss

- WHEN the worker reports cumulative loss with signed semantics
- THEN the local worker code stores it with signed types where required
- AND the top-level `rtpStream` FlatBuffers schema can represent that value correctly
- AND JSON conversion does not reinterpret the value as a huge unsigned number

### Requirement 2

The repository SHALL discard duplicate original RTP packets in `RtpStreamSend` once the packet is already buffered for retransmission.

#### Scenario: repeated packet with same sequence

- WHEN `RtpStreamSend` receives a packet whose sequence is already present in the retransmission buffer
- THEN the packet is discarded before it is re-counted and re-sent as a fresh packet

## Regression Expectations

- Existing unaffected behavior:
  - current stats JSON keys remain unchanged
  - current retransmission/NACK path remains functional
- Required automated regression coverage:
  - top-level unit coverage for signed `packetsLost` JSON conversion
  - vendored worker regression coverage for duplicate RTP packet discard
- Required manual smoke checks:
  - inspect generated `rtpStream` FlatBuffers headers after regeneration
  - confirm targeted worker/build commands still compile the touched code paths if environment permits
