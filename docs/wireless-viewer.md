# Offline wireless results viewer

Export a completed audited outcome-measurement result:

```sh
python scripts/visualize_access.py results/access-auth-20261114T153317Z-b9c69ff0 output/wireless-seed1.html
```

Use your own result directory on a different machine. The command reruns outcome,
coverage,ECMP and seed audits and refuses a mismatched saved audit or existing
output file. Open the generated HTML in Chrome or Edge; no server or internet is
needed. The generated file can be copied to another PC without the source results.

Choose a zone and outage phase,filter tasks by outcome or approval requirement,
and select a task to inspect its source,destination,delay and recorded events.
Download displayed run data exports the embedded JSON. All offered tasks remain
in the summary denominator. The diagram shows schematic configured connectivity,
not a measured full radio/GTP path. Radio endpoints are grouped by access type;
individual task identities remain available in the table. Per-device inspection,
seed comparison and measured fabric-hop exploration are future viewer additions.

The first500-device seed1 export contains1506tasks,470robots and500total devices.
Offline Chrome checks passed for rendering,zone/phase/outcome filters and event
selection.21core checks passed. Download interaction and visual screenshot review
are not yet validated. The older wired viewer remains separate.

## Device inspection and three-seed comparison

Use repeated `--compare RESULT_DIRECTORY` options to add independently audited
summary rows. All inputs must match except radio seed/run; duplicate runs reject.
For example,append the seed2 and seed3 result directories from
[validated results](access-500-device-results.md) to the export command.
The new local export is `output/wireless-three-seeds-reviewed.html`.
Detailed task/device panels use the primary result;comparison rows use all runs.
Select a recorded device to see its attachment,position and task counts. Related
task search finds tasks involving that device,subject to current filters.
DeviceCSV omits EPC/server rows;the500total includes them. Three-seed comparison
and device inspection supersede their earlier pending status above. Generated
downloadJSON was tested in-browser;actual filesystem saving remains untested.
