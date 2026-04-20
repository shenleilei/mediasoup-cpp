# External Review Follow-ups

## Summary

`review.md` contains a mixed external review ledger:

- confirmed defects that are still present in the current tree
- findings already invalidated by later changes
- low-priority hygiene notes that would make the next patch too mixed to review safely

The current tree still has confirmed bugs in the shared media/FFmpeg helpers, room/QoS lifecycle cleanup, and room-registry redirect hardening.

## Reproduction

1. Review the current implementations in `common/media/rtp/H264Packetizer.cpp`, `common/media/rtp/RtpHeader.h`, and `common/ffmpeg/*`.
2. Review the reconnect/leave cleanup paths in `src/RoomServiceLifecycle.cpp`.
3. Review the cached remote redirect path in `src/RoomRegistry.cpp` and the startup path in `src/RoomRegistry.cpp`.
4. Review the duplicated stats-response helper in `src/RoomServiceQos.cpp` and `src/RoomServiceDownlink.cpp`, plus the unused registry heartbeat wrapper in `src/RoomServiceRegistryView.cpp`.

## Observed Behavior

- H264 FU-A fragmentation drops the original NAL F bit.
- `RtpHeader::Parse()` accepts non-RTP version values and does not strip payload padding.
- `OutputFormat::Close()` frees the muxer without a best-effort trailer write when `WriteHeader()` already succeeded.
- FFmpeg wrapper send methods throw on `EAGAIN`, which makes normal send/drain loops impossible to express cleanly.
- `MakeBitstreamFilterContext()` ignores the `av_bsf_alloc()` return code and loses the real failure reason.
- `join(reconnect)` and `leave()` clear only peer-scoped QoS override state and leave room-pressure override records behind.
- `claimRoom()` trusts a cached remote room owner address without revalidating that the remote node is still live.
- `RoomRegistry::start()` bypasses the normal command-connection locking discipline during startup registration.
- the same stats-response helper exists twice in different translation units
- `RoomService::heartbeatRegistry()` is declared and defined but has no call sites

## Expected Behavior

- shared RTP helpers MUST preserve protocol bits and reject malformed packets instead of silently accepting or altering them
- FFmpeg wrappers MUST preserve container integrity and expose normal codec/filter backpressure without throwing on expected `EAGAIN` flow control
- reconnect and leave paths MUST clear both peer-scoped and room-pressure-scoped automatic override state for the affected peer
- room-registry cached redirects MUST not send clients to a dead remote node based only on stale cache state
- public room-registry command paths MUST follow one locking protocol, including startup registration
- touched RoomService stats/QoS helper code SHOULD keep one shared response builder and SHOULD not retain dead helper entrypoints

## Known Scope

- `common/media/rtp/H264Packetizer.cpp`
- `common/media/rtp/RtpHeader.h`
- `common/ffmpeg/AvPtr.h`
- `common/ffmpeg/OutputFormat.*`
- `common/ffmpeg/Decoder.*`
- `common/ffmpeg/Encoder.*`
- `common/ffmpeg/BitstreamFilter.*`
- `src/RoomServiceLifecycle.cpp`
- `src/RoomServiceQos.cpp`
- `src/RoomServiceDownlink.cpp`
- `src/RoomStatsQosHelpers.h`
- `src/RoomService.h`
- `src/RoomServiceRegistryView.cpp`
- `src/RoomRegistry.cpp`
- targeted media, review-fix, room, and multinode tests

## Must Not Regress

- QoS signaling ack semantics already accepted in `specs/current/qos-signaling.md`
- runtime-safety fixes already accepted in `specs/current/runtime-safety.md`
- existing room-registry ordering and local-node fallback behavior when Redis is unavailable
- normal recorder finalization and existing FFmpeg error-reporting behavior for explicit `WriteTrailer()`

## Suspected Root Cause

Confidence: high.

The functional refactor wave improved structure but left several small correctness gaps:

- protocol helpers were split without preserving all wire-level edge cases
- the FFmpeg adapter layer wrapped allocation and send/receive paths asymmetrically
- room/QoS cleanup kept peer-key cleanup but missed the parallel room-pressure key shape
- room-registry caching still optimizes for the happy path and does not revalidate cached remote ownership before redirecting
- a few low-risk refactors left duplicated helpers and unused entrypoints behind

## Acceptance Criteria

### Requirement 1

Shared RTP media helpers SHALL preserve protocol semantics for fragmented H264 and parsed RTP packets.

#### Scenario: FU-A fragmentation preserves the original NAL classification bits

- WHEN `H264Packetizer` fragments a NAL unit into FU-A packets
- THEN the generated FU indicator SHALL preserve the original F and NRI bits

#### Scenario: RTP parser rejects malformed packets and strips padding

- WHEN `RtpHeader::Parse()` receives a packet whose RTP version is not 2
- THEN parsing SHALL fail

- WHEN `RtpHeader::Parse()` receives a padded RTP packet
- THEN the parsed payload span SHALL exclude the trailing padding bytes

### Requirement 2

The FFmpeg adapter layer SHALL preserve container cleanup and normal codec/filter backpressure semantics.

#### Scenario: muxer close after header write

- WHEN `OutputFormat::Close()` is reached after `WriteHeader()` succeeded but before an explicit `WriteTrailer()`
- THEN close SHALL perform a best-effort trailer write before freeing the format context
- AND it SHALL still release resources even if trailer finalization fails

#### Scenario: expected codec/filter backpressure

- WHEN `avcodec_send_packet()`, `avcodec_send_frame()`, or `av_bsf_send_packet()` returns `AVERROR(EAGAIN)`
- THEN the wrapper SHALL report that backpressure as a non-exceptional result rather than throwing

#### Scenario: bitstream-filter allocation failure

- WHEN `av_bsf_alloc()` fails
- THEN the wrapper SHALL preserve the real error path instead of collapsing it into a null-only failure

### Requirement 3

Room/QoS peer lifecycle cleanup SHALL remove all automatic override state owned by the departing or reconnecting peer.

#### Scenario: reconnect replaces an existing peer

- WHEN `join()` replaces an existing peer with the same `roomId/peerId`
- THEN both peer-scoped and room-pressure-scoped override records for that peer SHALL be removed before the new peer continues

#### Scenario: leave removes a peer from a live room

- WHEN `leave()` removes a peer
- THEN both peer-scoped and room-pressure-scoped override records for that peer SHALL be removed

### Requirement 4

Room-registry remote redirects SHALL be validated strongly enough to avoid stale-cache redirects, and startup command paths SHALL follow the standard locking discipline.

#### Scenario: cached remote owner is stale

- WHEN `claimRoom()` finds a cached remote owner address
- THEN it SHALL confirm the remote ownership is still valid before returning a redirect
- AND it SHALL fall back to local claim or refreshed resolution instead of redirecting to a dead node

#### Scenario: startup registration path

- WHEN `RoomRegistry::start()` performs initial node registration and sync
- THEN Redis command access SHALL follow the same mutex discipline used by other public command paths

## Regression Expectations

- existing unaffected room-registry resolution and fail-open local fallback behavior remains unchanged
- existing QoS ack/worker-completed behavior remains unchanged
- new regression coverage is added for:
  - H264 FU-A F-bit preservation
  - RTP version/padding parsing
  - FFmpeg trailer/backpressure/allocation edge cases
  - reconnect/leave room-pressure override cleanup
  - stale cached remote redirect validation
- required manual smoke checks:
  - none expected if targeted automated coverage is sufficient, unless the registry test surface exposes an environment-only gap
