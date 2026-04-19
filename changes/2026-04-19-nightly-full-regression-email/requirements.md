# Requirements

## Summary

Add a nightly full-regression automation flow that runs the accepted repository-wide regression at 03:00 local time, saves durable per-run logs, emails a summary, and attaches the latest generated Markdown reports.

## Business Goal

Developers need unattended nightly feedback for the repository's accepted full regression surface without having to manually launch `scripts/run_all_tests.sh`, inspect logs, or collect generated reports by hand.

## In Scope

- a nightly automation entrypoint that delegates to `scripts/run_all_tests.sh`
- per-run timestamped log and artifact retention
- email summary generation with task pass rate and failed-case extraction
- attachment of selected generated Markdown reports
- a repo-local installation flow for a 03:00 daily scheduler entry
- operational documentation and accepted-spec updates for the nightly flow

## Out Of Scope

- changing the test coverage or behavior of `scripts/run_all_tests.sh`
- changing the accepted QoS report generation owned by `scripts/run_qos_tests.sh`
- introducing a hosted CI system or external alerting platform
- rotating or pruning historical nightly artifacts beyond basic local retention hooks if any

## User Stories

### Story 1

As a developer
I want a single nightly job to run the full repository regression at 03:00
So that I get unattended daily signal on repository health.

### Story 2

As a developer
I want every nightly run to leave behind its own log directory
So that I can investigate failures after the generated docs are overwritten by later runs.

### Story 3

As a developer
I want the email body to summarize pass rate and failed cases
So that I can triage the nightly result without opening the raw log first.

### Story 4

As a developer
I want the email to include the generated Markdown reports as attachments
So that I can review the latest run artifacts directly from the message.

## Acceptance Criteria

### Requirement 1

The system SHALL provide a nightly automation entrypoint for the accepted full regression.

#### Scenario: nightly execution

- WHEN the nightly entrypoint runs
- THEN it SHALL invoke `scripts/run_all_tests.sh`
- AND it SHALL preserve the delegated behavior and final exit status of that script
- AND it SHALL save the full stdout/stderr stream to a durable log file

#### Scenario: local retention

- WHEN the nightly entrypoint starts a run
- THEN it SHALL create a timestamped artifact directory for that run
- AND that directory SHALL keep the raw test log and generated summary artifacts for later inspection

### Requirement 2

The system SHALL produce an email summary from the nightly run result.

#### Scenario: success or failure

- WHEN the nightly run finishes
- THEN the email body SHALL include the overall status
- AND it SHALL include the task pass rate derived from the full regression report when available
- AND it SHALL include the failed tasks
- AND it SHALL include the failed cases parsed from the raw log when any are detected

#### Scenario: missing structured report

- WHEN `docs/full-regression-test-results.md` is missing or unreadable after the run
- THEN the nightly entrypoint SHALL still send an email
- AND it SHALL fall back to a best-effort summary from the raw log and wrapper metadata

### Requirement 3

The system SHALL attach the latest generated Markdown reports to the email.

#### Scenario: report attachment snapshot

- WHEN the nightly run completes
- THEN the nightly entrypoint SHALL snapshot the selected Markdown report files for that run
- AND it SHALL attach the snapshot copies to the outgoing email
- AND missing report files SHALL be called out in the summary instead of failing silently

### Requirement 4

The system SHALL provide a repo-local 03:00 scheduler installation flow.

#### Scenario: cron installation

- WHEN the install script runs
- THEN it SHALL create or update a daily 03:00 scheduler entry for the nightly entrypoint
- AND repeated installs SHALL be idempotent
- AND the installed command SHALL reference the repo-local nightly config file

## Non-Functional Requirements

- Reliability: logs and run artifacts must be preserved even when tests or email delivery fail
- Observability: email and local artifacts must be sufficient to identify failed tasks and likely failed cases
- Maintainability: the nightly entrypoint must reuse `scripts/run_all_tests.sh` instead of copying its test orchestration logic
- Security: mail credentials or recipient overrides must come from local runtime configuration, not committed secrets

## Edge Cases

- The nightly run may fail before all generated Markdown reports are refreshed; the wrapper must still snapshot whichever configured files exist
- A nightly email transport may be unavailable; the wrapper must still preserve logs and a rendered email body locally
- The nightly summary should handle matrix-style failures such as `case L5 failed: ...` in addition to gtest-style `FAILED` lines
- The scheduler installer must not duplicate cron entries on repeated runs

## Open Questions

- None; the runtime email destination will default from local configuration and remain overrideable through an untracked env file
