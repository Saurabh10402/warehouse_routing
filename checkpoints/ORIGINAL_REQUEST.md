# MASTER PROMPT: COMPLETE, VALIDATE AND RECOVER THE WIPRO-FUNDED WAREHOUSE NETWORKING PROJECT

## 1. YOUR ROLE AND PRIMARY MISSION

Act as a senior C++ engineer, network systems architect, ns-3/5G-LENA simulation specialist, routing-algorithm researcher, software testing engineer, and technical mentor.

I am continuing an existing WIPRO-funded research project. Some development progress may have been lost, and I need you to recover the current implementation, finish as much of the project as possible, validate it, and prepare it for reliable continuation.

My Mac is temporarily unavailable. Your session or subscription access may expire before I regain access to it. Therefore, I cannot rely on being available to debug code, install dependencies, rerun commands, or repair unfinished integration work later.

**Your mission is to leave me with a working, tested, reproducible project requiring as little manual repair as reasonably possible.**

Do not merely inspect the code, give recommendations, write a roadmap, or generate isolated code snippets. Work directly on the uploaded repository wherever your environment permits. Execute builds, run tests, inspect failures, implement corrections, repeat the evaluation cycle, and preserve checkpoints throughout the process.

Do not promise that code works unless you have actually verified the relevant behavior.

When a capability cannot be verified because of a genuine external limitation, complete the independent work that can be tested, document the limitation precisely, and isolate the remaining integration task.

## 2. PROJECT CONTEXT

The project concerns low-latency, priority-aware communication in an industrial warehouse using a private 5G-enabled network with wired and Wi-Fi connectivity.

The conceptual objective is to improve latency and reliability while considering:

- Traffic criticality and deadline requirements.
- Routing correctness.
- Congestion and queueing.
- Network redundancy and link failures.
- Energy efficiency for genuinely energy-constrained devices.
- Local safety processing.
- Reproducible performance evaluation.

The project is WIPRO-funded.

The proposed routing algorithm is called **EALR — Energy-Aware Low-Latency Routing**. Existing routing implementations include Dijkstra and Greedy algorithms.

The intended research outcome is a defensible comparison of routing strategies under different network conditions, including traffic priorities, congestion, low-battery relays where relevant, and topology failures.

Do not assume the existing code correctly implements all these capabilities. Verify the implementation.

## 3. FILES YOU MUST INSPECT

The uploaded archive is:

`Routing-WIPRO-20261029T174553Z-1-001.zip`

The repository is named `Routing-WIPRO/`.

Important existing files include:

### Build and project configuration

- `README.md`
- `CMakeLists.txt`
- `CMakePresets.json`
- `cmake/CompilerWarnings.cmake`
- `cmake/EalrDependencies.cmake`

### Routing

- `include/ealr/routing/routing_algorithm.hpp`
- `include/ealr/routing/routing_types.hpp`
- `include/ealr/routing/dijkstra_routing.hpp`
- `include/ealr/routing/greedy_routing.hpp`
- `include/ealr/routing/ealr_routing.hpp`
- `src/routing/routing_types.cpp`
- `src/routing/dijkstra_routing.cpp`
- `src/routing/greedy_routing.cpp`
- `src/routing/ealr_routing.cpp`

### Robot, network and safety

- `include/ealr/robot/robot.hpp`
- `src/robot/robot.cpp`
- `src/network/device_policy.cpp`
- `src/safety/temperature_safety_controller.cpp`

### Configuration

- `include/ealr/config/configuration.hpp`
- `include/ealr/config/warehouse_configuration.hpp`
- `configs/baseline.json`
- `configs/low_battery.json`
- `configs/dense_network.json`
- `configs/high_traffic.json`
- `configs/factory_layout.json`
- `configs/random_layout.json`
- `configs/warehouse_leaf_spine.json`

### Simulation, metrics and visualization

- `include/ealr/simulation/ns3_private_5g_scenario.hpp`
- `src/simulation/ns3_private_5g_scenario.cpp`
- `src/simulation/nr_access_network.cpp`
- `include/ealr/metrics/metrics_collector.hpp`
- `src/metrics/metrics_collector.cpp`
- `src/metrics/ns3_flow_monitor_collector.cpp`
- `src/visualization/netanim_visualization.cpp`
- `src/app/ealr_ns3_main.cpp`

