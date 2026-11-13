# checkpoint_037 — Native task authorization gate validated

Separate native runner src/access-task-control.cc with access-task-fabric.h and
access-authorization.h now executes configured task table. Prepare via
scripts/prepare_access_tasks.py. Existing peer diagnostic runners preserved.
Main-server behavioral model is an extra external node dual-homed to spines,
configured upper-path delay and server processing;19 local/20 actual nodes.
Real UDP requests/replies; only matching timely allow releases local payload.
No cryptography/TSN/shim/upper5G implementation claimed.

Small normal + protected same-zone + protected cross-zone tasks tested in allow,
deny,timeout modes, each original/replay exit0 and byte-identical traces/devices.
Allow3/3 delivered. Deny/timeout normal delivered, both protected tasks blocked
with0 payload. Independent audit_access_auth.py checks ordering, server spine
boundary, payload absence at server, timing arithmetic and blocking; tampered
approval order rejected. Full task times15.991259/41.449187/35.242855ms in allow.
All meet illustrative100ms deadlines; no performance guarantee/multiple-seed claim.
Late-response control processing1200ms versus timeout1000ms: both approvals
ignored after timeout and0 protected payload. Native allow-delivery result nonzero
expected for this negative control; protection assertion independently passes.
Project scripts/verify.py21/21 PASS. No processes active. No Git upload.

Evidence .recovery/access-auth including input tables, logs, audit.json, late/audit,
receipt hashes. Build/run helper .recovery/run_access_auth.sh. Docs in
access-authorization-control.md. Checkpoint037 source manifest.
Next consolidate reproducible task runner/capability limits, add live spine failure
and ECMP, complete radio/S5/application path/deadline audit and multiple seeds,
then500-node scaling and viewer. Config remains partial contract: routing/failure
fields are not yet implemented by task runner. Small tasks send1x256-byte payload.
