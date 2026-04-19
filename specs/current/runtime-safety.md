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
- `Channel::close()` SHALL not leave a joinable read thread behind when invoked on the read thread itself.

## Recorder Guardrails

- Recorder startup SHALL fail safely if FFmpeg cannot allocate the required audio or video streams.
- H264 extradata construction SHALL reject SPS data shorter than 4 bytes instead of reading past the buffer.
