# Public headers

Public C++ headers are organised by the same module names as `src/`:

- `config/` — JSON loading, schema validation, and typed experiment settings.
- `energy/` — battery accounting and robot state transitions.
- `metrics/` — metric collection and CSV/summary export.
- `mobility/` — grid, static, and random-waypoint factory mobility adapters.
- `robot/` — robot domain model and neighbour representation.
- `routing/` — routing interface plus Dijkstra, Greedy, and EALR implementations.
- `simulation/` — orchestration of ns-3/5G-LENA scenarios.
- `traffic/` — packet generation and traffic schedules.
- `visualization/` — NetAnim output and presentation metadata.

Headers must not parse JSON, write plots, or own unrelated module behaviour.
