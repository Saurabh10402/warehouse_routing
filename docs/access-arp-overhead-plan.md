# Declared ARP overhead measurement protocol

The completed latency study leaves50ms and20ms tied on task success. Before
selecting a default, measure the control-traffic cost across all12 matched
300-device cases (seeds1/2/3 run1,wait1000/100/50/20ms).

Use an isolated diagnostic executable with read-only counters; keep the maintained
model and task traces unchanged. Count ARP requests/replies at each IPv4 interface
transmission boundary, separating Wi-Fi and wired interfaces. Count at one defined
boundary per metric to avoid double counting. Distinguish protocol send attempts
from actual Wi-Fi MAC/PHY transmissions and retransmissions. Frame byte counts
must state included headers; call neither IP-layer bytes nor PHY payload bytes
channel airtime. If airtime is reported, record actual PHY duration and aggregation
handling explicitly. Do not count receiver copies as extra transmissions.

Include the entire simulated interval and all nodes/interfaces rather than only
the earlier1.2–3.4s diagnostic window. Count unsolicited/discarded ARP replies
separately from pending IPv4 packet drops. The Drop callback can expose an empty
packet after ARP-header removal; never deserialize it as IPv4 without validation.
Aggregate by node,zone,access type,operation and outage phase,with total checks.

Before expensive runs, use a small single-resolution control and repeated sends
to verify expected request/reply and cache-reuse counters, plus safe handling of
empty/non-IP observer inputs. Demonstrate byte-identical native task trace and
device CSV with observers enabled. For each large diagnostic, require full trace
and CSV equality with its completed measurement before accepting counters; retain
source/executable/library hashes,raw counters and exact arguments. Logging must
not schedule events or draw random numbers. Apply bounded execution and preserve
failed/mismatched evidence. No dependency/model changes as part of observation.

Compare per-seed costs alongside delivery/deadline outcomes,not just pooled
totals. Lower mean traffic from three seeds alone is not a universal optimum.
Keep the1000ms baseline and explicit candidate setting in reproducible JSON.
This protocol is prepared; overhead instrumentation and measurements are pending.

## Initial observer validation

Isolated diagnostic executable records Wi-Fi MacTx offered
ARP requests/replies across all Wi-Fi devices and the full simulation interval.
Rows contain timeNs,node,operation,LLC+ARPbytes,sourceIPv4,destinationIPv4.
This is MAC-offered traffic,not actual PHY transmissions,retransmissions or airtime.
Point-to-point fabric does not use ARP; wired/EPC ARP boundary inventory and PHY
measurement remain to be validated. No maintained native model/dependency edits.

Parser self-test: two valid Ethernet/IPv4 ARP operations,36bytes each; empty,
truncated and non-ARP packets excluded. Small36-task baseline native0; task trace
and device CSV exactly match known-good access-auth-20261114T071641Z-44c76390.
Observed5requests+5replies,180LLC+ARPbytes each. Matching ns3 library hashes.
Executable hash5b6998fe860ff6b97d63444db4c325df124ddc13aa55a55963b7ebc275636dc6.
Receipt .recovery/access-arp/overhead/smoke/receipt.json; build/source/commands/raw
counter data retained beside it. This smoke is not a cache-reuse control or large
overhead study. Earlier21core/72access tests not rerun for isolated diagnostic.


## Cache reuse and PHY validation

Isolated PHY observer uses installed PhyTxPsduBegin callback
with WifiTxVector by value. The API callback typedef describes a const reference,
but actual trace uses value; v2 failed at trace connection and is preserved.
v3 matches actual signature. No maintained native model/dependency edits.

Three controls PASS: known-good36tasks,single cross-zoneWi-Fi send,and3repeated
sends200ms apart. Every task trace and device CSV exactly matches its maintained
counterpart. Known-good offered5requests/5replies; PHY12ARP MSDU transmissions.
Single and repeated controls both offer2requests/2replies and record5PHY ARP
MSDU transmissions,confirming cached address reuse. No retry-flagged or A-MSDU
ARP observed in these controls. PHY count can include AP forward transmissions;
it is not a retry estimate from offered-minus-PHY counts.

Interface inventory includes all ns3 nodes/devices; only Wi-Fi devices report
NeedsArp=true in this small control. Repeat for larger layouts before broad claim.
PHY traversal handles A-MPDU members and A-MSDU subframes in code,records retry
flag/sequence/A-MSDU indicator. Dedicated synthetic aggregation/retry tests remain
pending; do not launch large overhead study until those pass. Bytes are LLC+ARP
per MSDU,not complete MPDU/PPDU bytes or airtime. No observer scheduling/RNG.

Receipt .recovery/access-arp/overhead-v3/validation/receipt.json plus raw logs and
build metadata. Actual new tests:small native equivalence/cache controls; earlier
72access/21core tests not rerun for isolated diagnostic work. Executable hash
d72c79cbc9b6892d63a29d48544d585aa74296946542ca83c446bf54401cef1a.
No active simulation. No Git commit/push;handoff local. Source checkpoint057 and
separate recovery archive. Next: dedicated aggregation/retry observer tests,then
full12case overhead observation with exact task trace/device equality gates.

## Aggregation tests and full observation batch

Isolated observer synthetic tests pass in three small controls:
two A-MPDU members(request+reply),one retry-marked; one retry-marked A-MSDU
with request+reply subframes. Exactly4 ARP rows,3retry-flagged,2A-MSDU,36LLC+ARP
bytes each. Known-good/single/repeat simulations preserve task trace and device
CSV byte-for-byte. Cache reuse controls remain2requests/2replies. Matching ns3
library hashes. No native maintained model/dependency changes or default adoption.
Executable SHA cdf5e1fb9bb7dbc7cf55dbdb3f8cb9ef1f391c43501f36fe53d6a034509e8898.

Large observation worker: .recovery/access-arp/overhead-v4/large_worker.py.
Receipt .recovery/access-arp/overhead-v4/large/status.json.12cases,seeds1/2/3,
wait1000/100/50/20ms,sequential. One observer run per previously replay-validated
case,not a new task-study replay matrix. Requires native exit and full task/device
equality with corresponding original,matching libraries and only Wi-Fi NeedsArp
interfaces. Failure stops batch and retains logs. Each native execution1200s max.
Counters: MAC-offered vs PHY ARP MSDUs,LLC+ARPbytes,explicit retry flags,A-MSDU
indicators; aggregate by node/outage phase/operation. AP forwarding is counted
as transmission,not inferred retry. No airtime/complete MAC-frame-byte claim.
Received unsolicited ARP drop counts are outside these transmit metrics.

Evidence:overhead-v4/build receipt,validation/receipt.json and raw small controls.
Only isolated observer native tests rerun; earlier72access/21core tests preserved.
Source checkpoint058 and separate recovery snapshot. No Git commit/push; handoff
local. Next:monitor12case overhead observations; audit totals/equivalence,then
compare50/20ms per seed before selecting any explicit experiment preset.
500-device wireless validation remains pending. Do not duplicate simulations.
