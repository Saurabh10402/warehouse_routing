# Completed ARP overhead comparison

14 November2026.12 observer runs cover seeds1/2/3,run1,300devices and waits
1000/100/50/20ms. Each full task trace/device CSV matches its previously replayed
reference. Raw counters and node/phase/operation totals were independently
recounted and match. No A-MSDU ARP was observed in these large runs.

| Wait(ms) | MAC-offered ARP MSDUs | PHY ARP MSDU transmissions | Retry-flagged PHY MSDUs |
|---|---:|---:|---:|
|1000|1720|2224|30|
|100|1727|2235|34|
|50|1786|2282|22|
|20|1930|2438|34|

| Seed |50ms PHY ARP MSDUs|20ms PHY ARP MSDUs|
|---|---:|---:|
|1|765|824|
|2|756|805|
|3|761|809|

50ms records2282 PHY ARP MSDUs versus2438 at20ms,about6.4% fewer. It is lower
in each tested seed. Both deliver2718/2718 tasks within deadline in the matched
task study. Thus50ms is selected as an explicit next-stage experiment preset;
the omitted/global default remains1000ms. This finite workload does not establish
an optimal or guaranteed industrial setting.50ms still has more ARP traffic than
the1000ms baseline,which misses deadlines.

Counts are ARP-bearing MSDUs at MAC-offered and PHY-transmission boundaries.
AP forwarding contributes additional transmissions; retry counts use the actual
retry flag rather than subtracting offered counts. Bytes are LLC+ARP bytes per
MSDU transmission,not full frame size or channel airtime. Received unsolicited
ARP reply drops are not measured by this transmit-only comparison. Every node's
interface inventory confirms only Wi-Fi devices need ARP in these configurations.
See `.recovery/access-arp/overhead-v4/large/final-audit.json` and per-case receipts
for hashes,arguments,raw traces and counters. Nothing from this batch is running.