### Tests, experiments and documentation

- `tests/`
- `tests/fixtures/`
- `scripts/plot_results.py`
- `scripts/requirements.txt`
- `docs/architecture.md`
- `docs/routing.md`
- `docs/metrics.md`
- `docs/configuration.md`
- `docs/ns3-integration.md`
- `docs/netanim.md`
- `docs/plotting.md`
- `results/`
- `logs/`

Inspect the complete source tree rather than limiting yourself to this list.

The archive contains build outputs and historical logs. Treat them as evidence only, not proof that the current source builds.

Inspect the actual Git state and establish a new recoverable baseline if the archive lacks usable history.

## 4. FINALIZED ARCHITECTURE: REFERENCE DESIGN, NOT A FIXED TOPOLOGY

I have finalized the architectural direction shown in the uploaded image:

**Low-Latency Warehouse Network — Priority-Aware Leaf–Spine Architecture.**

Treat the image as the approved reference design. However, the actual warehouse layout has not yet been finalized.

This distinction is mandatory.

The reference image depicts approximately 500 endpoints across ten zones, one leaf switch per zone, two spine switches, two redundant edge/head nodes, multiple access technologies, local safety processing and four traffic priorities.

These counts are examples of the current reference configuration. They are NOT immutable system-wide requirements.

The physical warehouse may eventually have:

- More or fewer zones than ten.
- Unequal device counts across zones.
- Some zones with approximately ten endpoints and others with 70–80 or more.
- Different numbers of leaf switches.
- Different numbers of spine switches where the selected topology permits them.
- Different edge/head-node configurations.
- Different access technologies in different zones.
- Different uplink arrangements and link capacities.
- Different numbers and types of sensors, industrial controllers, scanners, cameras and other endpoints.

Build a flexible architecture model that can represent these layouts without requiring modifications to routing algorithm source code.

### Required architectural principles

Preserve these principles from the reference design:

1. Leaf–spine networking when selected by the configuration.
2. Redundant core paths where configured.
3. Primary and secondary edge/head nodes where configured.
4. Local processing for critical safety events.
5. Wired Ethernet, Wi-Fi and private 5G access as supported by the implementation.
6. Four configurable traffic priorities.
7. Local warehouse traffic that avoids unnecessary cloud dependencies.
8. Explicit link capacities, delays, availability and failure behavior.
9. A stationary physical topology for the primary experiments.
10. Configurable topology, traffic and safety parameters.

Do not redesign the architecture unnecessarily. Do not hardcode the number of zones, endpoints, leaves or spines into the routing implementation.

If a proposed configuration is unsupported or physically inconsistent, validate it and report the problem rather than silently constructing a different topology.

## 5. STATIONARY NETWORK REQUIREMENT

The primary research scope is a stationary warehouse network, not a mobile-robot navigation simulation.

All physical nodes must retain fixed positions during the primary experiments, including:

- Warehouse endpoints.
- Sensors and controllers.
- Access points.
- Leaf switches.
- Spine switches.
- Edge/head nodes.
- Servers and other fixed infrastructure.

Network conditions may change dynamically. Examples include:

- Queue growth.
- Traffic arrival.
- Congestion.
- Battery depletion for energy-constrained devices.
- Link failures.
- Node failures.
- Safety events.
- Route recomputation.
- Recovery after a failure.

These behaviors do not imply physical mobility.

Use constant-position models in ns-3 when required. Remove random movement from the primary experiment workflow.

The repository may contain legacy `randomWaypoint` functionality. Audit it and ensure the recommended experiments do not invoke it.

If legacy mobility code is still useful, isolate it as an optional, explicitly out-of-scope example rather than allowing it to influence the primary project.

Add a test or runtime validation proving that node positions remain unchanged throughout the primary simulation.

Do not remove useful routing code merely because existing class names refer to robots. Reuse it where technically appropriate, but do not claim to simulate moving robots.

## 6. FIRST PHASE: RECOVER AND AUDIT THE EXISTING PROJECT

Before substantial edits:

1. Extract the uploaded ZIP into a clean workspace.
2. Preserve the original archive.
3. Inspect the source tree and actual Git metadata.
4. Establish a recoverable source baseline.
5. Read the build files, README, architecture documents, routing interfaces, algorithm implementations, configuration loaders, simulation runner, safety controller, metrics pipeline, visualization code and tests.
6. Identify which components are actually invoked during execution.
7. Separate implemented behavior from documentation, configuration and intended future functionality.
8. Inspect stale CMake caches and build artifacts without assuming they are valid.
9. Check the available compiler, CMake version, Boost.JSON, ns-3 and 5G-LENA dependencies.
10. Run the available baseline build and tests.
11. Record failures with their actual causes.

Produce an initial status report containing:

- What already exists.
- What successfully builds.
- Which tests actually pass.
- What is incomplete or broken.
- What is blocked by unavailable dependencies.
- The most important correctness defects.
- The first concrete implementation task.

After producing the report, continue working. Do not wait for me to approve every routine, reversible change.

## 7. CONFIGURATION-DRIVEN ARCHITECTURE

Create or improve a configuration model that separates:

- Warehouse metadata.
- Zones.
- Endpoints and endpoint types.
- Zone membership.
- Access networks.
- Leaf and spine switches.
- Edge/head nodes.
- Links and their properties.
- Routing policies.
- Traffic classes and workloads.
- Safety-controller parameters.
- Simulation parameters.
- Experiment seeds and output settings.

Use the existing JSON infrastructure if practical. Extend it cleanly rather than introducing an unrelated second configuration system.

If the current dependency setup prevents a reliable core build, investigate the smallest maintainable solution. Do not introduce duplicate parsers or unnecessary dependencies without justification.

### Configuration requirements

Support arbitrary valid zone counts and unequal zone sizes.

For example, the following must be representable without changing routing code:

- Zone A: 10 endpoints.
- Zone B: 35 endpoints.
- Zone C: 75 endpoints.
- Zone D: 20 endpoints.

Do not assume the total number of endpoints is divisible by the number of zones.

Do not assume that all endpoints have the same access technology.

Do not assume each endpoint has a dedicated leaf switch.

Do not assume that every warehouse requires exactly two spines or exactly two edge nodes. Make these configurable within the supported topology model.

Do not force a leaf–spine topology when a deliberately configured test topology or another supported topology is needed.

### Required example configurations

Maintain at least these three examples:

**Reference warehouse:** approximately 500 endpoints, ten zones, dual spines and redundant edge nodes.

**Uneven warehouse:** a different number of zones with unequal endpoint counts, including zones with approximately 10 and 70–80 endpoints.

**Small deterministic topology:** a minimal network for rapid tests, debugging and repeatable algorithm comparisons.

These examples demonstrate configurability. They do not represent the final physical warehouse.

### Validation

Reject or clearly report:

- Duplicate IDs.
- Missing node references.
- Invalid zone membership.
- Invalid link endpoints.
- Invalid capacities or delays.
- Negative or non-finite routing costs.
- Invalid weights.
- Unsupported access technologies.
- Invalid priority values.
- Invalid safety thresholds.
- Contradictory topology constraints.
- Unreachable mandatory infrastructure.
- Invalid traffic definitions.

Provide actionable error messages that identify the affected field or entity.

Use documented defaults only for optional fields.

## 8. ROUTING ALGORITHMS AND EALR

Preserve the existing Dijkstra, Greedy and EALR implementations where useful.

Audit the actual implementation before changing its mathematical model.

The documented EALR cost function is:

    cost(u -> v) =
        alpha * distance(u, v)
        + beta * batteryPenalty(v)
        + gamma * congestionPenalty(v)

The documented battery penalty is:

    batteryPenalty(v) =
        (100 - remainingBatteryPercent(v)) / 100

The documented congestion penalty uses the candidate forwarding node's queue length.

These are the existing design intentions, not proof that the current implementation is correct.

### Required routing behavior

- Correct path selection.
- Explicit handling of unreachable destinations.
- Correct behavior for source equal to destination.
- Consistent source, destination and relay treatment.
- No invalid routes through excluded nodes.
- Finite, nonnegative edge costs.
- Validated configuration weights.
- Deterministic tie-breaking where practical.
- Tests for low-battery and congested relays.
- Tests for disconnected graphs and unavailable links.
- Correct behavior after topology changes.

### Important architectural distinction

The warehouse fabric contains wired switches and mains-powered infrastructure. Do not assign battery penalties to fixed infrastructure that has no relevant energy constraint.

Distinguish:

**Fixed-fabric routing:** choose paths through the configured switching topology using link availability, latency, congestion, capacity and applicable priority constraints.

**Energy-aware endpoint/relay routing:** account for battery state only for devices that genuinely have energy limitations.

Use distance-based Dijkstra as a baseline where meaningful, but do not treat Euclidean distance as a substitute for actual network latency.

For the fabric, use configured link costs and the actual topology graph.

Evaluate whether EALR's weighted sum is suitable. Distance, battery fraction and queue length have different units, so explain how terms are normalized and what the weights mean.

Do not select weights simply to make EALR win.

If a strict latency constraint, lexicographic objective or another model is more defensible, explain the trade-off and implement the simplest suitable approach.

Energy awareness must not cause unacceptable latency increases, and low-latency routing must not repeatedly drain a small set of energy-constrained relays.

## 9. TRAFFIC PRIORITIES AND LATENCY TARGETS

Implement four explicit traffic classes.

**Priority 0 — Safety emergency**

- Target: less than 10 ms.
- Examples: critical temperature events, fire/gas alarms and emergency stops.
- Highest priority, appropriate redundancy and local processing.

**Priority 1 — Robot and industrial control**

- Target: less than 20 ms.
- Examples: PLC commands and industrial control messages.

**Priority 2 — Normal warehouse operations**

- Target: less than 40 ms.
- Examples: scanners, routine status and ordinary sensor traffic.

**Priority 3 — Background data**

- Target: less than 100 ms, consistent with the warehouse configuration.
- Examples: logs, routine monitoring and noncritical updates.

Treat these as configured research targets, not guaranteed performance.

Implement actual priority-dependent behavior. A JSON label or priority-specific routing weight alone is not sufficient.

Where supported, implement priority-aware queues or scheduling, traffic classification, and appropriate packet-forwarding behavior.

Measure latency, packet delivery and deadline misses separately for each class.

Test under competing traffic loads. Keep topology and offered traffic consistent when comparing algorithms.

If the current ns-3 modules cannot support the intended QoS mechanism, document the limitation and implement a clearly labeled abstraction in the independently testable core model.

Do not claim that a simulation abstraction represents actual 5G NR scheduling or QoS enforcement.

## 10. COMPLETE END-TO-END SIMULATION

The existing repository may contain routing calculations without fully integrating those calculations into packet forwarding.

Investigate this carefully.

The completed project must have at least one coherent, executable end-to-end experiment path:

1. Load and validate the selected configuration.
2. Construct the configured stationary topology.
3. Attach endpoints to the correct zones and access networks.
4. Build actual graph connectivity.
5. Apply configured link capacities and delays.
6. Generate traffic for the four priorities.
7. Select routes using the configured algorithm.
8. Forward packets along the selected routes in the supported simulation model.
9. Model queues, transmission delay, contention or other relevant behavior at an explicitly documented fidelity level.
10. Handle configured link and node failures.
11. Apply local safety-event behavior.
12. Record end-to-end measurements.
13. Export reproducible results and configuration snapshots.

Do not report graph-level estimated latency as real ns-3 packet latency.

If the full ns-3/5G-LENA integration cannot be built in the current environment, establish an independently testable C++ core simulation that accurately models the behavior it claims to model.

Do not call this independent simulator ns-3 or claim that it implements a full 5G core.

Where practical, make the ns-3 adapter consume the same configuration and topology model to reduce duplicated logic.

The independent core build must not depend on a full ns-3 installation.

## 11. NS-3, 5G-LENA AND BUILD COMPATIBILITY

The original environment included:

- macOS on a MacBook Air M4.
- UTM with Ubuntu 24.04.
- C++17 for the core framework.
- CMake 3.22 or newer.
- Ninja.
- Boost.JSON.
- ns-3.
- Optional 5G-LENA NR integration.
- Python for experiment plotting.

The existing documentation notes a potential C++ standard difference between the core library and ns-3 3.46.

Inspect the actual targets and compiler settings. Use appropriate per-target language standards instead of blindly changing the whole project.

Try the documented bootstrap build:

    cmake --preset bootstrap
    cmake --build --preset bootstrap
    ctest --preset bootstrap

Then attempt the default build when the required dependencies are available:

    cmake --preset default
    cmake --build --preset default
    ctest --preset default

These commands are starting points, not assumed successes.

Resolve dependency errors where possible.

