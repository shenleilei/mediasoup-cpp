# Requirements

## Summary

Refactor the repository's CMake configuration so project-owned targets share a consistent, centralized definition of common includes, libraries, and test registration behavior.

## Business Goal

Developers need the build system to be easier to maintain across the whole repository so new targets can be added or updated without repeating fragile configuration blocks in multiple places.

## In Scope

- top-level `CMakeLists.txt`
- shared CMake helper modules under `cmake/`
- `client/CMakeLists.txt` where it can reuse the same project-wide build conventions
- consolidation of repeated include-directory, link-library, and test-registration logic for project-owned targets

## Out Of Scope

- behavior changes in SFU, QoS, client, or worker runtime code
- changes to `third_party/`, generated code, or vendored mediasoup worker sources
- renaming public binaries or test targets
- changing the set of required external system libraries

## User Stories

### Story 1

As a developer
I want shared target configuration to live in one place
So that I can change common dependencies without editing many repeated blocks

### Story 2

As a developer
I want test targets to be declared through one consistent path
So that target setup stays readable and less error-prone as the project grows

## Acceptance Criteria

### Requirement 1

The system SHALL centralize repeated project-owned target configuration into reusable CMake helpers or shared targets.

#### Scenario: shared include and link configuration

- WHEN a project-owned executable or test target needs common repository dependencies
- THEN those common dependencies SHALL come from centralized CMake definitions
- AND the configuration SHALL avoid duplicating equivalent include and link blocks across target declarations

### Requirement 2

The system SHALL preserve the existing top-level build outputs and test target names.

#### Scenario: existing target compatibility

- WHEN the project is configured and built after the refactor
- THEN existing main binaries and test targets SHALL keep their current names
- AND downstream scripts and docs that reference those names SHALL remain valid

### Requirement 3

The system SHALL keep project test registration behavior unchanged.

#### Scenario: ctest target registration

- WHEN CMake configures the repository with tests enabled
- THEN the same project test executables SHALL remain registered with `ctest`
- AND their working directory assumptions SHALL remain intact

## Non-Functional Requirements

- Maintainability: repeated target setup should be expressed once per shared concern
- Reliability: configuration changes must not introduce missing includes or missing libraries for existing targets
- Compatibility: existing configure and build commands remain valid
- Observability: the resulting CMake structure should make target dependencies easier to inspect during future maintenance

## Edge Cases

- targets with extra dependencies beyond the common baseline must still be able to add their own libraries cleanly
- tests that need `${CMAKE_SOURCE_DIR}/tests` in their include path must preserve that access without giving every target unnecessary visibility
- targets that require extended FFmpeg libraries such as `swscale` or `avdevice` must remain representable through the shared helper path

## Open Questions

- None for this refactor; runtime behavior is intentionally unchanged
