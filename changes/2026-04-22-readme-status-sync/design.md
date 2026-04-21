# Bugfix Design

## Context

This is a small documentation-refresh automation fix.

The desired source of truth for the root README status block already exists:

- `docs/generated/uplink-qos-matrix-report.json`
- `docs/generated/uplink-qos-cpp-client-matrix-report.json`

The missing part is wiring that source into:

- `scripts/run_all_tests.sh`
- nightly git recording

## Fix Strategy

### 1. Add A Script-Managed README Block

Wrap the root README matrix status lines in explicit HTML comment markers so they can be updated safely without regexing arbitrary prose.

### 2. Refresh The Block From Generated JSON

Extend `scripts/run_all_tests.sh` with a post-report step that:

- reads the latest browser matrix JSON
- reads the latest plain-client matrix JSON
- formats a concise status string
- rewrites only the marked README block

Formatting rule:

- always show passed / total
- append `FAIL` and `ERROR` counts only when non-zero

### 3. Include README In Nightly Auto-Recording

Extend `scripts/nightly_full_regression.py` so its dirty-path tracking includes:

- `docs`
- root `README.md`

This preserves the existing nightly “record changed docs” behavior while allowing the root README to travel with that same update set.

### 4. Document The Contract

Update `specs/current/test-entrypoints.md` so the script contract includes the root README refresh behavior.

## Verification Strategy

- syntax check `scripts/run_all_tests.sh`
- syntax check `scripts/nightly_full_regression.py`
- manually refresh the README block using current generated JSONs
- verify the nightly dirty-path tracking now includes `README.md`
