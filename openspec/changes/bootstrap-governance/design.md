## Context
Local hooks can be bypassed; public CI cannot access the private journal. Installed versions are OpenSpec 1.15.0 and Beads 1.2.2 (Dolt, no bd sync).

## Goals / Non-Goals
Enforce incremental linkage and stage evidence, preserve installed hooks, surface journal failures. No feature/ABI/build modernization here.

## Decisions
Version equivalent Python tools in both repositories. Pre-commit reads the Git index. A stage work record binds Bead and OpenSpec IDs to test evidence and a digest of the tracked payload excluding the record itself; receipt-only commits avoid a self-referential final SHA. Private events still record actual commit and PR (null before one exists); post-push events record final SHAs/PRs privately.

Compose existing hooks including pre-push stdin and other hooks. Pre-push and complete run strict spec validation, current evidence checks, claimed/closed bead linkage and Dolt commit/pull/push. Public hosted CI reruns hygiene/spec/tests and validates sanitized receipt structure plus payload binding; it cannot authenticate private wiki truth. Trusted completion requires local durable acknowledgement produced only after note/index readback. Fork PRs can carry pending journal state; a maintainer must finalize in a trusted environment before stage acceptance. No private credentials or personal runners in PR CI.

Deterministic event identity is repo+stage+kind+sequence. Events are immutable, retry is idempotent, independent event files support parallel stages; adapter serializes shared index writes. Private outbox persists before network calls, and adapter failures leave checkpoint incomplete. Never fabricate a verified receipt.

## Risks / Trade-offs
Branch protection must require the CI check; adding a workflow does not enforce merging. Public digest provenance is revision binding, not a signature or wiki attestation. A trusted adapter is code execution and must be configured outside the repository. Tests cover adapter failure and retries with doubles; the actual connector is exercised separately. External editors require coordination beyond the adapter's local index lock.

## Migration Plan
Install hooks explicitly, run test, record start and completion, commit, run complete, push review branch and open PR. Do not archive proposals or claim owner acceptance before review. Intermediate commits can have pending journal state. Exceptions require an owner-reviewed scoped record with reason, affected check, expiry and follow-up Bead; never permit secrets, fabricated evidence or an unverified journal to appear verified.

## Open Questions
Merge protection is currently absent; owner/repository policy must require Engineering / checkpoint and review. Product tests remain a later stage obligation, not waived success.
