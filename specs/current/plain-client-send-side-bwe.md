# Plain-Client Send-Side BWE

## Scope

This spec describes the accepted runtime behavior of the plain-client send-side bandwidth-estimation path when:

- `PLAIN_CLIENT_ENABLE_TRANSPORT_CONTROLLER=1`
- `PLAIN_CLIENT_ENABLE_TRANSPORT_ESTIMATE=1`

It does not describe browser-side QoS behavior or LiveKit's full downlink allocator context.

## Accepted Behavior

### Transport-Wide Sequence Stability

- A queued RTP packet keeps a stable transport-wide sequence number across local retries after `WouldBlock`.
- A failed local send attempt does not allocate a new transport-wide sequence number for the same queued packet.
- Only successfully sent packets enter the send-side BWE sent-packet tracker.

### TWCC Feedback Handling

- Parsed transport-cc feedback expands reference-time cycles and tolerates out-of-order reports on the supported main path.
- Published transport estimate comes from the send-side BWE path rather than from a local-only fallback estimator.

### Probe Isolation

- Probe RTP packets do not populate the ordinary video retransmission packet store.
- Probe RTP packets are not selected by the ordinary media NACK retransmission path.
- Probe RTP packets do not inflate ordinary video octet accounting.

### Probe Lifecycle

- The network-thread pacing path supports:
  - probe cluster start
  - transport-cc-enabled probe RTP send
  - goal-reached early stop
  - probe finalization
- Probe-trigger goal construction follows the sender-side equivalent of LiveKit's default padding-probe policy:
  - `availableBandwidthBps` from the published transport estimate
  - `expectedUsageBps` from current sender usage
  - `desiredIncreaseBps = max(transitionDeltaBps * ProbeOveragePct / 100, ProbeMinBps)`
  - `desiredBps = expectedUsageBps + desiredIncreaseBps`

### Observability

- The threaded plain-client trace path exposes transport-estimate and sender-side white-box fields needed for targeted effectiveness review, including:
  - sender-usage bitrate
  - transport-estimate bitrate
  - effective pacing bitrate
  - transport feedback count
  - probe activity / probe packet count
  - queue and retransmission counters

## Explicit Non-Goals

- This spec does not claim full parity with LiveKit's downlink `streamallocator` object model.
- Carrier-track selection for uplink probe RTP is a sender-specific equivalent mapping, not a literal reuse of LiveKit's deficient-track selection logic.
- This spec does not define a new weight-aware multi-track media scheduler under aggregate transport caps; existing multi-track pacing/fairness behavior remains outside this accepted scope.
