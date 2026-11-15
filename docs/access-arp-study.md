# Declared ARP retry sensitivity study

The 300-device baseline diagnosis motivates varying only ARP retry wait. The
baseline remains 1000 ms; experimental candidates are 100, 50 and 20 ms. All
candidates retain the installed maximum retry count, pending queue limit, cold
caches, radio settings, task schedule, 100 ms deadlines and 1000 ms authorization
timeout. Shorter waits may increase ARP overhead or exhaust retries sooner.

Use optional JSON `"arp": {"waitReplyTimeoutMs": 100}`. Omission retains 1000 ms.
The bounded integer range is 10..1000 ms. The setting applies to all newly created
ARP caches, including wired interfaces, rather than just Wi-Fi. It does not alter
NR neighbor resolution. Native CLI validation enforces the same bounds.

Before larger runs, verify omitted and explicit1000 ms small-control traces match
the known-good baseline exactly; verify experimental controls replay exactly and
retain authorization/outcome auditing. Native task traces deliberately keep their
existing format; the saved input and invocation identify the parameter.

The planned larger matrix is300 devices, seed1/2/3 run1, ECMP, waits1000/100/50/20 ms,
using outcome-measurement with every replay. Reuse the completed seed1 baseline
only when source changes reproduce its small baseline and model equivalence is
established; otherwise run a fresh baseline. Report every offered task, timeout,
late delivery, payload loss and phase/source coverage. No tested candidate is
promoted automatically. ARP overhead needs separate observer evidence; ordinary
task traces alone do not measure radio control traffic.500 devices remains pending.

## Small-control validation

14 November2026: omitted,explicit1000ms and20ms controls each delivered36/36
within deadline with exact replay. Omitted/1000ms traces and device CSV match the
prior baseline.72 access tests and21 core checks pass. Large candidate runs and
ARP overhead comparison remain pending; this is not evidence for a new default.
