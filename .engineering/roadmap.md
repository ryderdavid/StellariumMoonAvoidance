# Beads roadmap references

This is an ID map, not a second task board. Status and dependencies live in repository-local Beads. OpenSpec `modernize-stellarium` defines proposed behavior; all future work remains unimplemented.

- epic: `moon-7b2`
- bootstrap: `moon-7b2.1`
- upstream: `moon-7b2.2`
- build: `moon-7b2.3`
- regression: `moon-7b2.4`
- coverage: `moon-7b2.5`
- gui: `moon-7b2.6`
- lab: `moon-7b2.7`
- agent: `moon-7b2.8`
- upstream_pr: `moon-7b2.9`

Cross-repository external dependency references point to the peer GUI/bootstrap capabilities. Resolve them against the peer local board before lab or maintenance work. No provisioning is authorized here.

Cross-project capability targets carry `export:<ID>` labels. Only after closure run `bd ship <ID>`; `provides:` labels are reserved. Register peer checkout roots under `external_projects` in private `.beads/config.local.yaml` (never commit machine paths). Unavailable/unshipped peers remain blocking.
