# Brownfield baseline

## Purpose
Describe inspected source and build configuration at `9dbd53c969ae4b57c896953e563fcc61c4f8b338`. Source observations are not runtime acceptance. See `.engineering/baseline.md` for evidence and unknowns.

## Requirements
### Requirement: Evidence classification
The project SHALL distinguish source-observed behavior, executed evidence, intended changes and unknown runtime behavior.

#### Scenario: Unsupported runtime claim
- **WHEN** source or a workflow describes a feature without installed application evidence
- **THEN** the capability is recorded as source-observed and runtime-unverified

### Requirement: Preserve observed integration
The bootstrap SHALL preserve existing source, patch, test and release behavior while recording its limitations.

#### Scenario: Governance-only change
- **WHEN** the governance stage is submitted
- **THEN** no existing product source or test is disabled or replaced

#### Scenario: Incomplete baseline coverage
- **WHEN** test files exist without proven discovery and execution
- **THEN** their presence is recorded without claiming passing tests or coverage
