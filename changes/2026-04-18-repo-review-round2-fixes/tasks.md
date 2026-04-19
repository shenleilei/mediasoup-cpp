# Tasks

1. [completed] Fix WebRTC transport replacement lifecycle
Outcome:
- repeated WebRTC transport creation closes and cleans old peer-owned state
Verification:
- integration regression for repeated send/recv transport creation

2. [completed] Fix consumer producer-close cleanup
Outcome:
- producer-close notifications close and unregister local consumer state
Verification:
- integration regression for consumer disappearance after producer close

3. [completed] Fix strict consumer codec compatibility
Outcome:
- consumer RTP generation rejects unmatched codecs
Verification:
- updated ORTC unit tests

4. [completed] Run targeted verification and report remaining review debt
Outcome:
- affected suites pass and remaining findings are clearly separated from fixed issues
Verification:
- test commands recorded with pass/fail status
