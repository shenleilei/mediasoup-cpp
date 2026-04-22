# Design

## Context

This is a documentation and upgrade-assessment change, not a runtime behavior change.

The repository already has a platform-focused worker architecture document:

- `docs/platform/mediasoup-worker-architecture-analysis_cn.md`

What is missing is a companion document for maintainers answering a different question:

- what changed upstream since our current worker baseline
- what local worker patches exist today
- what breaks if we upgrade directly
- which fixes are worth backporting first

## Chosen Output

Add a new platform document:

- `docs/platform/mediasoup-worker-upgrade-assessment_cn.md`

and add a small discoverability link from:

- `docs/platform/README.md`

Track the work using a structured change folder:

- `changes/2026-04-22-mediasoup-worker-upgrade-assessment/`

## Document Structure

The assessment document should be organized around maintainer decisions rather than raw chronology.

### 1. Current Baseline

State:

- the current vendored worker tag and local extra commit
- the current runtime worker launch model (`workerBin`, external process)
- the currently observed local patch categories

### 2. Recent Upstream Releases

Summarize the worker-relevant parts of recent upstream releases and classify them as:

- bugfix
- new option / feature
- integration-breaking change
- build/toolchain change

### 3. High-Value Bugfixes

Give dedicated subsections for the fixes that matter most to this repo today, especially:

- `3.19.9` RTCP `packets_lost` signedness/statistics fix
- `3.19.15` duplicate RTP packet suppression in `RtpStreamSend`

For `3.19.9`, explain the implementation path clearly:

- `expected` from sequence tracking
- `received` from `mediaTransmissionCounter`
- why negative/descending cumulative loss is possible
- why unsigned propagation is wrong

### 4. Upgrade Blockers

Document the changes that make a direct worker jump expensive:

- `WORKER_CLOSE` request -> notification change
- FBS schema drift in `request`, `notification`, `rtpParameters`, `rtpStream`, `transport`
- C++20 / Meson requirement changes
- mismatch with current local environment and generated bindings

### 5. Local Patch Carry-Forward Assessment

Evaluate the existing local worker patches one by one:

- mirror URL patch status
- flatbuffers/meson patch status
- whether the patch still appears necessary on newer upstream

This section must distinguish:

- definitely keep
- rework before carrying
- likely obsolete unless local build flow requires it

### 6. Recommendation Matrix

Close with a short decision matrix:

- targeted backport candidates
- full-upgrade prerequisites
- lower-priority items

## Source Basis

The document should derive its claims from:

- current repository local files
- the vendored `src/mediasoup-worker-src` git history
- upstream `mediasoup` release/changelog material already inspected during this task

The document should avoid unverified claims such as:

- assuming a mirror artifact exists without checking
- assuming an upstream fix matters to this repo without checking current repo usage

## Verification Strategy

This is a docs-only change.

Verification should cover:

- `git diff --check`
- manual read-through for internal consistency
- confirm that the new platform doc is linked from `docs/platform/README.md`

## Risks

- the document could become a chat transcript dump instead of a maintainer guide
- the bugfix sections could over-focus on chronology rather than upgrade decisions
- the mirror section could go stale if it states availability too strongly

## Mitigation

- keep each section anchored to a maintainer question
- separate bugfix value from upgrade cost explicitly
- state mirror findings conservatively as current observations, not permanent guarantees
