# Design

## Context

The current documentation problem is not only “too many docs”.

It is specifically that the tree shape does not match the semantic roles of the documents. A reader currently has to infer document class from filename patterns such as:

- `uplink-qos-*`
- `downlink-qos-*`
- `linux-client-*`

That naming-only approach does not scale.

## Design Principles

### 1. Function First, Role Second

Directory placement should answer “what part of the system or product am I looking at?” before “what style of doc is this?”.

That means the top-level split should first separate:

- platform
- QoS
- operations
- planning
- generated
- archive

Within QoS, the next split should be functional again:

- uplink
- plain-client
- downlink
- shared QoS references

Document role still matters, but it should be expressed inside the functional subtree rather than by keeping everything flat and encoding the distinction in filenames alone.

### 2. Keep Current Entrypoints Shallow

Root-level `docs/` should mainly contain:

- `README.md`
- stable QoS status and result entrypoints
- a small number of top-level current-state summary docs

These are the docs a maintainer or reviewer opens first.

### 3. Do Not Move Generated Outputs

Generated outputs are already wired into scripts and reports.

Changing those paths would turn a documentation cleanup into a larger tooling migration. This refresh intentionally leaves generated paths stable.

### 4. Move Hand-Written Supporting Docs By Function

Planned target structure:

- `docs/platform/`
- `docs/operations/`
- `docs/planning/`
- `docs/qos/`
  - `docs/qos/uplink/`
  - `docs/qos/plain-client/`
  - `docs/qos/downlink/`
  - `docs/qos/shared/`
- existing `docs/generated/`
- existing `docs/archive/`

### 5. Make Future Drift Harder

The root index should explicitly state:

- what belongs at the root
- what belongs in each functional subtree
- what belongs in `archive/`
- what belongs in `changes/` instead of `docs/`

## Why This Is Better

- It reduces filename-driven ambiguity.
- It reduces the need for repetitive long prefixes as the only distinguishing mechanism.
- It makes current-vs-supporting-vs-historical boundaries visible in the tree.
- It improves future maintenance because new docs have a clear destination.

## Migration Strategy

1. Create the new directory structure.
2. Move hand-written docs whose roles are already clear.
3. Rewrite markdown links automatically based on the move map.
4. Refresh root and directory README files.
5. Validate markdown link integrity for the touched scope.

## Risks

- large path churn can leave stale links
- some docs may conceptually span multiple categories
- external bookmarks to moved hand-written docs may break

## Mitigation

- keep generated outputs stable
- use a deterministic path-rewrite pass for markdown links
- add clear directory README entrypoints
- move only documents whose category is high-confidence in the first pass

## Verification Strategy

- markdown link scan over touched docs
- `git diff --check`
- manual read-through of:
  - `docs/README.md`
  - new directory README files
  - major stable entrypoints that reference moved docs
