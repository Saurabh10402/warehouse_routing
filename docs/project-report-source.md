Warehouse Routing Simulation

Project Report and Local User Guide

3 November 2026 | Evidence through checkpoint 024

This report documents the completed stationary warehouse networking prototype, its measured behavior and the steps needed to view or reproduce the delivered demonstrations. It is intended for project reviewers, maintainers and users installing the project on a local computer.

<b>Main conclusion.</b> The project provides a working configurable wired routing simulation and a published offline visualization package. The 500-device example has been exercised on Windows with independent packet-trace audits and exact replays. It demonstrates normal communication, equal-cost path selection, alternate-spine routing, and the consequences of a zone-switch failure.

| Delivered capability | Verified outcome |

| Configurable topology | 8 zones, 500 devices and 506 links in the supplied profile |

| Traffic from every robot | 482 sources; 2,410 of 2,410 packets delivered in the baseline |

| Routing comparison | Dijkstra and three ECMP hash seeds; identical offered traffic |

| Failure demonstrations | Spine rerouting and zone-switch isolation with recovery |

| Distribution | Source on GitHub; public ZIP download and fresh extraction verified |

<b>Readiness boundary.</b> The saved website needs only a browser. Running new simulations requires the documented toolchain and wired ns-3 installation. macOS and Ubuntu execution remain untested. The results do not establish radio performance, hardware convergence times or guaranteed deployment deadlines.

<link href="https://github.com/Saurabh10402/warehouse_routing" color="#145f91">Open the GitHub repository and installation README</link><br/><link href="https://github.com/Saurabh10402/warehouse_routing/releases/tag/checkpoint-022" color="#145f91">Open the verified demonstration release</link>

Evidence: Current project state; checkpoints 019–024; published delivery receipt and anonymous-download verification.

How to read this report



| Pages | Subject | Purpose |

| 3–5 | Architecture and model | Understand topology, traffic, routing and failure assumptions |

| 6–7 | Features and development history | Distinguish completed implementation from planned work |

| 8 | Validation and metrics | Understand how results were checked and counted |

| 9–13 | Delivered demonstrations | Read normal-operation, routing and failure results |

| 14–16 | Broader research results | Review native QoS, experimental protection and capacity limits |

| 17–18 | Website walkthrough | Download and inspect saved packet paths |

| 19–22 | Installation and operation | Set up dependencies, reproduce runs and troubleshoot |

| 23–24 | Remaining work and evidence | Plan next steps and locate exact supporting data |

Three evidence layers

<b>Native wired simulation</b> uses ns-3 IPv4/UDP packets, point-to-point devices, real queue events and receive/drop observations. This is the model used by the current 500-device delivery.

<b>Independent core simulation</b> is a separate C++17 discrete-event abstraction of transmission, propagation, finite queues and routing. Its larger scheduling and admission studies are preserved, but its numbers are not native ns-3 measurements.

<b>Legacy examples and future work</b> include the earlier EALR relay research, radio-related scaffolding and unimplemented research contracts. Presence of source or a design document does not imply verified radio/runtime support.

Version and data conventions

The public demonstration ZIP remains the immutable checkpoint 022 snapshot. The newer GitHub README and publication checks are recorded at checkpoint 024. Result-folder timestamps are UTC; dates in this report use Asia/Calcutta. A PASS label means the stated checks completed, not that a deployment requirement has been proven.

Graphs use recorded summary data or documented completed-study tables. Latencies are in milliseconds; packet and flow counts are explicitly distinguished. Each result section cites its underlying batch or tracked evidence file.

Architecture and communication paths



Each zone contains stationary robots, a powered zone switch and a local edge server in the supplied configuration. Every zone switch connects to both shared spines. Robots and local servers attach to a switch within their own zone; robots never relay other robots' traffic.

Figure 1. Two-zone excerpt of the delivered graph. The same attachment rule extends to eight zones.

| Traffic scope | Route in the one-switch-per-zone profile |

| Same-zone peer | Robot → local switch → peer robot |

| Cross-zone peer | Robot → source switch → one shared spine → destination switch → peer |

| Local edge service | Robot → local switch → local edge server |

| Remote backup service | Robot → local switch → spine → remote switch → reachable backup |

The spines protect the shared backbone, not the single robot attachment or the zone switch. A switch outage isolates its attached devices even when both spines are healthy. With multiple switches in a zone, endpoints are attached round-robin; a same-zone path may then cross the backbone.

Powered infrastructure removes the architectural motivation for battery-aware robot relay selection. Dijkstra still computes a minimum-cost route; ECMP selects among equal-cost next hops. Neither requires moving robots or battery-aware forwarding.

Evidence: scripts/warehouse_layout.py; docs/warehouse-demo.md. Earlier core leaf-spine examples attach service nodes differently; their layout is not substituted for this delivery.

Configuration and supplied workload



| Zone | Robots | Switches | Edge servers | Zone total |

| A | 59 | 1 | 1 | 61 |

| B | 61 | 1 | 1 | 63 |

| C | 58 | 1 | 1 | 60 |

| D | 62 | 1 | 1 | 64 |

| E | 60 | 1 | 1 | 62 |

| F | 63 | 1 | 1 | 65 |

