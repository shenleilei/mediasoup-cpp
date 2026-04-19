# Tasks

1. [completed] Fix plain transport replacement lifecycle
Outcome:
- `RoomServiceMedia` closes and cleans old plain transports before replacement
Verification:
- targeted transport replacement regression test

2. [completed] Fix plain publish H264 contract
Outcome:
- `plainPublish()` selects Baseline H264 and registers matching RTP parameters
Verification:
- targeted server-side test for returned/profiled codec selection

3. [completed] Fix RTCP PLI and RR parsing
Outcome:
- PLI requests a fresh keyframe path and RR signed loss parsing is correct in both client paths
Verification:
- RTCP unit tests in `tests/test_thread_model.cpp` and `tests/test_review_fixes.cpp`

4. [completed] Fix QoS loss source switching
Outcome:
- QoS counter deltas reset safely when switching between local RTCP and server stats
Verification:
- new client QoS regression test

5. [completed] Re-run targeted verification and report remaining review debt
Outcome:
- affected suites pass and remaining validated review debt is separated from the fixed items
Verification:
- test commands and updated review result
