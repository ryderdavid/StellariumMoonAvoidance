## ADDED Requirements

### Requirement: Incremental staged linkage
The project SHALL validate staged hygiene, Bead/OpenSpec linkage and relevant strict specs without requiring a final journal receipt.

#### Scenario: Incremental staged linkage checkpoint
- **WHEN** a substantive commit is staged
- **THEN** validate staged hygiene, Bead/OpenSpec linkage and relevant strict specs without requiring a final journal receipt

### Requirement: Trusted stage completion
The project SHALL require current tests, version-correct Beads status/sync, verified start/completion events and private note/index readback acknowledgement.

#### Scenario: Trusted stage completion checkpoint
- **WHEN** a stage completes or pushes
- **THEN** require current tests, version-correct Beads status/sync, verified start/completion events and private note/index readback acknowledgement

### Requirement: Idempotent private journal
The project SHALL persist a deterministic immutable private event before delivery and publish only sanitized receipts after readback; offline attempts remain incomplete.

#### Scenario: Idempotent private journal checkpoint
- **WHEN** a start, completion, failure, blocker or key decision occurs
- **THEN** persist a deterministic immutable private event before delivery and publish only sanitized receipts after readback; offline attempts remain incomplete

### Requirement: Public PR boundary
The project SHALL rerun hygiene, strict specs and governance tests on hosted runners and verify revision-bound sanitized evidence without private access; external PR journal remains pending for trusted finalization.

#### Scenario: Public PR boundary checkpoint
- **WHEN** a PR is checked after bypassing local hooks
- **THEN** rerun hygiene, strict specs and governance tests on hosted runners and verify revision-bound sanitized evidence without private access; external PR journal remains pending for trusted finalization