| G | 59 | 1 | 1 | 61 |

| H | 60 | 1 | 1 | 62 |

| All zones | 482 | 8 | 8 | 498 |

| Shared backbone | — | 2 spines | — | 2 |

The zone array determines zone count. Counts are explicit JSON inputs, not a hard-coded eight-zone requirement. The supplied mildly uneven robot counts are fixed for reproducibility rather than rerandomized on every run. Total: 500 devices and 506 links.

| Parameter | Supplied value |

| Robot attachment | 100 Mbps; 0.1 ms propagation |

| Switch-to-spine and edge attachment | 1,000 Mbps; 0.1 ms propagation |

| Queue capacity | 100 waiting packets per configured direction |

| Generation and drain | 100 ms generation + 100 ms drain |

| All-robot traffic | 256-byte UDP payload, priority P1, 20 ms interval |

| Generation timestamps | 0, 20, 40, 60, 80 ms |

| Default routing and queue | ECMP and nonpreemptive strict priority |

The all-robot mixed profile creates one flow per robot: 163 same-zone, 161 cross-zone and 158 edge-service flows. Five packets per flow produce 2,410 offered packets. It provides all-source functional coverage, not a saturation test. The representative profile instead creates 24 flows from eight robot sources and offers 120 packets.

Evidence: configs/warehouse_layout.json; configs/warehouse_layout_all_robots.json; scripts/warehouse_layout.py.

Routing queues and failure semantics



Route selection

Native routes minimize free-flow propagation plus serialization cost for the actual wire size. A payload of B bytes has B + 28 IPv4/UDP bytes and B + 30 PPP wire bytes. Dijkstra installs a minimum-cost next hop. ECMP hashes a fixed transport flow among eligible equal-cost next hops; it is not packet spraying or congestion-aware load balancing.

Queue service

The native subset supports finite per-direction FIFO and shared nonpreemptive strict-priority queues classified using actual IPv4 DSCP. An active lower-priority frame finishes before a queued emergency packet can be served. A full shared buffer can reject P0, and a continuing P0 backlog can starve lower classes. No native protected buffer reservation is implied.

Availability and services

Configured node/link events are known immediately, and centrally installed tables rebuild with zero simulated detection/reconfiguration delay. A receive-time gate drops a frame if its link or either incident node is unavailable when reception occurs. Queues continue serializing: an outage that ends before reception does not necessarily destroy that frame.

New packets choose the first reachable destination in an ordered service-candidate list. Each packet keeps its chosen destination in flight. There is no retransmission, session migration, application-state replication or failback hysteresis. All-candidates-unreachable packets are recorded as no_route; unavailable sources are recorded separately.

Accounting boundaries

Every offered packet remains in delivery and deadline denominators. Queue overflow, policing in the core study, no route and cutoff all count as deadline misses. Cutoff packets are censored observations, not proven permanent network loss. Native deadline checks accept delivery within the configured target using a small numerical tolerance; historical core traffic contracts use their documented strict deadline semantics.

The P0/P1/P2/P3 targets are 10/20/40/100 ms. These are experiment inputs, not guaranteed application response times. Network delay excludes sensor sampling, application computation and actuation.

Evidence: docs/ns3-wired-warehouse.md; docs/ns3-resilience-study.md; scripts/ns3_warehouse.py; docs/traffic-requirements.md.

Implemented features and boundaries



| Feature | Status and interpretation |

| Explicit stationary schema v2 graph | Implemented; arbitrary zone membership and wired links; no random movement in the primary runner. |

| Configurable device and traffic counts | Implemented JSON layout and all-robot/representative modes; validation rejects unsupported bounds. |

| Peer and service communication | Implemented local/cross-zone paths and ordered reachable service candidates. |

| Dijkstra and ECMP | Implemented in native and core models; native transport hashing independently audited. |

| FIFO and strict priority | Implemented native and core queues; lower-class delay/loss tradeoffs retained. |

| Node/link failure and recovery | Implemented native receive-time gating and immediate table rebuilds; deterministic resilience controls. |

| Packet and queue evidence | Native packet, hop, queue, wire-header, availability, IP-drop and position records; summaries independently checked. |

| Recorded-path website | Offline HTML export; flow/packet selection, hop stepping, availability state, device details and source hashes. |

| Repeatability and recovery | Exact replay, source/input/runtime provenance, immutable checkpoint manifests and separate delivery ZIP. |

| Protected QoS | Experimental core-only reservation/policing/weighted service. Not the default native queue. |

| Delayed failure detection | Design contract exists; runtime implementation remains pending. |

| Safety and energy extensions | Some core/legacy features exist; native safety events and energy/radio behavior remain unsupported. |

| Radio and target platforms | No validated NR/Wi-Fi deployment model; Mac/Ubuntu execution pending. |

The deliverable is a working wired simulation product and inspection tool. The broader original research ambition is only partly complete. “Implemented” applies to the named backend and tested scope, not to every legacy example or future extension.

Evidence: checkpoints/PROJECT_STATE.json; current native capability checks; docs/ns3-resilience-study.md; docs/protected-queue-results.md.

Development milestones and scope changes



| Checkpoint | What changed | Evidence or consequence |

