# Brownfield audit — 2026-11-24

Both fresh source checkouts were clean. No existing AGENTS, OpenSpec or Beads store was found in either repository. Existing sources/workflows/tests were preserved. Work uses isolated `engineering/governance-bootstrap` branches. OpenSpec 1.15.0 and Beads 1.2.2 were executed; Beads initialized embedded Dolt and configured each existing GitHub repository as its Dolt Git remote. Database internals remain local. GitHub API returned `Branch not protected` for both main branches; no protection is claimed.

Upstream API resolved latest stable v26.3 (published 2026-09-28) and master 3863ab071ae08737c00832bfdd00c40279ad055a at audit time. These are observations, not new support claims. Existing configuration remains at v25.3. Future upgrades are tracked in `modernize-stellarium` and Beads, not implemented here.

## MoonAvoidance at 9dbd53c969ae4b57c896953e563fcc61c4f8b338

Source-observed: C++17 shared library using Qt6 Core/Widgets and Stellarium headers. qt_version.conf pins exact 6.8.3. Qt plugin metadata/getStelModule create MoonAvoidance; plugin.ini and install scripts exist. CMake requests macOS dynamic_lookup and MSVC /FORCE:UNRESOLVED. These linker flags are not evidence that stock Stellarium exports matching symbols or can safely load the library. Dynamic loading and ABI are unverified on every platform; integrated packaging may be necessary.

The Windows workflow (windows-build.yml, not build.yml) resolves ci/stellarium-compat.yml to v25.3, Qt 6.8.3/MSVC2022, builds and packages a DLL. It publishes a main-branch nightly artifact. It runs no tests. Its existing contents:write token scope and unpinned dependencies are baseline limitations for later least-privilege/reproducibility work. No existing workflow uses personal runners.

Production source computes altitude relaxation inside inclusive min/max bounds, adjusts width, and uses days from full moon in a Lorentzian radius with a one-degree lower bound after nonpositive separation handling. These are inspected formulas, not verified astronomy behavior. Tests include helper formulas that differ from production (testCircleRadius uses a phase multiplier) and two QTEST_MAIN definitions in one proposed executable. The root CMake does not add the tests subdirectory; tests CMake requires a Stellarium::StelCore target not established by the root. Independent fixtures, buildability/discovery and coverage are unknown.

Executed: source, CMake and workflow inspection only for product behavior. No product configure/compile/load/GUI success is claimed. The governance suite is separate from the existing product tests and leaves them intact.

Unknown: stock-binary loader discovery, Qt/compiler/ABI/symbol compatibility, generated Stellarium headers, native packaging, numerical correctness, runtime null safety, settings and rendered zones on Windows x64/macOS ARM64/Ubuntu x64. Next: prove or reject loader/ABI assumptions on pinned upstream before promising cross-platform dynamic support.
