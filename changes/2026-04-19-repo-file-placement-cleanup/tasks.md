# Tasks

- [x] 1. Record the file-placement cleanup intent in change docs
  - Files: `changes/2026-04-19-repo-file-placement-cleanup/requirements.md`, `changes/2026-04-19-repo-file-placement-cleanup/design.md`, `changes/2026-04-19-repo-file-placement-cleanup/tasks.md`
  - Verification: change docs explicitly identify the misplaced fixtures and obsolete generated report

- [x] 2. Move tracked sweep media fixtures under `tests/fixtures/media/`
  - Files: moved fixture files plus every in-repo caller that references them
  - Verification: `rg` shows no active callers still pointing at the old root-level paths

- [x] 3. Archive the obsolete non-QoS report
  - Files: `docs/non-qos-test-results.md` → `docs/archive/non-qos-test-results.md`
  - Verification: the archived file exists and the active docs surface no longer contains the obsolete top-level report

- [x] 4. Verify script and docs references after the moves
  - Files: updated scripts/docs as needed
  - Verification:
    - `bash -n client/run_sweep_test.sh`
    - any edited JS/Python files pass syntax checks
    - reference grep confirms the new canonical paths