| 001–004 | Recovered and established stationary core | Preserved archives, configuration, tests and legacy APIs. |

| 005 | Powered peer/service study | ECMP and FIFO/priority comparison; 132-run primary matrix; EALR retained as legacy. |

| 006–007 | Experimental emergency protection and capacity | 144 protected runs; 288 capacity/admission runs; adverse flood and lower-class outcomes recorded. |

| 008–010 | Real wired ns-3 integration | Pinned dependency, analytical UDP checks, schema-v2 forwarding, native DSCP priority and ECMP. |

| 011–012 | Process supervision and resilience | Bounded native cases; 100 native cases and every replay; fresh-source 700-file match. |

| 013 | Delayed-detection design | Contract and controls declared; no runtime measurement or completed implementation. |

| 014–018 | Deadline-focused demonstration | Configurable 500-device layout, offline viewer, template fix, spine outage and verified consolidated package. |

| 019–020 | Traffic from all robots and runtime fixes | 482 flows; unused endpoint route entries omitted; audit distance cache; clean-result-directory and Windows path fixes. |

| 021 | All-robot routing comparison | Four audited cases, eight native executions; same arrivals and per-spine usage report. |

| 022 | Zone-switch isolation | Affected/unaffected phase assertions; losses counted; recovery and viewer checked. |

| 023–024 | GitHub distribution and guide | Source history pushed; delivery asset uploaded; anonymous download and fresh extraction verified. |

Historical documents describe what was pending at their own checkpoint. Later native failure support supersedes earlier “dynamic failures unsupported” statements; delayed detection, radio and native protected queues are still pending. The current state and this report separate those timelines.

Evidence: checkpoints/CHANGELOG.md; sessions 016–020; checkpoint 024 current state. No older manifest or raw batch was relabeled as a newer experiment.

Validation and measurement method



A completed run is accepted only after configuration/build checks, native execution, independent raw-data audit and deterministic replay. The Python audit reconstructs expected routing/timing and checks observable packet, queue, availability and header behavior rather than trusting the simulator summary alone.

| Layer | Evidence |

| Native project tests | 25 tests in the current Windows build; analytical routing/queue/packet and process-control coverage. |

| Core tests | 21 tests in the current independent-core build; separate from native measurements. |

| Native acceptance matrix | 100 cases + 100 replays: 12 controls, 36 static policy cases and 52 resilience controls. |

| Core study | Full 132-run / three-seed default matrix, with exact baseline regression evidence. |

| Fresh-source reproduction | Independent native build reproduced all 700 raw artifacts in the 100-case suite. |

| Current demo checks | Generator tests, comparison arrival guards, outage phase checks, independent exporter audit and browser inspections. |

| Distribution checks | Public ZIP download, SHA256 match, fresh extraction, 4,424 file hashes and six linked HTML/menu pages. |

How to interpret the numbers

Delivery fraction is delivered divided by all offered packets. Deadline misses include non-delivery. Mean, P95 and maximum delay describe delivered packets only; P95 uses nearest rank in the all-robot comparison. Per-spine counts are observed packet paths, not measured bandwidth utilization. Queue waits measured at service start exclude packets that never started service.

The three ECMP seeds in the native comparison vary hash assignment, not random traffic arrivals. The core studies have their own declared phase/seed rules. Exact replay demonstrates deterministic reproducibility, not an independent statistical replication or confidence interval.

The 100-case and 132-run regressions were last rerun for the all-robot/runtime milestone. Later orchestration, documentation and packaging changes did not rerun those full studies. This report introduces no new simulation experiment.

Evidence: checkpoints/evidence/checkpoint_019; checkpoint_021; checkpoint_022; checkpoint_024/github-download-verification.json.

Baseline and all robot communication results



Both supplied normal-operation workloads delivered all offered packets. The all-robot workload activates all 482 robot sources, but each sends only five short packets over 100 simulated milliseconds. Equal generation times create synchronized offered traffic; the configuration remains a light functional example.

Figure 2. All-robot baseline deliveries by communication scope. Each scope delivered every offered packet.

| Measure | Representative | All robots |

| Devices / links | 500 / 506 | 500 / 506 |

| Active robot sources | 8 | 482 |

| Flows | 24 | 482 |

| Offered / delivered | 120 / 120 | 2,410 / 2,410 |

| Deadline misses | 0 | 0 |

| Mean delivered latency | See raw representative batch | 0.317991 ms |

| Maximum delivered latency | See raw representative batch | 0.480080 ms |

An alternate two-zone configuration with 11 total devices, multiple switches and a zone without a local edge also passed: 30/30 packets delivered with independent audit and replay. This exercises configuration flexibility rather than proving arbitrary layouts correct by itself.

The native route-initialization optimization omits route entries on endpoints that cannot forward a given flow. The audit caches pure reference-distance calculations per immutable availability snapshot. Earlier 500-device baseline/outage artifacts remained byte-identical (14 files), and the full native regression retained all 700 artifacts. Runtime observations varied with host load and are not controlled speed benchmarks.

Evidence: layout-20261101T203251Z-838ca5b1; layout-20261101T203641Z-425a1ee4; layout-20261102T121400Z-1b3f3906; checkpoint_019 comparison evidence.

