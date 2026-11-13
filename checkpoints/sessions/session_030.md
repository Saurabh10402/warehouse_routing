# checkpoint_034 — Cellular placement and expected paths specified

Selected delegated baseline: shared local EPC below Layer1, SGW/PGW dual-homed
to spines; gNB attached to zone gateway, routed S1 backhaul using
NrNoBackhaulEpcHelper/AddS1Interface. Pinned local helper source inspected.
No automatic direct gNB-core bypass permitted in new implementation.
Same-zone NR still traverses shared core/spines; no per-zone cellular breakout
claim. Wi-Fi direct zone links provide local path options. Scope excludes upper
TSN/shim/backhaul/main-server internals. Preserve these limitations explicitly.

Updated both access configs placementStatus and validator. Added access_paths.py
expected-path compiler and test_access_paths.py: 3/3 PASS. Contract tests4/4 PASS,
project verify21/21 PASS. Docs/agreed-access-architecture.md authoritative.
Native corrected architecture NOT IMPLEMENTED; expected paths not measurements.
No processes running, no Git upload. Source checkpoint_034.
Next implement explicit routed S1 using NoBackhaul helper in separate corrected
small runner, preserving historical controls. Then authorization state machine
and packet/path/timing audit. Do not treat design compiler as simulation.
