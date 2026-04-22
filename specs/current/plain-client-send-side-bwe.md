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

### Sender-Pressure Semantics

- The transport inner loop publishes a sender-pressure semantic with at least:
  - `none`
  - `warning`
  - `congested`
- The warning state may be triggered by sustained fresh-video backlog together with elevated RTT/jitter even before transport estimate clamp is visible.
- The congested state remains transport-owned and is reserved for stronger evidence such as pacing clamp or repeated local send retention.
- QoS outer-loop warning transitions may consume the warning state directly.
- QoS outer-loop `bandwidthLimited` semantics are only driven by browser bandwidth limitation or the strong sender-pressure congested state.

### Generation Switch Handling

- When encoder recreation bumps `configGeneration`, already queued fresh video RTP for the same SSRC SHALL remain queued and be sent in order.
- Generation switches may still clear generation-sensitive caches such as retransmission queue and cached keyframe state.
- Plain-client QoS adaptation MUST NOT create synthetic RTP sequence gaps by discarding fresh queued media on a downgrade action.

### Observability

- The threaded plain-client trace path exposes transport-estimate and sender-side white-box fields needed for targeted effectiveness review, including:
  - sender-usage bitrate
  - transport-estimate bitrate
  - effective pacing bitrate
  - transport feedback count
  - probe activity / probe packet count
  - queue and retransmission counters
  - sender-pressure state
- The canonical sender snapshot also carries plain-client-specific parity fields for:
  - sender transport delay / RTT-like pressure
  - sender transport jitter / variation pressure
  - sender limitation reason
- These plain-client parity fields are explicit local-equivalent fields.
  They do not redefine browser `qualityLimitationReason`.
- The current accepted implementation does not directly fold those local timing
  fields into canonical `roundTripTimeMs` / `jitterMs`, because that was shown
  to regress `J3` as `过强` during targeted matrix validation.

## Explicit Non-Goals

- This spec does not claim full parity with LiveKit's downlink `streamallocator` object model.
- Carrier-track selection for uplink probe RTP is a sender-specific equivalent mapping, not a literal reuse of LiveKit's deficient-track selection logic.
- This spec does not define a new weight-aware multi-track media scheduler under aggregate transport caps; existing multi-track pacing/fairness behavior remains outside this accepted scope.
