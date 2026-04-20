# Runtime Safety

## WebSocket Signaling

- Malformed WebSocket request payloads SHALL be dropped without terminating the signaling server process.
- A malformed request SHALL NOT poison the connection or prevent a later valid request from being processed.

## Room Registry Ordering

- When geo data is unavailable, room-registry candidate ordering SHALL prefer lower room count first.
- When room counts tie, the current node MAY be preferred only as a deterministic tie-breaker.
- The no-geo comparator SHALL satisfy strict weak ordering.

## Worker And Channel Shutdown

- Worker close and worker-death paths SHALL be idempotent and safe when reached from different threads.
- Worker spawn SHALL avoid post-`fork()` non-async-signal-safe setup and SHALL clean up partially created pipe resources on failure.
- `Channel::close()` SHALL not leave a joinable read thread behind when invoked on the read thread itself.
- `Channel` send/close paths SHALL not write through a stale producer fd during concurrent shutdown.
- Non-threaded `Channel` message pumping SHALL tolerate notification callbacks that re-enter `requestWait()` without replaying the same buffered message.

## RTP And Capability Correctness

- `repaired-rtp-stream-id` header extensions SHALL map to the repair-stream URI enum rather than the plain stream-id enum.
- Advertised supported RTP capabilities SHALL not reuse the same non-zero payload type for different codec variants.

## Input Hardening

- Malformed Redis reply shapes or missing string payloads SHALL be ignored or rejected without dereferencing null reply elements.
- Invalid numeric CLI option values SHALL fail startup explicitly rather than crashing or silently falling back to defaults.

## Recorder Guardrails

- Recorder startup SHALL fail safely if FFmpeg cannot allocate the required audio or video streams.
- H264 extradata construction SHALL reject SPS data shorter than 4 bytes instead of reading past the buffer.
- Recorder QoS timestamps SHALL use a synchronization-safe first-RTP baseline.