Inspect stale caches for machine-specific paths. Reconfigure in a clean build directory.

If the current environment lacks Boost.JSON or ns-3, report the exact blocker and provide a reproducible installation or build procedure.

The bootstrap target must remain useful without ns-3.

The optional simulation target must report missing dependencies clearly and must never silently claim that unavailable capabilities were tested.

## 12. SAFETY CONTROLLER

Inspect the existing temperature-safety controller, configuration loader, tests and visualization.

The current warehouse configuration specifies:

- Warning threshold: 70°C.
- Stop threshold: 80°C.
- Emergency-stop broadcast enabled.
- Local edge processing enabled.
- Manual reset required after a stop.

Validate that these values are configurable and consumed correctly.

Test:

- Warning transition.
- Stop transition.
- Threshold boundaries.
- Invalid threshold combinations.
- Latched stop behavior.
- Required manual reset.
- Recovery conditions.
- Local response when the cloud is unavailable.
- Priority of safety traffic.
- Safety-event logging and measured response time.

Keep local safety handling independent of remote analytics and cloud availability.

This is a research simulation, not a certified safety system. Do not claim industrial functional-safety certification or real-world emergency-stop guarantees.

## 13. REDUNDANCY, FAILURES AND RECOVERY

Make redundancy configurable.

Test applicable scenarios:

- Normal operation.
- Failure of one spine.
- Failure of a leaf uplink.
- Failure of a primary edge/head node.
- Congestion on a path.
- Cloud disconnection.
- A low-battery relay becoming ineligible.
- Complete loss of connectivity.
- Recovery after a failed component returns, if supported.

Verify that route recomputation occurs according to a documented mechanism.

Demonstrate that alternate paths carry traffic when available capacity permits.

Do not claim failover based solely on the existence of redundant components in the configuration.

## 14. METRICS AND RESEARCH EVALUATION

Inspect and validate the metrics collector, FlowMonitor integration, CSV output and plotting scripts.

Collect, where supported:

- Mean end-to-end latency.
- Median latency.
- p95 and p99 latency when sufficient per-packet observations exist.
- Packet delivery ratio.
- Packet loss.
- Throughput.
- Deadline-miss rate.
- Per-priority performance.
- Energy consumption for energy-constrained devices.
- Minimum and average battery remaining.
- Network lifetime using an explicit definition.
- Route recomputations.
- Failure recovery time.
- Routing computation time where useful.

Do not calculate percentiles from averages.

Define each metric's units, denominator, observation window and failure semantics.

Handle zero transmitted packets safely.

Distinguish application throughput from lower-layer throughput.

Generate comparative experiments for:

1. Baseline topology.
2. Uneven zone sizes.
3. Low-battery relays.
4. High traffic and congestion.
5. Different priority mixes.
6. Link and node failures.
7. Critical safety events.
8. Increasing endpoint counts.
9. Multiple random seeds where stochastic behavior exists.
10. Ablation experiments separating energy, congestion and priority effects.

Use identical configurations, offered traffic and seed conditions when comparing algorithms wherever possible.

Use multiple independent seeds for stochastic experiments. Report sample counts and variability.

Never invent results or tune the algorithm solely to win a particular benchmark.

If EALR performs worse than Dijkstra on a metric, report it honestly and investigate why.

Every reported result must be traceable to the source state, configuration and seed that generated it.

## 15. CHECKPOINT AND SESSION-RECOVERY SYSTEM

My access to Astra may expire before I regain access to my Mac. You must preserve the project state throughout development.

Create a durable checkpoint system inside the repository.

Use a structure such as:

    checkpoints/
      CURRENT_STATUS.md
      PROJECT_STATE.json
      CHANGELOG.md
      DECISION_LOG.md
      TEST_STATUS.md
      NEXT_STEPS.md
      sessions/
        session_001.md
        session_002.md
      manifests/
        checkpoint_001.json
        checkpoint_002.json

Adapt this layout if the repository already has suitable files, but ensure all required information is recorded.

### Required checkpoint content

**CURRENT_STATUS.md**

Record the current phase, latest completed milestone, last known-good build and test status, latest experiment status, current blockers, checkpoint identifier and exact next action.

**PROJECT_STATE.json**

Record the schema version, project identifier, checkpoint identifier, UTC timestamp, current phase, source state, configuration versions, implemented and pending features, known defects, build status, test status, experiment identifiers, dependencies, blockers and next action.

