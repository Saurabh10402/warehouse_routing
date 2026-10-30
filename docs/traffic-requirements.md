# Traffic and acceptance specification — experimental version 1

## Purpose and assumptions

Evaluate stationary powered warehouse communication under congestion and component failure.
No measured warehouse trace or final floor plan has been supplied. The following values are
**declared experiment inputs**, not stakeholder-approved deployment requirements. The JSON
workloads and topology remain configurable. A real deployment needs an explicit acceptable
loss rate and availability target; neither is invented here.

Network delay is application packet generation to delivery in the core simulator. It excludes
application computation, sensor sampling, actuator response and real access protocols.
Deadline success requires delivery strictly before the configured deadline; lost, policed,
reservation-rejected and cutoff packets all count as deadline misses.

| Class | Illustrative use | Payload bytes | Deadline ms | Baseline interval ms | Contention interval ms |
|---|---|---:|---:|---:|---:|
| P0 | Emergency message | 96 | <10 | 200 | 10 |
| P1 | Industrial control | 256 | <20 | 200 | 10 |
| P2 | Routine operations | 512 | <40 | 200 | 5 |
| P3 | Background transfer | 1200 | <100 | 200 | 4 |

In the mixed examples, each endpoint has three flows: a same-zone peer, a next-zone peer,
and an ordered edge0/edge1 service. Endpoint index modulo four assigns the class only to
balance this synthetic experiment; real traffic must be classified per application/flow.
The 500-endpoint scaling case selects one scope per endpoint to keep its offered load modest.

Baseline observation: 1000 ms generation plus 200 ms drain, initial random phase [0,20) ms.
Contention: 200 ms generation plus 200 ms drain, phase [0,1) ms; two zones with eight endpoints
each, 10 Mbps leaf-spine directions, 32 waiting slots on those uplinks. Destination bottleneck
also sets zone1-ep0's attachment to 1 Mbps/32 slots and directs zone0 cross-zone flows there.
Unmentioned links retain the exact warehouse-study generator values. Forwarding and queue
policies always receive identical inputs and arrival phases within each seed.

## Stress traffic

- Emergency overload: contention topology, P0 interval 0.5 ms and phase [0,0.1) ms. This
  deliberately exceeds the emergency admission budget on some links.
- Synchronized emergency burst: destination bottleneck plus 32 additional 96-byte P0 messages
  at 50 ms, four from each zone0 endpoint to zone1-ep0. These are offered packets even when
  a rate limiter rejects them. The burst alone needs 24.576 ms on a 1 Mbps link, so all
  messages cannot meet a 10 ms deadline even without other traffic.
- Spine failure/restoration: the existing 300/700 ms failure schedule, retaining the
  simulator's explicitly ideal immediate failure knowledge.

## Acceptance before interpreting performance

1. Configuration validation rejects impossible buffer reservations, invalid rates/weights,
   and P0 payloads larger than the configured burst allowance.
2. Analytical tests establish serialization/blocking time, exact token refill, independent
   directions, reservation behavior, and byte-weighted lower-class service.
3. Every queue conserves admissions: attempts = admitted + all admission drops; admitted =
   service starts + reroutes + waiting at cutoff. These are per-hop observations.
4. Every generated packet remains in the end-to-end denominator. No filtering of rejected
   packets or reporting only admitted emergency traffic as the primary success metric.
5. The nominal communications case must meet its configured deadlines for all six policies.
   Stress cases are measurements, not pass/fail promises that every deadline is achievable.
6. The preserved four-policy 132-run warehouse matrix must reproduce its inputs and raw
   packet traces byte-for-byte. The new policy does not redefine the old baseline.
7. Three seeds, raw traces, per-scope/per-class outcomes, rejection causes, queue observations,
   deterministic replay and source/configuration/executable provenance are required.

Performance acceptance for the new policy is reported rather than assumed: determine whether
P0 deadline success improves at the same offered load, what other classes lose, how much
traffic is rejected, and where finite capacity or nonpreemptive blocking remains decisive.

## Inputs needed for calibration

Actual zone membership and device/access types; per-flow sizes, periods and burst envelopes;
acceptable loss/deadline-miss rates; network versus application deadline budgets; capacities;
and required failure tolerance. Until supplied, these experiments support mechanism studies
and capacity sensitivity, not a claim of deployment readiness.
