# Design

## Context

Phase 1 established the transport-control boundary and TWCC wire-up, but the estimate path still looked only at aggregate lost-vs-total counts. For phase 2 we need the missing inner-loop data path:

- sender writes transport-wide sequence numbers
- sender records send time and packet size per sequence
- feedback parser reconstructs packet receive timestamps
- estimator compares receive timing against send timing and updates a pacing estimate

This is still intentionally smaller than libwebrtc GCC. The goal is a reviewable delay-based estimator that is directionally correct and testable.

## High-Level Design

Add a small `DelayBasedBwe` module with three responsibilities:

1. own sent-packet history for TWCC sequence numbers
2. consume parsed TWCC feedback and build correlated observations
3. maintain a bounded transport estimate from:
   - delay trend classification
   - acknowledged bitrate samples
   - startup/bootstrap rules

`NetworkThread` remains the owner of UDP send execution. It records each successfully sent transport-cc packet in `DelayBasedBwe` and feeds parsed TWCC feedback back into the estimator.

## Detailed TWCC Parsing

`TransportCcHelpers.h` will grow a detailed parse path:

- packet metadata:
  - sender SSRC
  - media SSRC
  - base sequence number
  - packet status count
  - reference time
  - feedback packet count
- per-packet observations:
  - sequence number
  - received/lost flag
  - reconstructed receive time in microseconds for received packets

Implementation notes:

- reference time is expanded as `referenceTime * 64000us`
- small and large deltas are parsed as signed values in 250us units
- receive timestamps are reconstructed cumulatively across received packets only
- the existing summary helper becomes a thin derivation over the detailed parse result

## Sent-Packet History

The estimator stores a compact ring indexed by transport-wide sequence number:

- `sequence`
- `sendTimeUs`
- `sizeBytes`
- `valid`

Rules:

- only successfully sent transport-cc packets are recorded
- unsent or rewrite-failed packets never enter history
- on successful feedback correlation, entries may be invalidated once consumed
- sequence wrap is handled by ring overwrite plus exact-sequence match

## Delay Trend Model

For each acknowledged packet correlated with history:

- `sendDeltaUs = send_i - send_{i-1}`
- `recvDeltaUs = recv_i - recv_{i-1}`
- `delayDeltaUs = recvDeltaUs - sendDeltaUs`
- `accumulatedDelayUs += delayDeltaUs`
- `smoothedDelayUs = alpha * accumulatedDelayUs + (1-alpha) * prevSmoothedDelayUs`

A bounded sliding window of `(recvTimeUs, smoothedDelayUs)` points is kept. When the window is large enough, a simple linear-regression slope is computed.

Classification:

- slope above positive threshold => `Overusing`
- slope below negative threshold => `Underusing`
- otherwise => `Normal`

This is intentionally a simplified trendline detector rather than a full libwebrtc port.

## Acknowledged Bitrate

The estimator also keeps a recent deque of acknowledged packets over a fixed receive-time window.

From that deque it computes:

- `ackedBitrateBps = sum(ackedBytes) / deltaRecvTime`

This gives a throughput anchor for estimate movement.

## Estimate Update Rules

Estimate update inputs:

- current estimate
- trendline state (`Overusing`, `Normal`, `Underusing`)
- current `ackedBitrateBps`
- configured min/max bounds

Rules:

- bootstrap:
  - if the estimate is zero, seed it from aggregate target bitrate if present, otherwise from the first valid acknowledged bitrate, then clamp
- overuse:
  - decrease estimate toward a guarded fraction of acknowledged bitrate
  - if no acknowledged bitrate is available, apply a bounded multiplicative decrease
- normal:
  - keep the estimate stable unless the acknowledged bitrate and trendline justify a small additive increase
- underuse:
  - allow the same additive increase path, slightly more willing than `Normal`
- aggregate-target growth:
  - keep the existing phase-1 safeguard that avoids permanent pinning when the application target grows from an app-limited state, but only as bootstrap assistance rather than the primary control signal

## Integration

`NetworkThread` changes:

- instantiate `DelayBasedBwe`
- on successful transport-cc RTP send, record `(sequence, sendTimeUs, sizeBytes)`
- on RTCP transport-cc feedback, parse detailed feedback and feed it into `DelayBasedBwe`
- update `transportEstimatedBitrateBps_` from the estimator output
- continue to publish the estimate into `SenderTransportController`

`SenderTransportController` does not change scheduling policy. It continues to use:

`effectivePacingBitrate = min(aggregateTarget, transportEstimate, applicationCap)`

The improvement in phase 2 is that `transportEstimate` is now delay-driven rather than loss-summary-driven.

## Verification Strategy

Unit coverage:

- detailed TWCC parsing reconstructs expected sequence/timestamp observations
- trendline overuse causes estimate decrease
- stable delay plus acknowledgements causes increase
- clamp-to-min and malformed feedback no-op hold

Integration coverage:

- `NetworkThread` feeds parsed feedback into the estimator and surfaces estimate change through the existing public getter
- aggregate-target bootstrap regression stays covered

## Deferred Work

- active probe clusters
- richer application-limited detection
- hysteresis and hold-down coordination with the outer QoS loop
