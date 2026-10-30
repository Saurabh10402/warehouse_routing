# Architecture

## Primary stationary warehouse workflow

`fabric_configuration` parses/validates schema-v2 explicit nodes, links, workloads and events.
`fabric_simulator` owns the event calendar, queues, forwarding, availability and constrained
device energy. The primary powered-fabric study uses Dijkstra/ECMP with FIFO/priority queues;
Greedy/EALR are preserved as legacy comparisons. All use an optional link-cost callback; legacy
distance behavior remains the default when no callback is supplied. `fabric_metrics` aggregates
raw packet outcomes without mutating topology. The CLI writes results; Python orchestrates
fair comparisons and provenance. See [fabric model](fabric-model.md) for the executable contract.
See [warehouse study](warehouse-study.md) for peer/service paths, failure domains and the
current controlled experiment design. The first study isolates wired fabric behavior.

The following section describes the retained legacy ns-3/robot design; some named modules
there were planned interfaces, not implemented orchestration.

The framework is divided by responsibility so experiments remain reproducible and routing can evolve independently of ns-3 scenario setup.

```text
config -> simulation -> {mobility, traffic, robot, energy, routing}
                              |                 |
                              +---- metrics ----+---- visualization
```

`config` owns JSON parsing and validation. `simulation` creates and coordinates ns-3/5G-LENA objects. `robot` owns robot identity, position, neighbour data, queue length, and battery-driven availability state. `energy` calculates consumption and applies it through the robot API. `routing` selects routes from an already-prepared topology and must not know about JSON or output files. `metrics` records outcomes, exports reports, and receives ns-3 FlowMonitor counters through an adapter; `visualization` converts simulation state to NetAnim artifacts; `scripts` create comparison charts from exported CSV data.

New algorithms implement the routing interface under `include/ealr/routing/` and `src/routing/`. They receive topology, battery, and congestion inputs through domain types, making it possible to add obstacle or network-slicing modules later without coupling routing to configuration or presentation concerns.