Validate that it is valid JSON.

**CHANGELOG.md**

Record meaningful changes, affected files, reasons, tests executed, actual results and unresolved issues.

**DECISION_LOG.md**

Record significant architectural and algorithmic decisions, alternatives considered, reasoning and trade-offs.

**TEST_STATUS.md**

Record PASS, FAIL, BLOCKED or NOT RUN for each major build target, test suite, routing behavior, topology behavior, priority mechanism, safety behavior, failure scenario and experiment.

**NEXT_STEPS.md**

List the next tasks in priority order, with dependencies and acceptance criteria.

### Checkpoint rules

Create checkpoints:

- Before substantial or destructive changes.
- After the initial audit.
- After a recoverable baseline is established.
- After each major implementation milestone.
- After significant bug fixes.
- After long-running experiments.
- Before the session ends or its limit approaches.
- After final verification.

Use Git commits when available, with meaningful commit messages. Associate checkpoints with the corresponding commit or source-tree hash where practical.

Do not repeatedly copy large build directories into checkpoints.

Do not claim a checkpoint exists unless its files have been written and verified.

Never overwrite the only known-good state.

If the repository lacks usable Git history, inspect the source state and initialize a fresh baseline before substantial modifications.

### Resume protocol

At the beginning of every new session:

1. Read CURRENT_STATUS.md.
2. Read PROJECT_STATE.json.
3. Read NEXT_STEPS.md.
4. Read the latest relevant session summary.
5. Inspect the current source and Git status.
6. Verify that the source corresponds to the recorded checkpoint.
7. Run a small verification command where practical.
8. Continue from the next action without repeating completed work unnecessarily.

The checkpoint files must contain enough context for another session to continue even without access to this conversation.

Do not store credentials or secrets in checkpoints.

## 16. MANDATORY ITERATIVE DEVELOPMENT LOOP

Work continuously through the following loop:

    INSPECT
       |
       v
    IDENTIFY THE HIGHEST-PRIORITY DEFECT
       |
       v
    DEFINE A TESTABLE ACCEPTANCE CRITERION
       |
       v
    IMPLEMENT A SMALL, COHERENT CHANGE
       |
       v
    BUILD AND RUN TESTS
       |
       v
    INSPECT THE DIFF AND CHECK FOR REGRESSIONS
       |
       +---- Failure found ----> Diagnose and fix
       |                            |
       |                            v
       |                         Re-test
       |
       +---- Pass --------------> Run relevant experiments
                                    |
                                    v
                              Evaluate results
                                    |
                                    v
                               Create checkpoint
                                    |
                                    v
                           Select the next defect

Do not perform a large untested rewrite.

After each meaningful cycle:

- Review the code changes.
- Run relevant tests.
- Run the full available test suite when appropriate.
- Evaluate correctness against explicit acceptance criteria.
- Inspect performance and research metrics.
- Update the documentation if behavior changed.
- Create and verify a checkpoint.
- Continue with the next highest-priority issue.

When an experiment reveals a defect, fix the cause rather than altering expected results.

Do not rewrite working code unnecessarily.

If a proposed optimization performs worse than the baseline, investigate the trade-off and report it honestly.

If a dependency is unavailable, complete the independent work that can still be tested.

Do not stop after the first successful build. A compiling project can still have incorrect routing, broken forwarding, invalid metrics or nonfunctional failover.

## 17. TESTING AND CODE QUALITY

Use the existing test framework and project conventions wherever practical.

Cover:

- Configuration loading and validation.
- Unequal zone sizes.
- Arbitrary supported zone counts.
- Topology construction.
- Stationary-node invariants.
- Duplicate identifiers.
- Invalid and disconnected links.
- Dijkstra correctness.
- Greedy failure behavior.
- EALR cost calculation.
- Invalid and non-finite weights.
- Low-battery relay handling.
- Queueing and congestion.
- Priority-dependent forwarding.
- Deadline calculations.
- Link and node failure.
- Route recomputation.
- Safety thresholds and manual reset.
- Metric aggregation.
- Zero-packet cases.
- CSV output.
- Visualization output validity.
- Reproducibility across repeated runs.

Preserve clean modular boundaries.

