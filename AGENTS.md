# Project continuation instructions

Before work, read checkpoints/CURRENT_STATUS.md, PROJECT_STATE.json, NEXT_STEPS.md and the
latest sessions entry. Inspect Git status and verify the named source manifest using
`python scripts/checkpoint.py CHECKPOINT_ID --verify`. Investigate mismatches; do not reset
or delete user changes. Keep original archives, previous manifests and known-good results.

Primary scope is the schema-v2 **stationary** warehouse network. No random movement in the
primary runner. Zones and endpoint counts are configuration data; ten equal zones is only
a reference example. Preserve legacy APIs/examples but clearly label their different scope.

The approved direction now emphasizes powered infrastructure, peer/service communication,
priority scheduling and failure resilience. Default experiments compare shortest path/ECMP
and FIFO/strict priority. Preserve EALR as legacy work; do not restore battery-aware relay
routing to the primary study without a relevant architecture and explicit user direction.
Read docs/warehouse-study.md before extending the primary experiment matrix.

Run `python scripts/verify.py` after coherent code changes. Use `scripts/run_all_experiments.py`
for full validation; a quick run is not evidence of multiple-seed evaluation. Never call core
abstraction results ns-3/NR measurements or configured deadlines guaranteed performance.

Checkpoint meaningful milestones, actual tests, limitations and exact next action. Preserve
raw experiment data and provenance. Commit source when authorized; generated results need a
separate recovery package. Never include credentials, large toolchains or build trees in Git.
