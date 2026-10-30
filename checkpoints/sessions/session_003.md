# Session 003 â€” declared traffic and protected queue comparison

User authorized continuing the completion steps. Immediate scope implemented: an explicit
experimental traffic/acceptance contract and a protected queue comparison against the saved
baseline. No measured physical warehouse traffic was supplied; assumptions stay labeled.

Added optional protectedPriority: P0 reserved waiting slots, aggregate per-direction token
bucket, self-clocked byte-weighted P1/P2/P3 service. Added queue_stats accounting and explicit
policed/reservation loss outcomes. Default FIFO/strict policies unchanged; EALR remains legacy.
Tested implementation commit 2d0dced. Both core presets pass 19/19. Full protected 144 and baseline 132
runs pass; all 132 old traces reproduce exactly. Replay and 40/55 charts pass. Later source
changes are docs only; no parameters were retuned after results.

Key result: cross-zone P0 destination-bottleneck misses 41.25% -> 6.25%, but emergency-overload
P0 misses 1.5% -> 25.5625% with ECMP protected. The always-on 20% cap rejects P0 even without
competing traffic on a slow endpoint link. Do not promote this configuration to a default.
Next: preregister capacity/admission sensitivity and per-port workload envelopes. Preserve
admission losses in offered-traffic denominators. ns-3/NR, calibrated physical inputs and
cross-platform execution remain unfinished. See checkpoint_006 and the results report.
