# Tasks

1. [x] Define the RoomServiceStats split boundary
Outcome:
- change docs clearly define the new implementation ownership
Verification:
- `requirements.md` and `design.md` align

2. [x] Move QoS control logic out of `RoomServiceStats.cpp`
Outcome:
- QoS ingest/override/connection-quality logic lives in a dedicated file
Verification:
- `mediasoup-sfu` and targeted QoS builds succeed

3. [x] Move registry-view helpers and downlink ingest
Outcome:
- registry/load helpers and `setDownlinkClientStats()` no longer live in `RoomServiceStats.cpp`
Verification:
- affected builds succeed

4. [x] Re-run targeted stats/QoS verification
Outcome:
- stats reporting and QoS behavior remain unchanged after the structural split
Verification:
- targeted integration tests pass

5. [x] Update docs and record final structure
Outcome:
- docs reflect the narrowed responsibilities truthfully
Verification:
- docs updated
