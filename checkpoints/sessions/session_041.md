# checkpoint_045 — Eight-zone wireless controls and ECMP source correction

Checkpoint045: eight-zone/100-device corrected wireless controls: 1224/1224 delivered, 1220/1224 deadline-met.

Architecture:70 stationary wireless robots (32 NR,38 Wi-Fi),24 zone infrastructure devices,2 spines,3 shared EPC support nodes,1 external server=100 total.306 tasks/case. Every robot is an active source in every phase. JSON defines unequal counts. Default64-local-device guard retained; explicit --scale-control allows at most128 local devices and300s/native run. No mobility or wired robot substitution.

Fixed AccessEcmp route source selection: default-constructed IPv4 source must be replaced using IsInitialized(), not just IsAny(). The independent audit rejects unassigned sources. Historical ECMP traces with102.102.102.102 are preserved but superseded as source-valid routing evidence. Ten older ECMP runs and the first100-device probe were reviewed; receipts are in .recovery/access-eight-zone. Shortest-path evidence and original four-zone ARP diagnostic remain valid. Old ECMP fixture preserved separately; current fixture uses corrected small control. No radio/ARP/deadline tuning.

All nine accepted controls, exact replays, matched comparison, times and deadline misses are documented in docs/access-eight-zone-control.md and .recovery/access-eight-zone/validation-summary.json. Access51/51 and core21/21 tests PASS. No active process. No Git commit/push. Source manifest checkpoint_045; separate recovery package checkpoint_045-eight-zone.zip. Raw rejected and accepted records retained.

Limits: two-seed100-device comparison, not full eight-case matrix or500-device capacity validation. L3 hash groups GTP tunnels; upper TSN/shim/backhaul, cryptography, mobility and protocol convergence remain outside model. No calibrated saturated workload or latency guarantee. New eight-zone misses need a separate causal diagnosis.

Next exact action: Prepare a bounded runtime/capacity probe for the next size before increasing the explicit128-local-device ceiling; retain100-device matched controls, cold-start misses and authorization/boundary regressions. Do not claim500-device wireless completion.

## Corrected validation results

| Result directory | Policy | Seed/run | Devices (including server) | Offered | Delivered | Deadline met | Max full task ms |
|---|---|---|---:|---:|---:|---:|---:|
| `access-auth-20261107T113553Z-29c5e23a` | ecmp / allow | 1/1 | 20 | 36 | 36 | 36 | 45.002193 |
| `access-auth-20261107T113638Z-73a76d46` | ecmp / allow | 1/1 | 100 | 306 | 306 | 306 | 49.843427 |
| `access-auth-20261107T114049Z-d969ca00` | shortest-path / allow | 1/1 | 100 | 306 | 306 | 306 | 49.843427 |
| `access-auth-20261107T114519Z-5af5e60f` | ecmp / allow | 2/1 | 100 | 306 | 306 | 304 | 978.502990 |
| `access-auth-20261107T163513Z-6b2523d9` | shortest-path / allow | 2/1 | 100 | 306 | 306 | 304 | 978.502990 |
| `access-auth-20261107T164052Z-3ec428ee` | ecmp / allow | 1/2 | 50 | 144 | 144 | 143 | 979.171427 |
| `access-auth-20261107T164220Z-f47868df` | ecmp / deny | 1/1 | 20 | 36 | 30 | 30 | 16.099259 |
| `access-auth-20261107T164240Z-5536692d` | ecmp / timeout | 1/1 | 20 | 36 | 30 | 30 | 16.099259 |
| `access-auth-20261107T164300Z-3567b65b` | shortest-path / allow | 1/1 | 20 | 46 | 43 | 43 | 45.001054 |

The four 100-device cases offer 1224 tasks: 1224 delivered and 1220 meet their 100 ms deadlines. Every case has byte-identical replay. Every 100-device case has all 70 sources delivering in every phase. Paired effective inputs differ only in routing policy after normalizing omitted seed1/run1 to the documented defaults.
Per-task measurements identical within each seed: [{'seed': 1, 'effectiveInputDifference': 'fabric.routing; omitted seed1/run1 normalized to documented defaults', 'taskMeasurementsIdentical': True}, {'seed': 2, 'effectiveInputDifference': 'fabric.routing; omitted seed1/run1 normalized to documented defaults', 'taskMeasurementsIdentical': True}].

This is a two-seed, four-case comparison, not the full eight-case multi-seed matrix. Denied or timed-out tasks remain in the offered denominator; the boundary case intentionally retains observed losses.

### Delivered tasks that missed their deadlines

- `access-auth-20261107T114519Z-5af5e60f` / `before-F-nr-3`: full 978.502990 ms; authorization None ms; payload 978.50299 ms.
- `access-auth-20261107T114519Z-5af5e60f` / `before-F-wifi-peer`: full 858.861144 ms; authorization None ms; payload 858.861144 ms.
- `access-auth-20261107T163513Z-6b2523d9` / `before-F-nr-3`: full 978.502990 ms; authorization None ms; payload 978.50299 ms.
- `access-auth-20261107T163513Z-6b2523d9` / `before-F-wifi-peer`: full 858.861144 ms; authorization None ms; payload 858.861144 ms.
- `access-auth-20261107T164052Z-3ec428ee` / `before-A-wifi-2`: full 979.171427 ms; authorization 976.691266 ms; payload 2.480161 ms.

The earlier four-zone ARP diagnostic remains applicable to its original shortest-path result. New eight-zone delays have not been attributed to a specific PHY/MAC cause; do not infer a cause from duration alone. No caches were prewarmed and no deadlines were relaxed.

### Runtime and checks

| Result | Original seconds | Replay seconds |
|---|---:|---:|
| `access-auth-20261107T113638Z-73a76d46` | 103.03 | 103.79 |
| `access-auth-20261107T114049Z-d969ca00` | 106.19 | 152.73 |
| `access-auth-20261107T114519Z-5af5e60f` | 174.29 | 179.15 |
| `access-auth-20261107T163513Z-6b2523d9` | 176.61 | 146.26 |

Wall times depend on this host and its load; they are not simulated packet delays or a 500-device runtime prediction. Access tests: 51/51 PASS. Core verification: 21/21 PASS. The first Windows test invocation was blocked by temporary-folder permissions; the normal-access rerun passed. Upstream 729-test validation was not rerun this milestone.

Next: prepare a bounded capacity/runtime probe before increasing device counts further. Retain the corrected 100-device controls and explicit deadlines. The full 500-device wireless workload, its visualization integration and final wireless report remain unfinished.