Dijkstra and ECMP spine distribution



The comparison changes only forwarding algorithm and ECMP hash seed. Topology, payloads, priorities, queue discipline and all arrival signatures are identical. Dijkstra has one deterministic control; ECMP runs seeds 1, 2 and 3. Every case has an independently audited exact replay.

Figure 3. ECMP distributes flows across both equal-cost spines. Dijkstra tie-breaking selects spine 0 for all 161 cross-zone flows in this topology.

| Case | Spine 0 packets | Spine 1 packets | Spine 0 / 1 flows |

| dijkstra-s1 | 805 | 0 | 161 / 0 |

| ecmp-s1 | 435 | 370 | 87 / 74 |

| ecmp-s2 | 390 | 415 | 78 / 83 |

| ecmp-s3 | 375 | 430 | 75 / 86 |

All 805 cross-zone packets traverse one spine; same-zone and local service traffic do not. A stable flow uses the same path while the topology remains unchanged. ECMP therefore splits flows, not individual packets. The different seed totals demonstrate that equal-cost availability does not promise an exact 50/50 split.

Both algorithms delivered 2,410/2,410 packets with zero deadline misses. Better distribution in this case is evidence of using redundancy, not evidence that Dijkstra is unnecessary or that ECMP guarantees congestion avoidance.

Evidence: routing-20261102T152642Z-82d74709/comparison.json; tracked copy in checkpoint_021; declaration.json and matched-arrival guard.

Routing latency comparison



Figure 4. Zero-based latency axis. The mean improvement is small under this workload; tail latency also changes.

| Case | Mean ms | P95 ms | Maximum ms | Cross-zone mean ms |

| dijkstra-s1 | 0.321698 | 0.489232 | 0.496096 | 0.472235 |

| ecmp-s1 | 0.317991 | 0.468640 | 0.480080 | 0.461136 |

| ecmp-s2 | 0.317948 | 0.468640 | 0.477792 | 0.461009 |

| ecmp-s3 | 0.318024 | 0.468640 | 0.484656 | 0.461236 |

Dijkstra mean delay was 0.321698 ms; ECMP means ranged from 0.317948 to 0.318024 ms. The difference is roughly 0.0037 ms, or 3.7 microseconds. Cross-zone means fell from 0.472235 ms to approximately 0.4610–0.4612 ms. These are finite-run observations, not expected improvements for every traffic matrix.

Scope-specific results are retained in the raw comparison JSON. The ECMP seed 1 case reproduced all seven artifacts from the prior all-robot baseline. All four cases preserve identical offered arrivals and deadlines; the comparison does not alter admission policy to favor a routing method.

A capacity claim would require declared offered-load sweeps, queue/loss outcomes, controlled bottlenecks and appropriate variability analysis. This four-case demonstration establishes routing behavior and repeatability at one light configuration.

Evidence: routing-20261102T152642Z-82d74709; docs/routing-comparison.md. P95 uses the nearest-rank delivered-packet definition.

Spine failure and zone isolation



The spine demonstration takes shared spine 1 down at 30 ms and restores it at 70 ms. With representative traffic, all 120 packets arrive: 48 before failure, 48 during outage and 24 after recovery. Six cross-zone flows switch from spine 1 to spine 0 and back; all 40 same-zone peer packets remain local.

The zone-switch demonstration uses all-robot traffic and fails Zone A's sole switch over the same interval. The 59 Zone A source flows and 20 incoming Zone H cross-zone flows depend on that switch. Other traffic can continue despite healthy spines being unable to repair the isolated attachment.

Figure 5. Phase counts differ because before/outage contain two generation times each; recovery contains one. Use the offered denominators below when comparing phases.

| Phase | Affected offered / delivered | Other offered / delivered |

| before | 158 / 158 | 806 / 806 |

| outage | 158 / 0 | 806 / 806 |

| recovered | 79 / 79 | 403 / 403 |

Total: 2,410 offered, 2,252 delivered and 158 no_route losses, all counted as deadline misses. Restoration permits newly generated packets; lost packets are not retransmitted. Samples are spaced away from event boundaries, so this presentation does not measure hardware detection or convergence time.

Evidence: layout-20261102T065135Z-da39e8c6; layout-20261102T171806Z-fe0a210a; run/replay outage phase reports and seven-file replay checks.

Recorded failure view



Figure 6. Actual exported viewer at packet #2, generated at 40 ms. Zone A switch is marked DOWN; status is no_route and the observed path contains only the source.

The aggregate 2,252/2,410 count is expected for this failure experiment. Selecting packet #4 shows successful delivery at 80 ms after recovery. Selecting Zone B's same-zone flow and packet #297 shows successful delivery at 40 ms while Zone A is still down.

Evidence: Browser-checked exported HTML from layout-20261102T171806Z-fe0a210a. Screenshot is a recorded view; the PDF is not an embedded live simulator.

Native scheduling tradeoffs under contention



The earlier native policy study deliberately exercises bottlenecks, unlike the light 500-device demonstration. Each case offers 156 packets, including 60 P0 and 24 P3 packets. The table gives minimum–maximum deadline-miss counts over ECMP hash seeds 1/2/3, aggregated across communication scopes.

