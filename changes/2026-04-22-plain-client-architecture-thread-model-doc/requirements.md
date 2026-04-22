# Requirements

## Problem Statement

The repository has multiple plain-client documents, but there is no single current document that explains the whole plain-client runtime architecture and thread model after the threaded-only cutover.

This makes it harder to answer basic implementation questions such as:

- what threads exist now
- which thread owns transport
- why there is one `SourceWorker` per video track
- how queues, commands, acks, and snapshots flow between components

## Goal

Produce one current-source-of-truth document for the plain-client architecture and thread model that matches the current code, not a historical migration stage.

## In Scope

- current plain-client runtime structure
- thread inventory and responsibilities
- queue and ownership model
- startup and teardown flow
- inner-loop / outer-loop QoS and transport boundaries
- why the current multi-thread split exists
- links from existing plain-client docs to the new document

## Out of Scope

- browser-side QoS architecture
- historical migration detail beyond brief context
- LiveKit alignment roadmap details already covered elsewhere
- code changes to runtime behavior

## Acceptance Criteria

1. A single plain-client document SHALL describe the current architecture and thread model end to end.
2. The document SHALL explicitly describe:
   - `PlainClientApp`
   - `PlainClientThreaded`
   - `ThreadedQosRuntime`
   - `NetworkThread`
   - audio worker
   - per-track `SourceWorker`
3. The document SHALL explain why there is one video worker per track but only one transport owner thread.
4. Existing plain-client entry docs SHALL link to the new document as the main detailed reference.
