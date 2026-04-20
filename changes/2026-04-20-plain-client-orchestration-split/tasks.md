# Tasks

1. [x] Introduce plain-client app orchestration modules
Outcome:
- `PlainClientApp` owns shared runtime state and `main.cpp` becomes thin
Verification:
- `plain-client` builds

2. [x] Move legacy runtime orchestration out of `main.cpp`
Outcome:
- legacy loop lives in a focused module
Verification:
- targeted build/regression checks

3. [x] Move threaded runtime orchestration out of `main.cpp`
Outcome:
- threaded loop lives in a focused module
Verification:
- targeted build/regression checks

4. [x] Update docs and verify final structure
Outcome:
- live docs reflect the new orchestration layout
Verification:
- docs updated and targeted commands rerun
