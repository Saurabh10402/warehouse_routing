# 300-device wireless outcome and address-resolution diagnosis

## Measurement

The stationary eight-zone configuration contains 270 robots (128 NR and 142 Wi-Fi),
24 zone infrastructure nodes, two spines, three EPC nodes and one external server.
The run uses real ns-3 Wi-Fi and 5G-LENA access, seed 1/run 1 and ECMP. All 270
robots offer traffic in each of three phases; spine 0 is unavailable from 3 to 5 s.
The task deadline remains 100 ms and authorization timeout remains 1,000 ms.

| Quantity | Result |
|---|---:|
| Offered tasks | 906 |
| Delivered | 905 |
| Deadline met, out of all offered tasks | 901 |
| Delivered after deadline | 4 |
| Authorization timed out; no payload sent | 1 |
| Explicitly denied | 0 |
| Payload sent but unreceived at stop | 0 |

Result `results/access-auth-20261113T101202Z-025cb365` has status **MEASURED**,
validation **PASS**, with native exit **2** retained. Its original and replay
are byte-identical, and both match the earlier failed 300-device run. This validates
the measurement and accounting, not universal delivery or deadline compliance.
The earlier strict result remains FAILED. Native original/replay took 894.978 s
and 838.432 s respectively, each within the 1,200 s limit.

The ECMP audit checks 8,694 wire hops, 2,020 equal-cost selections and 1,596 restored
flow mappings. Both spines are used before/after the outage; only spine 1 during it.
All robots offer tasks in every phase; the first phase has 269 delivered sources,
and the other phases each have 270. The blocked task remains in every denominator.

## What ARP does

Address Resolution Protocol (ARP) maps an IPv4 address to a link-layer address.
Before an access point can send an IP packet to a Wi-Fi robot whose mapping is
unknown, it broadcasts an ARP request and holds the pending IP packet. The robot
replies, allowing the AP to release queued traffic. A missed request can therefore
delay an authorization reply or a payload even when the IP route is valid.

## Observed frame and ARP timeline

Times below are simulation milliseconds. The PHY drop reason for each initial
request is `BUSY_DECODING_PREAMBLE`: the receiver is already decoding a frame's
preamble. The overlapping observations are ARP traffic within the same zone.

| Event | Zone A, robot 0 | Zone H, robot 0 |
|---|---:|---:|
| AP starts waiting for mapping | 1304.627335 | 2301.679230 |
| Initial request transmitted over Wi-Fi | 1309.818133 | 2305.811800 |
| Initial request dropped at target robot | 1309.822132 | 2305.815791 |
| Retry transmitted over Wi-Fi | 2304.636133 | 3274.652133 |
| Retry reception ends at target robot | 2304.748176 | 3274.764176 |
| AP receives ARP reply and flushes pending traffic | 2304.915219 | 3274.931219 |

At the first drop, zone-A robot 0 begins receiving another ARP request from
192.168.0.16. Zone-H robot 0 begins receiving an ARP reply from 192.168.7.17 at its
first drop. Packet UID, transmitter MAC address, sequence number and ARP target
correlate each AP transmission with the target robot's observations.

The installed ArpCache implementation uses a cache-level wait timer; it starts a
timer only when none is pending and scans waiting entries on expiry. Thus the
waiting interval for a newly added entry need not be exactly one second. The logs
show approximately 1000.288 ms waiting in A and 973.252 ms in H. Reception-end
events alone are not treated as proof of successful decoding: the ARP reply and
pending-queue flush confirm completion of address resolution.

## Effect on tasks

| Task | Affected exchange | Observed outcome |
|---|---|---|
| 16, before-A-wifi-0 | Approval returning to A Wi-Fi robot 0 | Server allows at 1294.426423 ms; client times out at 2275.2 ms; late approval at 2305.269791 ms is ignored; no payload |
| 33, before-A-local-protected | Payload to A Wi-Fi robot 0 | Received at 2305.627262 ms; full delay 950.527262 ms |
| 226, before-G-nr-0 | Payload to H Wi-Fi robot 0 | Received at 3275.512108 ms; full delay 1013.312108 ms |
| 280, before-H-wifi-0 | Approval returning to H Wi-Fi robot 0 | Approved at 3275.599629 ms; payload received at 3278.671427 ms; full delay 762.671427 ms |
| 298, before-H-local-protected | Payload to H Wi-Fi robot 0 | Received at 3275.966262 ms; full delay 675.366262 ms |

The late task timestamps follow release of the corresponding pending ARP queue.
Authorization protection is working: task 16 does not send a payload after timeout.
The observed chain is missed Wi-Fi ARP request, delayed address resolution, then
late approval or payload. This is a modeled access-layer finding, not evidence
that NR access is replaced with wires, nor a calibrated deployment claim.

## Diagnostic integrity and retained evidence

The 20-node smoke trace and CSV matched exactly. The complete 300-device diagnostic
also matched the earlier native trace and CSV byte-for-byte, with matching library
hashes and native exit 2. It finished in 658.246 s. Its executable SHA-256 is
`20efab1a6338384ea4f9e58c084578eaef791d924f62f2dbfe17af83becc713d`.
The worker has completed; no simulation remains active from this sequence.

The diagnostic is an isolated executable derived from the maintained native
source, adding read-only frame observers and ARP logging. It does not change the
maintained executable, radio parameters, event scheduling or random draws. A
20-node smoke control must match its known-good native trace and device CSV;
the complete 300-device diagnostic must match both original files as well.
The diagnostic preserves native exit 2 rather than relabeling the delivery failure.

Evidence lives in `.recovery/access-300-resume/`: measurement receipt, build receipt
and compiler commands, smoke receipt, full diagnostic receipt and raw native/ARP/
Wi-Fi logs, plus `frame-analysis.json`. Prior interrupted attempts are retained
under `.recovery/access-300-measured/` and the first timed-out broad trace remains
under `.recovery/access-300/`. No result or manifest has been overwritten.

## Limits and next experiment

This is one configuration and one radio seed. It establishes a reproducible
failure mechanism for this run, not a packet-loss rate or a warehouse capacity
guarantee. The focused frame window is 1.2–3.4 s. Upper TSN/shim/backhaul behavior,
mobility, authentication cryptography and distributed route convergence remain
outside this study. The 500-device wireless case is still unvalidated.

Next, predeclare a controlled address-resolution sensitivity study: retain this
unchanged baseline, vary the ARP wait/retry policy as an explicit experimental
parameter, and compare delivery, all-offered deadline outcomes, authorization
timeouts and ARP overhead across seeds. Shorter retries may increase contention;
no setting should be adopted from a single favorable run. Do not silently prewarm
caches, lengthen deadlines/timeouts or bypass approval to remove failures. Review
the runner and native task/device limits before proceeding to the 500-device case.
