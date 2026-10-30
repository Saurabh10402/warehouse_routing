# Configuration files

Primary experiments use **schemaVersion 2**, loaded by `ealr::fabric::load`. It explicitly
represents arbitrary/unequal zones, fixed nodes, links, workloads, priorities and events.
See [fabric-model.md](fabric-model.md#configuration-and-topology) and the warehouse_small,
warehouse_uneven and warehouse_reference JSON examples. No random mobility is accepted.
The sections below document preserved legacy schemas. Their equal-zone planning restrictions
do not apply to schema v2; they are not the current architectural requirements.

`ConfigurationLoader::load()` turns a JSON scenario into an `ExperimentConfiguration` before the simulation is created. The loader accepts the following top-level sections: `simulation`, `routing`, `battery`, `traffic`, `mobility`, `robots`, `chargingStation`, `metrics`, `private5g`, and `output`.

All required fields are type checked. Numeric values must be finite; counts, packet rates, and packet sizes must be positive integers. Battery percentages are constrained to 0–100 and their thresholds must satisfy `0 <= critical < low <= 100`. Unknown properties are rejected to prevent silent configuration typos.

`private5g` supplies the gNB position, carrier frequency, channel bandwidth, numerology, transmit powers, and 5G-LENA channel factory names. `mobility` also provides random-waypoint speed and pause values. These inputs keep ns-3 and NR setup free of experiment-specific constants.

Scenario files can inherit from another scenario with `"extends": "baseline.json"`. The parent is resolved relative to the child file and nested objects merge recursively, allowing a scenario to override only the relevant fields. Circular inheritance is rejected. The supplied derived configurations follow this pattern.

Errors are reported as `ConfigurationError` with the failed file, section, and property whenever possible.

## Warehouse leaf-spine plan

`configs/warehouse_leaf_spine.json` is intentionally separate from the legacy multi-hop robot experiment files. It describes the new warehouse design: zone count, endpoint density, redundant leaf–spine links, vendor-neutral radio policy, latency targets, and temperature-safety thresholds.

This separation lets the operational plan change without changing C++ code. In particular, `warningCelsius`, `stopCelsius`, and all priority latency targets are editable in that JSON file. The loader rejects unsafe plans such as a single spine, a single head node, inverted temperature thresholds, or latency classes that are not ordered from most to least urgent.

`requireManualResetAfterStop` defaults to `true`. A stop event must remain latched until the current temperature is below the warning threshold and an authorised operator or safety system acknowledges it. This is simulation/control-plane logic only; a production emergency-stop circuit must still be independently designed, certified, and validated.

`deviceProfiles` is the device-assignment section of that plan. A profile has a device count, one access technology (`wired`, `wifi`, or `private5g`), and a traffic priority (`safety`, `control`, `normal`, or `background`). Profile counts must add up to the planned endpoint total. The current editable defaults use wired critical temperature sensors, private-5G robot-control endpoints, Wi-Fi normal operations, and Wi-Fi background telemetry.
