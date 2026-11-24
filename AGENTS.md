# Engineering operating contract

OpenSpec owns behavior/spec changes. Repository-local Beads owns work/dependencies. Git owns reviewed implementation/evidence. The private journal owns decisions/learning. Delegation transport is not a backlog. Read `.engineering/operating-contract.md` and `.engineering/baseline.md` before work.

Use an isolated feature branch/worktree. Preserve others' changes; no reset or force push. Run `bd prime`, inspect/claim the linked Bead, then read the OpenSpec proposal/design/tasks/scenarios. Beads 1.2.2 uses embedded Dolt with `bd dolt commit/pull/push`, not `bd sync`. Keep database internals out of Git. Do not create markdown TODOs; OpenSpec tasks are implementation traceability, not another backlog.

Install composed hooks with `python3 tools/engineering/lifecycle.py hooks`. `pre-commit` validates the index; `pre-push` and `complete` validate final evidence/sync and trusted journal acknowledgements. Never bypass a failing gate silently. Public PR CI repeats offline checks on GitHub-hosted runners. Do not run public PR code on personal runners or access the private journal from CI.

Every meaningful stage start/end, failure/blocker and key decision requires a deterministic private event and index readback. A failed write leaves a durable private outbox and incomplete stage. Public Git contains only sanitized receipt IDs/digests. Keep credentials, personal data, private paths and delegation references out of public Git. Configure adapters/outbox only outside this repository. Never retrieve journal secrets to work around an inaccessible connector.

Record actual command outcomes and distinguish source observations from executed behavior. Do not claim loader/ABI, geometry, GUI or cross-platform success based on source or compilation. No tests may be disabled. Product changes require applicable product evidence. Do not merge your own work automatically. Scope exceptions require owner review as documented; no exception can turn an unverified journal into a verified one.
