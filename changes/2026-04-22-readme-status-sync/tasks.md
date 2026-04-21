# Tasks

## 1. Refresh Wiring

- [ ] 1.1 Add a script-managed status block to the root `README.md`
  Verification: README contains stable begin/end markers

- [ ] 1.2 Update `scripts/run_all_tests.sh` to refresh the block from current matrix JSON artifacts
  Verification: running the refresh logic updates the two status lines

## 2. Nightly Recording

- [ ] 2.1 Update `scripts/nightly_full_regression.py` to include root `README.md` in nightly auto-recording
  Verification: pre/post status collection includes `README.md`

## 3. Knowledge Update

- [ ] 3.1 Update `specs/current/test-entrypoints.md`
  Verification: the root README refresh behavior is documented

## 4. Delivery Gate

- [ ] 4.1 Run script syntax checks
  Verification:
  - `bash -n scripts/run_all_tests.sh`
  - `python3 -m py_compile scripts/nightly_full_regression.py`
