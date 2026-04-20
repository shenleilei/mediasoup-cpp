# Tasks

1. [x] Define the Recorder split boundary
Outcome:
- change docs clearly constrain the work to declaration/implementation separation
Verification:
- `requirements.md` and `design.md` are aligned

2. [x] Move `PeerRecorder` implementation into `src/Recorder.cpp`
Outcome:
- `Recorder.h` becomes declaration-oriented and runtime bodies move to `Recorder.cpp`
Verification:
- recorder production targets compile

3. [x] Update link targets that depended on header-only recorder code
Outcome:
- all affected executables/tests link after `PeerRecorder` is no longer header-only
Verification:
- affected build targets succeed

4. [x] Run recorder-focused regression checks
Outcome:
- recorder behavior remains unchanged after the split
Verification:
- targeted recorder-related tests pass

5. [x] Update docs and record results
Outcome:
- live docs and change tasks reflect the final structure truthfully
Verification:
- docs updated
