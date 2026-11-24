# Repository Beads board

Beads 1.2.2, embedded Dolt. Linked worktrees discover the main checkout board automatically. Initialize new clones with `bd init --skip-hooks --skip-agents --non-interactive` and pull the Dolt remote before creating work. Run `bd dolt commit`, `bd dolt pull`, `bd dolt push`; no `bd sync` exists in this version. Database internals, backup files and machine metadata stay outside Git. The Git transport stores Dolt data on its own ref; JSONL is interchange, not sync. See the operating contract.
