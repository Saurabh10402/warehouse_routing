# Eight-zone intermediate-scale wireless control

`configs/access_eight_zone_control.json` defines eight stationary zones and100
modeled nodes:99 local devices plus the external main-server model.

| Devices | Count |
|---|---:|
| NR robots (4 per zone) | 32 |
| Wi-Fi robots (4 in A/B,5 in C–H) | 38 |
| Zone gateways, Wi-Fi APs and NR gNBs (one each per zone) | 24 |
| Shared spines | 2 |
| Shared EPC support nodes | 3 |
| External main-server model | 1 |
| **Total** | **100** |

All70 robots offer tasks in each of three phases. The slightly unequal zone counts
are explicit JSON data. There are102 tasks per phase,306 overall, covering normal
and protected same-zone/interzone traffic and both access technologies. Tasks
start every15ms from1200/3300/5300ms; spine0 goes down at3000ms and returns at5000ms.
The main control has no direct zone links. Robots remain actual NR/Wi-Fi devices.

## Explicit scale-control invocation

```sh
python3 scripts/run_access_authorization.py configs/access_eight_zone_control.json --scale-control --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

Use the existing pinned Linux/WSL ns-3/NR installation. The default64-local-device
guard is unchanged. `--scale-control` explicitly permits at most128 local devices
and bounds each native execution to300 seconds instead of180. This is an
intermediate validation ceiling, not validated capacity and not500-device support.
`execution-timing.json` records elapsed native wall time separately from simulated
latency. Original/replay traces remain deterministic even when wall times differ.

The seed-comparison command also accepts `--scale-control`, but its default eight
cases take substantially longer at this size. The bounded comparison documented
here uses two seeds with both policies; it is not the full eight-case matrix.
Inputs and every native result remain preserved separately.

## Source-address correction

The first100-device probe exposed a correctness gap despite successful delivery.
For an unbound UDP socket, ns-3's `RouteOutput` request can have a default-constructed
IPv4 source, whose sentinel is102.102.102.102. The earlier ECMP code replaced only
0.0.0.0, allowing that uninitialized value into some SGW tunnel packets. The old
audit recomputed the hash from the observed source but did not check assignment;
a correct hash calculation alone could therefore accept an invalid source.

`AccessEcmp` now checks `Ipv4Address::IsInitialized()` and `IsAny()`, substituting
the node's stable assigned identity when needed. Explicit source addresses remain
unchanged. The independent auditor rejects any wire source outside the declared
endpoint identities before checking next-hop selection. A regression fixture
explicitly injects the old sentinel and must fail. No radio, task deadline, ARP
setting or physical robot link was changed by this fix.

The initial `access-auth-20261107T113139Z-18999dd8` result and old ECMP records are
preserved, including their historical statuses; review receipts reject them as
source-valid ECMP evidence. They are not counted as corrected validation. The
corrected small control `access-auth-20261107T113553Z-29c5e23a` delivers36/36 with
exact replay and assigned sources. The earlier test fixture is archived separately.

The corrected100-device probe `access-auth-20261107T113638Z-73a76d46` delivers306/306,
all within100ms, with exact replay, all70 active sources per phase and no unassigned
sources. Its audit checks2760 wire hops,640 equal-cost selections,64 repeated
multipath-flow groups and510 restored mappings. Both spines are used before/after;
only spine1 during outage. Native original/replay took103.03/103.79 seconds on this
host. These are measured wall times for this case, not estimates for500 devices.

## Limits

The task schedule is a functional control rather than a calibrated saturated
warehouse load. Radio position and device counts are configuration, but movement
is excluded. ECMP still groups IP flows and outer GTP tunnels, uses hop-count
costs and immediate centralized outage knowledge. Full upper TSN/shim/5G protocols,
cryptographic authorization, ECMP-specific boundary-loss and inner-NR loss
correlation remain outside this result. Cold-start address-resolution delays and
all deadline misses must remain in the offered-task accounting.

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
Per-task measurements are identical between policies for each seed; no latency advantage is measured in this control.

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

The corrected four-zone case reproduces the known task6 deadline miss at979.171427ms. Both small deny/timeout controls block all6 protected tasks and deliver30 normal tasks. The boundary control retains2 authorization timeouts and1 observed payload loss; its entire native trace and device CSV are byte-identical to `access-auth-20261107T062629Z-b36612da`.
