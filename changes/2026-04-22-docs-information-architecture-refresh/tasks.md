# Tasks

## 1. Define The Structure

- [ ] 1.1 Create the target function-first documentation taxonomy and directory layout
  Files: `docs/README.md`, new directory README files
  Verification: manual review that each functional domain has a clear purpose

## 2. Move Hand-Written Docs

- [ ] 2.1 Move platform/runtime docs under `docs/platform/`
  Verification: moved files exist and are indexed from `docs/README.md`

- [ ] 2.2 Move operations docs under `docs/operations/`
  Verification: moved files exist and are indexed from `docs/README.md`

- [ ] 2.3 Move planning docs under `docs/planning/`
  Verification: moved files exist and are indexed from `docs/README.md`

- [ ] 2.4 Move QoS docs into function subtrees under `docs/qos/*`
  Verification: moved files exist and stable QoS entry docs still link correctly

- [ ] 2.5 Move obvious one-off process docs into `docs/archive/`
  Verification: root clutter is reduced and archive index remains coherent

## 3. Rewrite Links

- [ ] 3.1 Rewrite affected markdown links across the repository
  Verification: touched markdown files resolve moved links correctly

## 4. Refresh Entry Docs

- [ ] 4.1 Rewrite `docs/README.md` around the new information architecture
  Verification: root index is shorter, clearer, and role-based

- [ ] 4.2 Add directory README files for the new major doc groups
  Verification: each directory has a clear local entrypoint

## 5. Delivery Gates

- [ ] 5.1 Run markdown link checks for the touched scope
  Verification: no broken local links remain in the reorganized docs set

- [ ] 5.2 Run `git diff --check`
  Verification: command exits cleanly
