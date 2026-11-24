## Context
The baseline is source-observed. Runtime claims, ABI safety and mathematical correctness need independent evidence. The target matrix is Windows x64, macOS ARM64 and Ubuntu x64.

## Goals / Non-Goals
Propose reproducible testing and promotion. No implementation, infrastructure provisioning or product claims in bootstrap.

## Decisions
Resolve latest stable and master to immutable SHAs; use a release-blocking stable lane and a tracked master canary. Promote the exact installed/tested package and provenance without rebuilding. Compare vanilla versus feature-disabled patched upstream on the same SHA. Use independently derived math fixtures and meaningful coverage (proposed 95% feature lines, 90% branches, 95% changed upstream lines, not measured). Qualify installed GUI with real input, controlled settings/time/location, geometric screenshot checks, renderer/JUnit/video evidence. API-only checks are insufficient.

Start GUI qualification alongside build work after its prerequisites. Qualify GitHub-hosted sessions before AWS/local backends. No public PR on personal runners; isolate trusted lab runs, use ephemeral credentials/OIDC and restore/destroy. Separate per-repository features from optional combined-install tests. Cross-repository Beads edges coordinate lab and maintenance without duplicate backlog.

## Risks / Trade-offs
Moon loader/ABI may need integrated packaging; unresolved symbols do not prove safe loading. Mosaic changes existing Oculars rendering paths, so feature-disabled regression is critical. VM software OpenGL does not prove GPU performance. Cloud cost and platform suitability remain unknown.

## Migration Plan
Follow Beads dependencies, one reviewed proposal per bounded change; require independent review and evidence before upstream PRs. Owner review determines coverage exceptions and cloud spend. Preserve existing release behavior until replacement promotion is proven.

## Open Questions
Which supported stock binaries can load MoonAvoidance? Which Oculars paths regress? Which native GUI backends expose adequate renderers and interactive sessions?
