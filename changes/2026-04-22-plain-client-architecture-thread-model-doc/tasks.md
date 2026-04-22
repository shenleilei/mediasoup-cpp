# Tasks

## 1. Gather

- [x] 1.1 Inspect the current plain-client runtime code paths and responsibilities
  Verification: current components and thread ownership are enumerated explicitly

## 2. Author

- [x] 2.1 Add a single consolidated plain-client architecture and thread-model document
  Files: new doc under `docs/qos/plain-client/`
  Verification: the new doc covers runtime architecture, thread model, queues, startup, and ownership

- [x] 2.2 Update plain-client entry docs to link to the new single detailed document
  Files: `docs/qos/plain-client/README.md`, related summary doc(s)
  Verification: readers can discover the new doc from existing entrypoints

## 3. Validate

- [x] 3.1 Run formatting / patch sanity checks
  Verification: `git diff --check` passes for the changed docs
