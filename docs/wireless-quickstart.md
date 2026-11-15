# Wireless simulation and offline viewer

## View the completed results

On this workspace,open `output/wireless-three-seeds-reviewed.html` in Chrome or Edge.
Copy that single file to another PC to view it offline;Python,WSL and ns-3 are
not needed for viewing. The generated file and raw results are local artifacts,
not guaranteed to be present in a GitHub clone. The published older release is
the wired demonstration;it does not contain these wireless results.

The top cards and detailed tasks refer to seed1. The comparison table shows all
three seeds. Choose a zone and phase,inspect a device,then filter/select tasks to
see their outcomes and event times. The topology is a connectivity schematic.
On narrow windows,scroll inside the topology/table panels. Download run data
exports the embedded JSON. A browser may ask where to save it.

## Use this PC's existing simulator

1. Open Ubuntu24.04 in WSL as the configured build user. In this workspace the
   Linux user is `warehousebuild`;the dependency paths below belong to that user.
2. Change to the repository directory:

```sh
cd /mnt/c/Users/ermec/Downloads/Routing-WIPRO-20261029T174553Z-1-001/Routing-WIPRO
```

3. Run the small control first:

```sh
python3 scripts/run_access_authorization.py configs/access_ecmp_control.json --outcome-measurement --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

4. For the validated500-device workload:

```sh
python3 scripts/run_access_authorization.py configs/access_500_device_probe.json --probe-500 --outcome-measurement --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

Run one build/simulation workflow at a time. Keep the computer awake. Each500
native run has a3600s wall limit;the command runs both original and replay.
It prints a new result directory. Read its `status.json`:MEASURED with validation
PASS means accounting/replay passed. Also inspect `outcomes`,allTasksDelivered
and allDeadlinesMet;measurement validity does not imply zero losses/misses.
FAILED/INTERRUPTED means inspect logs;never relabel it successful.

## Export your result

From the repository,use Python3 on Linux or Python on Windows. Replace the result
directory with the one printed by your run and choose a new output filename:

```sh
python3 scripts/visualize_access.py results/YOUR_RESULT output/my-wireless-run.html
```

For comparison,append `--compare results/SECOND_RESULT --compare results/THIRD_RESULT`.
Configurations must match except radio seed/run;every case is independently
re-audited. Duplicate seeds or mismatched inputs reject. Existing output files
are preserved;choose another name. Then open the generated HTML directly.

## Configuration and other machines

Copy a configuration before changing it. Explicit zone device counts are under
`inventory.zones`;tasks reference robot IDs and must be updated consistently
when counts change. `radioRandomness.seed` changes the experiment seed.
`authorization.requiredTaskTypes` determines approval requirements.
`arp.waitReplyTimeoutMs` is explicit in the500preset(50ms). Default omission is
1000ms. See [study results](access-500-device-results.md) for measured scope.

The radio runner requires an existing configured Linux dependency tree:
ns-3 revision `ea50b72ab79b1ec5912c66d5b384effa829f18bc`,NR revision
`f29ebd33450c49af934ea5dde8606f855c22c2a6`,RelWithDebInfo build and the reviewed
test-only patch applied by `scripts/apply_radio_test_patch.py`. It verifies these
before execution. See [integration history](wireless-integration.md).

The new [wireless installation workflow](wireless-installation.md) provides a
pinned installer. A separate local-mirror build, upstream tests and small
warehouse replay passed; clean OS/internet-clone acceptance remains pending.
The existing `setup_ns3.py` instructions primarily support the older wired build;
they are not a replacement for the pinned NR/Wi-Fi environment. Consequently,
viewing an exported HTML is portable now;fresh-PC simulation setup remains a
packaging task. Do not claim a clone alone installs the complete wireless stack.