| Scenario | Policy | P0 misses / 60 | P3 misses / 24 |

| Uplink contention | Dijkstra / FIFO | 38 | 14 |

| Uplink contention | Dijkstra / priority | 29 | 16 |

| Uplink contention | ECMP / FIFO | 18–38 | 12–14 |

| Uplink contention | ECMP / priority | 5–26 | 15–16 |

| Destination bottleneck | Dijkstra / FIFO | 19 | 7 |

| Destination bottleneck | Dijkstra / priority | 15 | 7 |

| Destination bottleneck | ECMP / FIFO | 0–19 | 6–7 |

| Destination bottleneck | ECMP / priority | 0–14 | 7–8 |

Priority improves emergency-class results in these cases, but may increase lower-class misses. ECMP can reduce concentration on a shared path, yet the hash-dependent range remains wide. A single seed with zero P0 misses at a destination bottleneck is not a safety or capacity guarantee.

Mechanisms explicitly tested

The priority ordering control confirms that an active low-priority frame is not preempted, queued P0 messages preserve their admission order, and P1 precedes remaining P3. A one-slot buffer can reject an arriving P0 while retaining an admitted P3. A flood can leave P3 waiting at cutoff. These are intentional exposed limitations of shared strict-priority buffers.

The study ran 12 native controls plus 36 declared policy cases and every replay. Independent fresh-source builds matched 48 inputs and 240 original measurement artifacts. Later resilience validation expanded the suite to 100 cases without erasing those earlier observations.

Evidence: docs/ns3-policy-results.md; native batches ns3-warehouse-20261031T215600Z-dd1a4b9a and ns3-warehouse-20261031T215445Z-85e4fb40. Counts are not confidence intervals.

Experimental protected queues in the core model



This research is separate from the native delivered queue. The optional core policy reserves P0 waiting space, applies a per-direction P0 byte token-bucket admission policer, and uses self-clocked byte-weighted service for admitted lower classes. FIFO and ordinary strict priority remain the defaults for the primary comparison.

Figure 7. Three-seed means from the completed 144-run abstract-link study. The fixed protection policy improves some cases but markedly worsens emergency overload.

| Scenario | P3 misses priority % | P3 misses protected % | Mean P0 policed |

| Uplink contention | 0.333 | 0.778 | 0 |

| Destination bottleneck | 25.111 | 25.167 | 6 |

| Emergency overload | 5.889 | 3.778 | 1,227 |

| Synchronized burst | 25.056 | 25.222 | 35.667 |

The fixed 20% admission ceiling can reject P0 even when total link capacity is otherwise unused. At the 1 Mbps attachment, ordinary P0 outbound traffic of 0.2304 Mbps exceeds the 0.2 Mbps budget. During overload, 1,227 of 4,800 offered P0 packets per seed are policed. These remain deadline misses, not hidden rejected demand.

Reservation improves waiting-space access but creates neither capacity nor preemption. The destination case also averages 225 lower-class reserved-capacity rejections per run. Weighted service for admitted packets does not mean every offered flow is starvation-free.

Evidence: docs/protected-queue-results.md; experiments-20261030T173430Z-db8fbaf2; seeds 1/2/3. Values are core abstractions, not native ns-3 or NR measurements.

Capacity and admission limits



The predeclared capacity/admission sweep executed 288 core runs using held-out seeds 4/5/6, eight settings and four attachment capacities. Independent recomputation reproduced all 96 operating-limit rows. No tested setting eliminated P0 misses for burst or flood traffic.

| Case and policy | P0 misses % | P3 misses % | P3 delivery % |

| Mixed 1 Mbps / strict | 15.278 | 25.056 | 79.889 |

| Mixed 1 Mbps / reserved unpoliced | 1.944 | 25.111 | 79.667 |

| Mixed 1 Mbps / rate20 | 4.444 | 25.111 | 79.667 |

| Mixed 2 Mbps / reserved unpoliced | 0.000 | 23.556 | 83.000 |

| Burst 5 Mbps / reserved unpoliced | 6.250 | 7.778 | 92.222 |

| Flood 5 Mbps / strict | 12.139 | 18.278 | 85.278 |

| Flood 5 Mbps / reserved unpoliced | 1.285 | 24.889 | 79.500 |

| Flood 5 Mbps / rate20 | 46.688 | 10.833 | 89.167 |

Disabling policing can reduce P0 losses but worsen lower-class delivery and allow starvation. The mixed workload offers 9.4016 Mbps to the bottleneck across all classes, more than even the 5 Mbps case can carry indefinitely. Source-time envelopes do not prove downstream per-port arrival bounds after queuing.

| Attachment Mbps | Active 1200-byte frame ms | 32 × 96-byte burst ms |

| .5 | 19.2 | 49.152 |

| 1 | 9.6 | 24.576 |

| 2 | 4.8 | 12.288 |

| 5 | 1.92 | 4.9152 |

These serialization-only calculations exclude propagation and other queued traffic. At 2 Mbps the burst alone exceeds a 10 ms target. At 5 Mbps it can fit in isolation, yet finite admission and competing traffic still cause misses. Keep protected QoS experimental; do not select a default solely from one zero-miss mixed-load result.

