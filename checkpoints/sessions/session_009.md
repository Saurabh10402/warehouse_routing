# Native resilience continuation

Implemented in 1c4944d and 93516df: native link/node receive-time availability gates, immediate centralized route rebuilding, ordered backup service selection per newly generated packet, and explicit TTL loop guard. Stationary nodes and legacy APIs retained. Queues continue serializing during outages; no migration, retransmission or protocol convergence claim.

Final native batch ns3-warehouse-20261101T155343Z-9c1d253c passed all 100 cases, independent audits and every replay (200 processes). Core experiments-20261101T154333Z-7a7bd456 passed all 132 cases, baseline comparison, replay and plots. Original 48 static cases preserve 240 measurement files byte-for-byte against checkpoint 011. Prepared clean source matches current source manifest.

Checkpoint 011 verification differences were investigated: exactly the 12 committed implementation/test/documentation files; no uncommitted user edits. Previous manifests/results/archives retained. Intermediate 96-case batches retained.

Next: finish clean-source 100-case validation in .recovery/ns3 resilience final 68c1cd41, compare its 700 measurement files, then finalize checkpoint 012 and separate recovery packages. Do not claim fresh validation or packaging complete until checked. Mac/Ubuntu ARM64, NR, calibrated traffic and nonzero detection/reconfiguration timing remain unvalidated.


## checkpoint_012 completed validation
# Native wired resilience results

Implementation: 1c4944d and 93516df. Final batch: `ns3-warehouse-20261101T155343Z-9c1d253c`. All 100 cases passed independent artifact audits and deterministic replay (200 completed processes). Native verification passed 25/25 tests. The original 48 static cases retain all 240 original measurement files byte-for-byte against checkpoint 011.

The suite adds 13 controlled resilience fixtures across Dijkstra/ECMP and FIFO/strict-priority: alternate links and nodes, preferred-service failback, all services unavailable, source outage, initial unavailability, queued recovery, receive-time boundaries, in-flight route loss, same-zone survival, leaf outage, simultaneous events, and repeated route changes exhausting the hop limit. The static matrix uses three ECMP hash seeds; resilience controls are deterministic, not stochastic reliability trials.

The 132-run core regression `experiments-20261101T154333Z-7a7bd456` passed baseline comparison, replay and plots; core verification passed 21/21 tests. Core abstraction results are separate from native wired measurements.

Availability is checked when a frame reaches the receiving device. Queues continue serializing during outages. Route tables rebuild with zero simulated detection/reconfiguration delay. Newly generated packets choose the first reachable configured service; in-flight packets retain their target. No retransmission or application-state replication is modeled. The loop guard records native route-error reason 5 at TTL 1, avoiding unsupported ICMP traffic.

Clean-source batch `ns3-warehouse-20261101T201540Z-3d3ee628` passed 25/25 tests and all 100 cases/replays. All 700 measurement files and both comparison tables match the main batch byte-for-byte. Nonzero detection timing, native protected queues/safety, NR and actual Mac M4/Ubuntu ARM64 execution remain pending. These controlled results do not guarantee deployment deadlines or hardware failure behavior.

Next exact action: declare separate physical and detected availability with nonzero detection/reconfiguration timing, then add controlled native tests before broader evaluation.
