# Tasks

1. [x] Lock the shared-helper boundary
Outcome:
- the change docs explicitly separate protocol helpers from runtime ownership
- phase 1 is limited to `RtpHeader`, `H264Packetizer`, and Annex-B/AVCC conversion
Verification:
- `requirements.md` and `design.md` match on phase-1 scope and non-goals

2. [x] Introduce the shared media helper layer
Outcome:
- repo-level shared helper files exist for:
  - RTP header parsing
  - H264 RTP packetization
  - Annex-B/AVCC conversion
Verification:
- root build and standalone `client/` build both compile with the new shared path

3. [x] Migrate plain-client packetizer call sites
Outcome:
- plain-client legacy and threaded send paths both use the same shared H264 packetizer
- send-side sinks remain role-local (`send()+RTCP` vs pacing queue)
Verification:
- `plain-client` target builds
- targeted plain-client regressions remain green

4. [x] Migrate helper tests to the public shared API
Outcome:
- shared helper tests no longer rely on recorder-private test access for migrated helpers
- packetizer behavior has direct unit coverage
Verification:
- focused helper-level tests pass

5. [x] Re-evaluate H264 depacketization as a gated follow-up
Outcome:
- explicit decision recorded:
  - either define a caller-owned `H264AuAssembler` design for phase 2
  - or defer depacketization until recorder boundaries are cleaner
Verification:
- tasks/design updated with the accepted phase-2 decision

## Explicit Non-Goals After Phase 1

- Do not start `H264Depacketizer` just because a shared media directory now exists.
- Do not merge recorder and plain-client lifecycle/threading code.
- Do not move RTCP/NACK/PLI or FFmpeg resource ownership into the shared layer without a separate change.
