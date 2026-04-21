# Bugfix Design

## Context

The nightly wrapper already has two relevant parsing layers:

1. top-level report parsing from `docs/full-regression-test-results.md`
2. nested raw-log parsing via `parse_task_blocks()`

The missing piece is presentation.

The wrapper computes group-level case summaries from nested blocks, but it does not render the delegated QoS task rows themselves.

## Fix Strategy

### 1. Preserve Top-Level Report Parsing

Keep the existing markdown-report parsing untouched for:

- top-level attempted tasks
- top-level failed groups
- top-level failed tasks

### 2. Extend Nested Block Parsing

Teach the nested block parser to retain:

- status
- elapsed duration

for nested task blocks extracted from the raw log.

### 3. Build Delegated QoS Task Rows

Add a dedicated extraction step that:

- selects nested raw-log blocks under the `qos` group
- excludes helper/noise labels such as:
  - `system:*`
  - report-generation helper labels
  - build-only labels
- preserves task order

The result should represent the delegated QoS execution surface rather than the top-level `qos:qos-regression` wrapper.

### 4. Render The New Section

Render a new “QoS delegated tasks” section in:

- plain-text email body
- HTML email body
- summary JSON payload

## Why This Is Better

- It gives the reader the missing visibility they expect from nightly.
- It avoids changing `run_all_tests.sh` contract just to expose nested QoS tasks.
- It reuses the raw log that the nightly wrapper already captures.

## Verification Strategy

- feed an existing nightly `run_all_tests.log` and matching report through:
  - `scripts/nightly_full_regression.py run --skip-tests --source-log ... --source-report ... --print-summary --no-mail`
- verify the output contains delegated task rows such as:
  - `cpp-client-matrix`
  - `cpp-client-ab`
  - `cpp-threaded:gtest`
