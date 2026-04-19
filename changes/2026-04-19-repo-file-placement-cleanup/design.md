# Design

## Context

Most of the repository root follows stable project conventions: build system files, top-level docs, source trees, and tool entrypoints. Two tracked areas stand out as obvious placement problems:

1. `test_sweep.mp4`, `test_sweep_av.mp4`, and `test_sweep_cpp_matrix.mp4` live at the repository root even though they are only test fixtures.
2. `docs/non-qos-test-results.md` is a generated report from an older entrypoint contract and no longer belongs in the active top-level docs surface.

## Goals

- move tracked media fixtures into a test-owned fixture directory
- preserve all existing test and script behavior by updating every in-repo reference
- archive the obsolete non-QoS report so the active docs surface only shows current result artifacts

## Proposed Approach

### 1. Move sweep MP4 fixtures under `tests/fixtures/media/`

Use git moves:

- `test_sweep.mp4` → `tests/fixtures/media/test_sweep.mp4`
- `test_sweep_av.mp4` → `tests/fixtures/media/test_sweep_av.mp4`
- `test_sweep_cpp_matrix.mp4` → `tests/fixtures/media/test_sweep_cpp_matrix.mp4`

Update every caller to use the new canonical location:

- `client/run_sweep_test.sh`
- `tests/qos_harness/cpp_client_runner.mjs`
- `tests/test_thread_integration.cpp`
- docs snippets that mention the old root-level path

### 2. Archive the obsolete non-QoS report

Move:

- `docs/non-qos-test-results.md` → `docs/archive/non-qos-test-results.md`

Reasoning:

- the current accepted entrypoint is `docs/full-regression-test-results.md`
- the old file is still useful as historical reference
- keeping it in `docs/` makes the current docs surface noisier and less truthful

### 3. Leave intentional top-level conventions untouched

Do not move:

- `AGENTS.md`
- `CLAUDE.md`
- `.codex`

These are explicit tool-entrypoint conventions described by the project docs.

## Alternatives Considered

### Alternative: leave the root-level MP4s in place

Rejected because they are test-only assets and materially clutter the root directory.

### Alternative: delete `docs/non-qos-test-results.md`

Rejected because it is still a valid historical artifact; archiving is safer than deletion.

## Testing Strategy

- `rg` verification that no active in-repo references still point to the old root-level MP4 paths
- `bash -n client/run_sweep_test.sh`
- syntax-check any edited JS/Python scripts if applicable
- confirm the moved report is no longer under `docs/` and is preserved under `docs/archive/`
