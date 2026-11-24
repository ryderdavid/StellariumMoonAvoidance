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

### Requirement: Source-observed MoonAvoidance boundaries
The baseline SHALL distinguish inspected production formulas and plugin metadata from unverified numerical correctness, loader discovery and binary compatibility. See `.engineering/baseline.md` for the audited source SHA and paths.

#### Scenario: Inspected radius calculation
- **WHEN** reading production calculateSeparation, calculateWidth and calculateCircleRadius
- **THEN** altitude relaxation uses inclusive min/max bounds and the radius code uses days from full moon in a Lorentzian factor, with nonpositive separation handling and a one-degree floor; these are source observations, not independently executed astronomy acceptance

#### Scenario: Shared library is built or packaged
- **WHEN** inspecting Qt plugin metadata, exact Qt6.8.3 configuration, macOS dynamic_lookup and MSVC FORCE:UNRESOLVED
- **THEN** successful stock-binary loading, symbol resolution and ABI compatibility on Windows/macOS/Ubuntu remain unknown until installed-binary evidence is obtained

#### Scenario: Existing product tests
- **WHEN** inspecting root CMake and the two Qt Test sources
- **THEN** the root does not add tests, the proposed test executable has two QTEST_MAIN sources and its radius helper differs from production; no passing product tests, coverage or numerical correctness is inferred