Evidence: docs/capacity-results.md; experiments-20261030T175442Z-cb0b1ccc; operating_limits.csv and endpoint_envelopes.csv. Percentages are three-seed means.

Download and open the visualization website



<b>Repository:</b> <link href="https://github.com/Saurabh10402/warehouse_routing" color="#145f91">github.com/Saurabh10402/warehouse_routing</link><br/><b>Release:</b> <link href="https://github.com/Saurabh10402/warehouse_routing/releases/tag/checkpoint-022" color="#145f91">checkpoint 022 verified demonstration</link>

1. Open the release link. Under Assets download <b>warehouse-demo-checkpoint-022.zip</b>. Do not select the automatic Source code ZIP for saved demos: it omits the generated result folders.

2. Extract the entire archive. On Windows, right-click and choose Extract All; use a short folder such as C:\Warehouse to avoid long-path problems. On macOS/Linux use a short folder in your home directory.

3. Find the folder containing OPEN_WAREHOUSE.html, START_HERE.md and results. Keep these together. Double-click OPEN_WAREHOUSE.html or use your browser's File → Open command.

4. Select baseline, all robots, routing comparison, spine failure or zone-switch failure. The saved website works offline without Python, ns-3 or a local server. Browser behavior can vary; the optional server below provides a local fallback.

Optional local browser server

With Python installed, open a terminal in the extracted directory and run:

python -m http.server 8000 --bind 127.0.0.1

Open http://127.0.0.1:8000/OPEN_WAREHOUSE.html. Leave the terminal running; press Ctrl+C to stop. Use python3 where your system names Python that way. The loopback binding serves only the local computer.

Check package integrity

python scripts/verify_delivery.py

The delivery package includes DELIVERY_MANIFEST.json. The check verifies hashes and local links; a plain source clone has no such package manifest. The published ZIP was downloaded anonymously, checksum-verified and extracted afresh: all 4,424 file hashes and six local menu/viewer pages passed.

SHA256: 052f2eebbc78a2aaf2be1fc4da7946dba4726eeeaeaeddaafe37a5eb8e57ed9f

Evidence: README.md section 1; recovery_packages/github-download-verification.json; checkpoint_024 evidence. This download test is not a fresh operating-system/toolchain installation test.

Operate the website step by step



| Control | How to use it |

| Traffic flow | Choose a same-zone, cross-zone or edge-service flow; the all-robot profile has 482 choices. |

| Recorded packet | Choose a packet timestamp/status. This selects an observation, not a new simulated transmission. |

| Next hop and Previous hop | Step through the observed path. Show full path restores the full recorded route. |

| Show all robots | Hide inactive endpoints for clarity; hidden robots remain in the simulation. |

| Device click | Read node identity, role, zone and configured coordinates. |

| Availability events | Read failures/recoveries and availability at the selected packet's generation time. |

| Receive times and integrity | Inspect native receive timestamps and recorded source hashes. |

Three minute demonstration sequence

<b>Normal operation.</b> Open the all-robot viewer. Pick a local flow and confirm that its route stays within the zone. Pick a cross-zone flow and identify the shared spine. Pick an edge-service flow and identify the local server.

<b>Routing alternatives.</b> Open the comparison page. All cases deliver 2,410 packets; Dijkstra places 805 cross-zone packets on one spine, while ECMP divides them across both. Read latency values with the workload caveat.

<b>Backbone resilience.</b> In the spine-failure viewer select A-cross-zone and packets #6 at 20 ms, #7 at 40 ms and #9 at 80 ms. The path changes to the available spine and returns after recovery.

<b>Single-zone vulnerability.</b> In the zone-switch viewer use the first flow and packets #1, #2 and #4. The middle packet has no route. Then select zone-B-robot-0-same-zone and #297 to show unaffected communication during the same outage.

What the screen cannot establish

The display is a schematic saved trace, not a warehouse floor plan, live traffic dashboard, wireless coverage map or real-time animation. Changing selection cannot test a newly proposed topology or failure; run a new simulation and export its result for that.

Evidence: scripts/warehouse_viewer.html; README.md; browser checks at checkpoints 021 and 022. The source template in scripts is intentionally not populated.

Install and run on Windows



The project's executed validation used Windows with verified GCC 14.2.0, Boost 1.85 and wired ns-3.46.1. The steps below use ordinary Python rather than a Codex-specific path. A new user still needs internet access, disk space and time for dependency compilation. Windows ARM is not validated; the portable compiler package is x86-64.

1. Install Git and Python 3.12 from their official sites. Reopen PowerShell, then obtain the source:

git clone https://github.com/Saurabh10402/warehouse_routing.git<br/>cd warehouse_routing<br/>py -3.12 --version<br/>py -3.12 -m venv .venv

2. Install the local portable toolchain and verify the independent core:

.\.venv\Scripts\python.exe scripts/setup_portable.py --plots<br/>.\.venv\Scripts\python.exe scripts/first_run.py

The setup verifies pinned archive checksums and installs under .tools without editing global PATH or the registry. The core fixture should report 180/180 delivered and zero deadline misses. That is an abstract-link check, not native ns-3 validation.

3. Install wired ns-3 and run the analytical smoke check:

