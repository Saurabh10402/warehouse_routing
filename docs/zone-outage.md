# Zone-switch failure and recovery

The redundant spines do not protect a robot's single attachment to its zone switch. This native wired demonstration uses the same 500-device, 482-source all-robot layout, but takes Zone A's sole switch down at 30 ms and restores it at 70 ms. Positions, traffic, routing and queues otherwise remain unchanged.

Reproduce after native setup:

```sh
python scripts/run_warehouse_layout.py --layout configs/warehouse_layout_all_robots.json --zone-outage A
```

The selected zone must exist and have exactly one switch. `--zone-outage` and `--spine-outage` are mutually exclusive. To inspect a completed batch, use `scripts/visualize_warehouse.py --batch PATH --output PATH/warehouse-viewer.html`.

Validated batch: `layout-20261102T171806Z-fe0a210a`. All 59 Zone A source flows and 20 incoming cross-zone flows are affected. The remaining 403 flows do not require Zone A. Service flows with an available backup outside the failed zone are classified as unaffected. This specific profile uses one local service candidate.

| Phase | Affected offered / delivered | Unaffected offered / delivered |
|---|---:|---:|
| Before failure (0, 20 ms) | 158 / 158 | 806 / 806 |
| Outage (40, 60 ms) | 158 / 0 | 806 / 806 |
| Recovered (80 ms) | 79 / 79 | 403 / 403 |

Total: 2,410 offered, 2,252 delivered, 158 `no_route` losses and deadline misses. The rejected packets never entered the network; they are still included in offered-traffic and deadline denominators. Restoration enables newly generated traffic; lost packets are not retransmitted by this model.

Both native executions passed independent trace audits and all seven raw replay artifacts matched exactly. Phase assertions verify loss within the isolated zone, delivery outside it and resumed delivery after recovery. No simulator implementation or protected-queue parameters changed.

Open the zone-switch demo from OPEN_WAREHOUSE.html. The first flow is `zone-A-robot-0-same-zone`: select #1 (20 ms) for normal delivery, #2 (40 ms) for `no_route` and the red DOWN switch, then #4 (80 ms) for restored delivery. Select `zone-B-robot-0-same-zone`, #297 (40 ms), to see unaffected communication during the outage. Incoming Zone H cross-zone flows also lose their route to Zone A.

Failure knowledge and table updates are immediate and centralized. Samples are spaced away from event boundaries; this is not a hardware convergence test or in-flight loss experiment. Native event-boundary controls remain separate prior evidence. This light illustrative workload does not establish capacity or deployment deadline guarantees. There is no radio modeling or executed Mac/Ubuntu validation.
