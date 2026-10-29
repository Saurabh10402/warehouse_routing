# Routing contract

All routing algorithms implement `RoutingAlgorithm`. They receive a `RoutingRequest` containing immutable `RoutingTopology` data and return a `RouteResult`; the hop sequence always starts at the source and ends at the destination.

`RoutingTopology` is created after neighbour discovery and validates robot IDs, neighbour references, physical values, and duplicate links. It intentionally trusts the supplied adjacency graph instead of recomputing it from positions, which lets future radio and obstacle models control link availability.

Algorithms should begin with `validateRequest()`. It supplies consistent route statuses for invalid IDs and robots that have entered `ReturningToCharge`. A low-battery robot remains usable by routing, allowing EALR to make its own cost-based decision.

`DijkstraRouting` is the baseline implementation. It computes shortest paths using only the Euclidean distance of each permitted neighbour link and skips robots in `ReturningToCharge` state. `GreedyRouting` selects the unvisited neighbour closest to the destination at each hop and deliberately does not backtrack; it is fast but may miss an available route or choose a longer one.

`EalrRouting` uses a shortest-path search with a composite forwarding cost: `alpha * distance + beta * batteryPenalty + gamma * congestionPenalty`. Battery penalty is `(100 - remainingBatteryPercent) / 100`, so it grows from 0 at full charge to 1 at empty. Congestion penalty is the candidate forwarding robot's queue length. `EalrCostWeights` validates the configurable `alpha`, `beta`, and `gamma` coefficients before routing begins.