Routing algorithms should not parse JSON directly. Metrics should not own topology. Visualization should not determine routing decisions. Simulation orchestration should not duplicate algorithm logic.

Use RAII, clear ownership, const-correctness, validated configuration, explicit units and actionable errors.

Avoid undefined behavior, unexplained constants, unnecessary dependencies, machine-specific paths and duplicated implementations.

Do not suppress warnings, disable tests or silently substitute fallback behavior merely to obtain a successful build.

## 18. ONE-COMMAND REPRODUCIBILITY

Create a documented command such as:

    ./scripts/run_all_experiments.sh

or an equivalent build-system target.

It should:

1. Validate prerequisites.
2. Configure and build the core framework.
3. Run the applicable tests.
4. Run deterministic small-scale experiments.
5. Compare the supported routing algorithms.
6. Run selected priority and failure scenarios.
7. Generate raw metrics and summaries.
8. Save the exact configuration and seed for every run.
9. Generate comparison plots if plotting dependencies are available.
10. Exit nonzero when a required step fails.
11. Print the output location and a concise execution summary.

Do not make Python plotting dependencies mandatory for compiling the core framework.

Use safe output-directory management and avoid silently overwriting previous research results.

## 19. WORKING RULES

These rules are mandatory:

1. Inspect before editing.
2. Preserve existing useful functionality.
3. Never assume that a filename proves an implementation exists.
4. Never claim a test passed without executing it.
5. Never invent performance results.
6. Never claim actual 5G-LENA use without verifying the relevant build and runtime behavior.
7. Never claim that packets follow EALR routes without verifying the forwarding integration.
8. Never claim priority guarantees from a routing cost function alone.
9. Never claim a complete leaf–spine simulation when only an abstract graph is modeled.
10. Never assume ten zones or equal zone sizes.
11. Never enable physical mobility in the primary workflow.
12. Never delete the only known-good state.
13. Never make large unrelated refactors.
14. Never change experiment parameters silently.
15. Never commit secrets.
16. Never stop after producing a roadmap.
17. Never claim completion merely because the code compiles.
18. Ask for approval before destructive actions.
19. If a task is blocked, explain the precise reason and continue with independent tasks.
20. If the context or usage limit approaches, checkpoint the current state and leave a clear recovery instruction.

For significant changes, explain what was wrong, why it mattered, what changed, which tests ran, what passed, what failed and what remains uncertain.

## 20. FINAL ACCEPTANCE CRITERIA

The completed project should provide:

- A configurable warehouse topology rather than a hardcoded ten-zone design.
- Support for unequal zone sizes and variable endpoint counts.
- Fixed physical nodes throughout the primary experiment workflow.
- Correct Dijkstra, Greedy and EALR behavior where each is applicable.
- A clear distinction between energy-aware relay routing and fixed-fabric routing.
- An executable end-to-end packet-forwarding model.
- Actual priority-dependent behavior in the supported simulation.
- Local safety processing and tested emergency behavior.
- Configurable redundancy and tested failure handling.
- Correct, reproducible metrics.
- Automated tests and a clean core build.
- Optional ns-3/5G-LENA integration with honestly documented verification status.
- Reproducible experiment commands.
- Accurate documentation.
- Durable checkpoints and reliable session recovery.
- A final report separating verified features, abstractions, limitations and remaining external blockers.

Do not fabricate results or claim latency targets are met unless the measured experiments support that conclusion.

A smaller, complete, reproducible and scientifically honest research prototype is preferable to a large collection of partially integrated features.

## 21. START WORKING NOW

Start with the uploaded ZIP.

First, establish a recoverable checkpoint and audit the real code.

Then stabilize the core build, make topology generation configuration-driven, verify stationary-node behavior, and complete the end-to-end routing and traffic model.

Continue through priority handling, safety behavior, failures, metrics, experiments and documentation.

Use the evaluation-and-correction loop throughout. Checkpoint every meaningful milestone.

Do not stop after the initial report. Do not wait for me to approve every routine engineering change.

At the end of each session, report:

- The latest checkpoint identifier.
- The associated source state or Git commit.
- What was implemented.
- The actual commands and tests executed.
- What is verified.
- What remains blocked or unverified.
- The exact next action for the next session.

Your final objective is to leave me with a recoverable, validated project that can be resumed by another session and requires minimal manual debugging when I regain access to my Mac.

Begin with the repository, not assumptions.