.\.venv\Scripts\python.exe scripts/setup_ns3.py --jobs 4<br/>.\.venv\Scripts\python.exe scripts/run_ns3_smoke.py

Use --jobs 2 on a low-memory PC. Python 3.12+ is needed for safe archive extraction. The native smoke fixture expects three delivered packets and 4.416 ms mean delay with exact replay. Upstream tests can be quiet for minutes; check the retained setup log before treating silence as a hang.

4. Run the warehouse with the same interpreter:

.\.venv\Scripts\python.exe scripts/run_warehouse_layout.py

Use .\.venv\Scripts\python.exe in place of python in the later examples. Environment activation or an execution-policy change is unnecessary. Open a terminal in the folder containing CMakeLists.txt and scripts; do not run the commands from inside scripts.

Evidence: README.md sections 2–4; scripts/setup_portable.py; scripts/setup_ns3.py; docs/first-run.md. Fresh public-download verification did not reinstall this toolchain.

Prepared Ubuntu and macOS installation



<b>Validation boundary.</b> These recipes are documented but have not been executed on Ubuntu or macOS for this delivery. They must be tested on the intended host before claiming platform support. Do not reuse Windows build caches or binaries on another operating system.

Ubuntu 24 04

sudo apt update<br/>sudo apt install git build-essential cmake ninja-build \<br/>  libboost-json-dev python3 python3-venv<br/>python3 -m venv .venv<br/>. .venv/bin/activate<br/>python -m pip install -r scripts/requirements.txt<br/>python scripts/first_run.py

Run from a fresh repository clone or extracted source folder. Check python --version is at least 3.12 before native dependency setup. On a Mac M4 virtual machine, use an ARM64 Ubuntu installation and matching compiler libraries.

macOS on Apple Silicon

Install Xcode Command Line Tools and Homebrew from its official site if absent. Complete the installer before continuing:

xcode-select --install<br/>brew install cmake ninja boost python@3.12<br/>python3.12 -m venv .venv<br/>. .venv/bin/activate<br/>python -m pip install -r scripts/requirements.txt<br/>python scripts/first_run.py

Use a native ARM64 terminal and avoid mixing Intel Homebrew with ARM64 libraries. An already-installed response to xcode-select is harmless.

Then install and verify native wired ns3

python scripts/setup_ns3.py --jobs 4<br/>python scripts/run_ns3_smoke.py<br/>python scripts/run_warehouse_layout.py

If a setup fails, retain logs and record architecture, Python, compiler and CMake versions. Reproduction requires compatible dependencies, not just the Python runner. The installer deliberately rejects unsupported or mismatched conditions instead of labeling fallback output as native success.

Evidence: README.md; docs/first-run.md; docs/reproducibility.md. Offline saved HTML viewing is a separate browser-only workflow.

Create new runs and export new viewers



Run the all-robot example after native setup:

python scripts/run_warehouse_layout.py \<br/>  --layout configs/warehouse_layout_all_robots.json

The runner prints a unique results/layout-... directory. It builds/tests, saves configuration/provenance, executes native simulation and replay, and audits both. Wait for PASS before interpreting a completed batch.

Export that specific directory, replacing YOUR_BATCH with the exact printed name:

python scripts/visualize_warehouse.py \<br/>  --batch results/YOUR_BATCH --output warehouse-viewer.html

Open warehouse-viewer.html. Choose a fresh output filename for later exports; existing outputs are not overwritten. The root menu points to the packaged examples and does not automatically discover your new batches. In PowerShell, put each command on one line; the displayed backslash continuation is for POSIX shells.

Other scenario commands

python scripts/run_warehouse_layout.py --spine-outage<br/>python scripts/run_warehouse_layout.py \<br/>  --layout configs/warehouse_layout_all_robots.json --zone-outage A<br/>python scripts/compare_warehouse_routing.py

The routing comparison writes comparison.html directly under its results/routing-... directory. The two outage flags are mutually exclusive. Zone isolation requires exactly one switch in the chosen zone and informative affected/unaffected samples.

Edit topology and traffic

Copy the layout JSON and change the zone array, explicit robot/switch/server counts or sharedSpines. The traffic object selects representative or allRobots mode, mixed/same-zone/cross-zone/edge-service scope, intervalMs, bytes, priority and serviceCandidates. Use --layout path/to/your-layout.json. JSON requires double quotes and forbids comments/trailing commas.

Payload limits are 1–1,472 bytes; priority 0–3; interval 0.001–100 ms; service candidates 1–16. The generator bounds devices at 4,096, flows at 4,096 and offered packets at 1,000,000. These are software limits, not demonstrated capacity or runtime promises.

Evidence: scripts/warehouse_layout.py; scripts/run_warehouse_layout.py; scripts/visualize_warehouse.py; docs/warehouse-demo.md.

Diagnostics reproducibility and recovery



| Symptom | Action |

| Missing or blank viewer | Use the complete delivery asset and root menu. Source clones omit saved results; scripts/warehouse_viewer.html is a template. |

| Python or compiler missing | Complete the OS setup and reopen the terminal. On Windows use the project environment and setup_portable.py. |

| ns-3 prefix absent | Complete setup_ns3.py. A custom --ns3-prefix must point to a compatible installed wired runtime. |

