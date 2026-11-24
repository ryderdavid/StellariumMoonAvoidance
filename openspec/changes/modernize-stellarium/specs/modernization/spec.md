## ADDED Requirements

### Requirement: Immutable upstream and reproducible promotion
The project SHALL resolve immutable upstream/toolchain/dependency identifiers, build natively on Windows x64/macOS ARM64/Ubuntu x64 and promote only the exact tested artifact.

#### Scenario: Immutable upstream and reproducible promotion checkpoint
- **WHEN** new stable release or master revision is detected
- **THEN** resolve immutable upstream/toolchain/dependency identifiers, build natively on Windows x64/macOS ARM64/Ubuntu x64 and promote only the exact tested artifact

### Requirement: Independent regression and feature evidence
The project SHALL run same-SHA vanilla versus feature-disabled differential regression, independent feature fixtures and measured coverage; keep unsupported claims unknown.

#### Scenario: Independent regression and feature evidence checkpoint
- **WHEN** compatibility is proposed
- **THEN** run same-SHA vanilla versus feature-disabled differential regression, independent feature fixtures and measured coverage; keep unsupported claims unknown

### Requirement: Installed graphical qualification
The project SHALL install and exercise real GUI input on all three native platforms with controlled fixtures, geometric visual assertions and renderer evidence.

#### Scenario: Installed graphical qualification checkpoint
- **WHEN** a package is proposed for release
- **THEN** install and exercise real GUI input on all three native platforms with controlled fixtures, geometric visual assertions and renderer evidence

### Requirement: Portable isolated maintenance
The project SHALL require isolated portable execution, approved costs, short-lived credentials, independent review and upstream-ready scoped changes.

#### Scenario: Portable isolated maintenance checkpoint
- **WHEN** a lab backend or agent proposes an upstream repair
- **THEN** require isolated portable execution, approved costs, short-lived credentials, independent review and upstream-ready scoped changes
