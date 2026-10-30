# Protected queue policy and evaluation

The `protectedPriority` scheduler is an optional core-link abstraction. It combines three
ordinary mechanisms, not a claimed novel routing algorithm or certified safety mechanism:
reserved P0 waiting slots, an aggregate per-direction P0 token-bucket admission policer,
and self-clocked byte-weighted fair queueing for P1/P2/P3. FIFO and strictPriority are unchanged.

```json
"simulation": {
  "durationMs": 200,
  "drainMs": 200,
  "seed": 1,
  "scheduler": "protectedPriority",
  "protectedQueues": {
    "reservedEmergencyPackets": 8,
    "emergencyRateFraction": 0.2,
    "emergencyBurstBytes": 512,
    "lowerClassWeights": [4, 2, 1]
  }
}
```

These are also the defaults when protectedPriority omits the optional object. Supplying the
object with another scheduler is rejected. Reservation must be less than each link's finite
waiting capacity. Zero reserved slots is allowed as an ablation. Weights are integers 1..1024;
rate fraction is strictly between zero and one; burst allowance is positive and must fit
each configured P0 packet (and 64-byte safety fan-out when configured). The legacy EALR
snapshot queue-cost estimator does not support this scheduler, and that combination is rejected.

## Admission and service semantics

All queues are per link direction, full duplex, and count waiting packets, excluding the
transmitting packet. A port's total buffer remains Q slots, not Q plus the reservation.
P1..P3 together may occupy at most Q-R slots. P0 may occupy any remaining slot, including
the shared part. There is no eviction of admitted packets and no borrowing of unused P0
reservation by background traffic. A full buffer can still drop P0.

P0 tokens initially equal the burst allowance B bytes and refill at
`link.capacityMbps * 1000 / 8 * emergencyRateFraction` bytes/ms, capped at B. All P0 flows
sharing that egress direction share the bucket. Reverse directions are independent. A P0
packet must have enough tokens when admitted; otherwise it is dropped as `priority_policed`.
Tokens are consumed only on successful enqueue, not on rejection. They are not refunded for
later failures/reroutes. Link downtime permits refill up to B; availability does not reset
the bucket. This is per-hop packet policing, not flow admission/reservation negotiation.
The rate ceiling applies even without competing lower-class traffic. It may reject P0
packets while total link capacity remains unused; the default 20% is a study parameter,
not a recommended production allocation or congestion-conditional low-latency queue.

Admission precedence: total-full `queue_overflow`; non-P0 reserved-space restriction
`reserved_capacity`; P0 token rejection `priority_policed`; otherwise enqueue. Admission
is evaluated before same-time service dispatch, preserving existing event ordering.

Service is nonpreemptive. An admitted P0 packet is selected before lower classes, but cannot
interrupt a frame already in transmission. The finite burst and rate fraction below one
bound P0 admission, leaving long-term service opportunity for admitted lower-class traffic
on an available link. They do not guarantee a finite end-to-end deadline or admission of
every packet. Buffers can still fill and P1..P3 share their admission space.

For P1..P3, enqueue finish tag = max(previous tag for the class, port fair clock) + payload
bytes / configured class weight. The smallest head tag is served; ties favor lower priority
number. The port clock advances to that tag when transmission starts. Each class retains FIFO
order. Empty classes do not reserve transmission time. Failure-flushed queues reset their
last tags to the existing clock; rerouted packets receive tags at their new output queue.
This is a self-clocked fair-queue approximation, not exact GPS or a hardware QoS configuration.

## Measurements and experiment design

`queue_stats.csv` reports each direction/class: admission attempts, admitted packets, service
starts, bytes started, each rejection cause, rerouted packets, waiting at cutoff, peak waiting
packets and mean/maximum queue wait **among service starts**. A frame started before cutoff
may not finish, so bytes_started is not delivered throughput or measured utilization. Waiting
packets at cutoff and rerouted waiting packets are excluded from the wait-time sample; do not
interpret that sample as the unconditional delay of every offered packet.
The rerouted counter counts removal for a rerouting attempt, including attempts that fail
to find another path. It does not assert successful delivery via the alternate route.

End-to-end packets and deadline denominators include all rejection causes. The scope/class
comparison table now includes their counts. The protected study uses the four existing
forwarding/scheduling combinations plus shortest-protected and ecmp-protected. Eight scenarios,
three seeds and six policies yield **144 runs**. Two one-factor ablations retain destination
bottleneck traffic but remove reserved slots or set lower-class weights to [1,1,1]. All
parameters are fixed before results are examined, and negative findings are retained.

```sh
python scripts/run_all_experiments.py --suite protected-qos
python scripts/run_all_experiments.py --suite warehouse --baseline results/experiments-20261030T140941Z-dd164497
```

`configs/warehouse_protected.json` is a standalone destination-bottleneck example. The original
132-run warehouse suite remains the default. See traffic-requirements.md for the offered
traffic contract and acceptance checks. Neither protected buffering nor ECMP increases the
capacity of a destination link. Real 5G/Wi-Fi scheduling and UPF paths remain separate work.