| Windows path too long | Extract to a short directory. Do not flatten the archive or remove evidence files. |

| Build memory failure | Retry dependency setup with --jobs 2; retain setup/build logs. |

| Checksum or audit mismatch | Stop interpreting results. Preserve the exact input, raw outputs, source manifest and failing log; do not disable checks. |

| Native timeout or stalled process | Read RUN_STATUS.json and processes/*.json/*.log. A partial trace is not PASS. The root cause of an earlier Windows exit stall remains unresolved. |

| Expected outage losses | Read no_route/deadline counts and phase reports; recovery enables new traffic rather than retransmitting lost packets. |

Useful validation commands

python scripts/verify.py<br/>python scripts/run_ns3_warehouse.py<br/>python scripts/run_all_experiments.py

The full core run has 132 cases across three seeds; --quick is not a substitute. Optional protected-qos and capacity-admission suites retain their own contracts and adverse results. Use a new build directory on another host rather than reusing old CMake caches.

Each batch stores inputs, source/executable/library hashes, runtime metadata and process logs. Checkpoint manifests describe source, while separate release archives preserve generated evidence. Older failed/interrupted batches remain distinct from accepted runs. The native process timeout mitigates waiting but does not diagnose the historical Windows exit-delay cause.

Evidence: docs/reproducibility.md; docs/ns3-run-control.md; README.md; scripts/checkpoint.py and package_delivery.py.

Remaining work and recommended priorities



| Priority | Work | Completion evidence required |

| 1 | Target-host installation validation | Fresh Windows user setup plus actual Mac M4/Ubuntu ARM64 build, smoke test, native demo and export; retain logs and dependency hashes. |

| 1 | Presentation and handoff review | Rehearse menu examples and installation guide; verify reviewer can distinguish expected failure losses from broken execution. |

| 2 | Real warehouse calibration | Obtain device roles, link capacities, traffic/burst traces, acceptable losses, application deadline boundaries and availability targets. |

| 2 | Nonzero failure detection | Implement the existing physical/detected/installed-state contract; exercise delay ordering and event boundaries before broader comparison. |

| 2 | Process reliability diagnosis | Investigate intermittent Windows exit behavior; retain bounded execution, receipts and explicit failed statuses. |

| 3 | Native protected QoS research | Specify and test compatible queue/admission semantics; compare all offered classes without hiding policed demand. |

| 3 | Capacity and robustness expansion | Declare load/port/reservation sweeps and additional seeds before execution; retain adverse outcomes and per-scope metrics. |

| Future | Radio or private 5G integration | Pin compatible ns-3/5G-LENA versions and validate real user-plane paths including UPF; do not infer radio from wired labels. |

Architectural decisions still open

Two spines protect only part of the network. Protecting a zone switch would require additional attachment design, endpoint redundancy and tested failover semantics. The current example does not automatically provide those properties by increasing the number of switches in JSON. Service-state replication and retransmission would also require separate application behavior.

The current product is suitable for demonstrating and reviewing the declared wired routing model. Production acceptance needs calibrated requirements and target-system evidence. The next work should preserve the completed known-good package rather than silently retuning it or replacing historical results.

Evidence: checkpoints/NEXT_STEPS.md; docs/ns3-detection-contract.md; docs/traffic-requirements.md; docs/capacity-results.md.

Evidence index and handoff checklist



| Evidence | Location or identifier |

| Current source and guide | GitHub main at report preparation: 88eb58212b0c35504c644093177abfd79d508412; checkpoint024 |

| Immutable demo source | Tag checkpoint-022; commit1382ba85ab9607ca4f10babecaec91433a11abdc |

| Public delivery ZIP | Release checkpoint-022 / warehouse-demo-checkpoint-022.zip; 16,117,258 bytes |

| Representative baseline | layout-20261101T203251Z-838ca5b1 |

| Alternate topology | layout-20261101T203641Z-425a1ee4 |

| Spine outage | layout-20261102T065135Z-da39e8c6 |

| All robots | layout-20261102T121400Z-1b3f3906 |

| Routing comparison | routing-20261102T152642Z-82d74709 |

| Zone outage | layout-20261102T171806Z-fe0a210a |

| Latest full native regression | ns3-warehouse-20261102T121634Z-4c776a9c |

| Latest full core regression | experiments-20261102T120006Z-a802bc3c |

| Protected study | experiments-20261030T173430Z-db8fbaf2 |

| Capacity study | experiments-20261030T175442Z-cb0b1ccc |

Result identifiers refer to folders under results in the project workspace. The public delivery ZIP contains selected native/demo/core-regression batches; not every historical protected/capacity raw batch is included. Tracked study summaries and checkpoint evidence preserve their results; retain the original workspace/recovery packages for full historical raw data.

Before presenting or reproducing

Confirm the correct release asset and checksum; extract all files; open the root menu; demonstrate normal routing, ECMP, spine rerouting and zone isolation; explain expected losses; identify the native/core boundary; state platform and convergence limits; keep logs and provenance for any new run.

The source repository and release are clickable on page 1. The detailed installation README is the maintained command reference. This report records the completed state and its remaining work as of 3 November 2026; it does not introduce a new simulation experiment.
