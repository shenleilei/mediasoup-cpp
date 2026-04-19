# Tasks

1. [completed] Harden WebSocket request parsing
Outcome:
- malformed request payloads no longer crash the server process
Verification:
- integration regression for malformed WebSocket input and follow-up valid request

2. [completed] Fix room-registry candidate ordering
Outcome:
- no-geo candidate sorting is strict-weak-ordering safe and unit-tested
Verification:
- unit regression for self tie-break ordering

3. [completed] Synchronize worker/channel shutdown paths
Outcome:
- worker close/death and channel self-close avoid unsynchronized teardown
Verification:
- targeted runtime/thread/unit suites compile and pass

4. [completed] Add recorder defensive guards
Outcome:
- recorder setup/shutdown rejects null stream allocation prerequisites and short SPS safely
Verification:
- recorder QoS unit regression for short SPS rejection

5. [completed] Run targeted verification and record remaining review debt
Outcome:
- fixed issues are separated from remaining validated-but-unfixed review items
Verification:
- build/test commands recorded with pass/fail status
