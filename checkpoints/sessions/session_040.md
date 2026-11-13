# checkpoint_044 — Four active zones /50 total devices validated

Added configs/access_four_zone_control.json:49 local+1 external devices,
32 stationary wireless robots (16 NR,16 Wi-Fi),4 gateways/APs/gNBs,2 spines,
3 shared EPC support nodes.144 tasks (48/phase), all32 robot sources in every
phase, mixed/same-access interzone and local tasks. Down3000ms/up5000ms.
64-local-device guard retained. No native model or dependency changes.

New audit_access_coverage.py checks actual CSV IDs/roles/radio types/attachments,
unique native IDs and aggregate node/task count, reports offered/sent/delivered
sources per phase and per-zone/access offered counts. Integrated into maintained
runner; general controls may intentionally use only a subset. Four-zone receipt
asserts all32 sources offered/delivered in every allow phase. Blocked tasks stay
offered but not counted as payload. Tests reject wired robot substitution etc.

Initial ECMP control results/access-auth-20261107T063414Z-3ad81450:144/144 delivered,
exact replay,1272 wire hops checked,296 equal-cost choices,228 restored mappings.
Main batch results/access-seeds-20261107T063613Z-8dbf345b:8/8 functional/replay PASS,
1152/1152 delivered, BUT1150/1152 met100ms deadlines. Preserve both misses.
Seeds1/2/3 run1 and seed1/run2, each shortest-path+ECMP. All32 robot sources
actually deliver in every phase/case. Raw inputs/logs/audits retained.

Deadline miss: task6 before-A-wifi-2, protected Wi-Fi A2 -> NR B2, seed1/run2
under both policies. Full979.171427ms; authorization976.691266ms; payload2.480161ms.
Same executable/config/seed diagnostic with only ARP logging reproduces entire
native stdout and device CSV exactly (.recovery/access-four-zone/arp-diagnostic).
AP queues approval at1413.992621ms with unresolved192.168.0.4. Initial request
1416.992621ms has no completed exchange; retry2356.118889ms, pending flush
2356.403224ms, approval2356.691266ms. ARP pending942.410603ms explains delay.
First-exchange PHY/MAC failure cause not established by ARP-only logs. No tuning,
cache prewarming or deadline changes.1000ms auth timeout permits release while
100ms task deadline is counted missed. Functional PASS is not all-deadline PASS.

Additional exact-replay controls:
- deny access-auth-20261107T111938Z-a86281cc:84 delivered,60 blocked/144 offered
- timeout access-auth-20261107T112034Z-1ee684ed:same counts
- direct gateway ring access-auth-20261107T112131Z-9b89519e:144/144 delivered/deadlines
- small regression access-auth-20261107T112246Z-46b7028e:36/36, raw trace+CSV equal
  checkpoint043 access-auth-20261107T062609Z-4595d670 exactly.
All results under results/. Deny/timeout retain all32 offered sources per phase;
only permitted normal tasks send payload. Ring case does not prove ARP-miss fix.

Validation:46/46 access tests, scripts/verify.py21/21 PASS. No C++ or dependency
rebuild changes. No process active. No Git commit/push. Documentation in
 docs/access-four-zone-control.md and authorization guide. Source checkpoint_044;
separate checkpoint_044-four-zone.zip recovery package. Prior data preserved.

Limits: small staggered load, shared-radio model uncalibrated, no movement,
crypto/upper TSN/shim/5G implementation, protocol convergence, inner-GTP ECMP
or500-device wireless capacity claim. Known deadline misses must stay visible.

Next exact action: prepare an explicit eight-zone layout around100 modeled
devices with all-zone/access source traffic. Assess runtime before changing the
64-device guard; enable only a bounded validated scale-control path, not silent
500-device acceptance. Retain small/four-zone regressions and cold-ARP misses.
Investigate first-exchange PHY/MAC loss separately if optimizing latency; do not
hide cold-start behavior by silently warming caches or relaxing deadlines.
