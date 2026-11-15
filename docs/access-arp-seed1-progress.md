# ARP sensitivity: completed seed1 results

14 November2026. Eight stationary zones,300 devices,906 offered tasks,ECMP,
radio seed1/run1,100ms task deadlines,1000ms authorization timeout,cold caches.

| ARP wait(ms) | Delivered/906 | Deadline met/906 |
|---|---:|---:|
|1000|905|901|
|100|906|904|
|50|906|906|
|20|906|906|

All four completed cases have byte-identical native original/replay traces and
device CSV, with successful outcome,authorization,coverage and ECMP audits.
The baseline retains native exit2 and its authorization timeout; other cases
retain native0. MEASURED validates accounting and replay, not a deadline guarantee.
The corrected-observer1000ms and100ms rechecks match the earlier traces exactly.

- 1000ms: `results/access-auth-20261114T081820Z-8bca2400`
- 100ms: `results/access-auth-20261114T085336Z-f2ac9a71`
- 50ms: `results/access-auth-20261114T071931Z-63e8482d`
- 20ms: `results/access-auth-20261114T074323Z-98e1b584`

The initial50ms attempt access-auth-20261114T053131Z-f28a8a1a crashed in our ARP
observer. ArpL3Protocol also emits Drop for received ARP replies after removing
their header; the observer previously assumed IPv4 data. Bounds and header checks
corrected the unsafe read. The corrected50ms trace preserves all26250 bytes
preceding that assertion, then completes and replays. The failed run is retained.

The50ms and20ms results show zero deadline misses for this one finite run.
Seeds2/3 and ARP overhead remain pending. No candidate is adopted as default.
The planned further matrix is seeds2/3 run1 at1000/100/50/20ms,each with replay,
with the same offered tasks,model and deadlines. Failures stop the batch and
retain logs.500-device validation and calibration remain unfinished.
