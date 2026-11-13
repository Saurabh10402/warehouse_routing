# checkpoint_033 — Agreed architecture replaces old experimental topology

User clarified responsibility: Layer3 stationary NR/Wi-Fi robots/access and
Layer2 spine routing only. TSN/shim/upper5G/main-server internals external.
Protected TASK TYPES listed in JSON always request MAIN SERVER authorization,
intra/interzone alike. After approval data local; deny/timeout blocks.
Direct robot radio deferred. Selected direct zone gateway links accepted;
redundant spines and ~500-device scope retained. No zone edge servers.

Authoritative spec: docs/agreed-access-architecture.md. Read BEFORE implementation.
New configs warehouse_access_small.json and warehouse_access_500.json DESIGN ONLY:
19 and500 local modeled nodes, 8 and471 robots. No edge servers. Main-server
external-service abstraction outside local count. Old configs/results preserved.
Access gateway direct links explicitly listed; wired baseline, two spines.
Cellular gateway/core placement explicitly PENDING user-plane design. Do NOT
reuse direct gNB/core shortcuts as approved final architecture. No new simulator
implementation this milestone. Required support core outside research task but
actual packet routes/delays must be modeled and audited. Example task names and
external-service timing are illustrative, not requirements.

scripts/access_study.py validates config/task authorization classification.
tests/test_access_study.py 4/4 PASS; scripts/verify.py21/21 PASS.
Manifest checkpoint_033. No active process, no Git upload.
Next: resolve cellular user-plane placement/topology within agreed Layer2/3
scope; then small topology and per-task authorization state machine, followed
by packet/path/timing audit. Preserve prior ARP shortfall as historical evidence.
