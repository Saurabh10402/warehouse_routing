> Update, 13 November 2026: the 300-device measurement and full diagnostic
> replay are complete. See [300-device diagnosis](access-300-diagnosis.md).
> Earlier failures and incomplete diagnostics below remain historical evidence.

# Outcome measurement for stationary wireless controls

`--outcome-measurement` is an explicit alternative to strict delivery control.
Real wireless experiments can contain late approvals, missing payloads and missed
deadlines. Previously, native exit2 stopped the runner before replay and auditing.
This mode accepts native exits0 or2 provisionally, runs the same case again, and
requires identical traces/device inventory plus consistent task accounting.
Crashes, other exit codes, execution timeouts and audit failures still fail the run.
The default strict mode and special boundary-loss control are unchanged.

```sh
python3 scripts/run_access_authorization.py configs/access_ecmp_control.json --outcome-measurement --ns3-source /home/warehousebuild/warehouse-radio/ns-3 --build /home/warehousebuild/warehouse-radio/build
```

Add `--scale-control` for an explicitly supported larger configuration. Existing
inventory/task/runtime guards still apply. `--boundary-control` and
`--outcome-measurement` are mutually exclusive. The seed-comparison wrapper has
not been extended to measurement mode; it retains its strict behavior.

## Reading results correctly

Successful accounting produces `status: MEASURED`, `validation: PASS` and a
separate `nativeExit`. MEASURED means the experiment was audited and replayed;
it does not mean that all tasks delivered or all deadlines were met. A successful
runner command can therefore retain nativeExit2 explicitly in status and exit files.

- `allTasksDelivered` and `allDeadlinesMet` answer those specific questions.
- `deliveryControlPassed` mirrors the native configured control contract. It is
  false for unexpected loss in an allow case. A configured deny/silent-server
  control may satisfy its blocking contract while still delivering no protected
  payload; its allTasksDelivered flag remains false.
- Mutually exclusive task outcomes are delivered, authorization denied,
  authorization timeout, accepted payload unreceived by simulation stop, and
  local payload send failure. Their sum must equal all offered tasks.
- Every task remains in the deadline denominator. Denied/blocked/unreceived tasks
  are not counted as deadline-met. This deadline metric is payload completion,
  not correctness of a denial decision.
- Observed IP payload drops are reported separately. A payload unreceived by stop
  is not automatically called an observed radio drop or attributed to a PHY cause.

The auditor checks unique/complete task records, configured start times, native
exit consistency, authorization ordering and processing delay, timeout duration,
ignored late replies, absence of blocked payload, delivery/deadline arithmetic,
server boundaries, scheduled interface outages and restored route counts.
Existing inventory/source-coverage and ECMP path/hash audits run afterward.
No cache warming, radio change, deadline relaxation or authorization bypass is
introduced. Normal payload does not acquire a new server dependency.

## Native validation

Each row below uses real ns-3/NR execution with its own byte-identical replay.
All are small20-node controls. The late-approval case explicitly uses20ms
authorization timeout to exercise the late-reply branch; this is a test input,
not a modification to the300-device study's1000ms timeout.

| Control | Offered | Delivered | Denied | Auth timeout | Payload unreceived | Deadline met | Native exit |
|---|---:|---:|---:|---:|---:|---:|---:|
| Allow | 36 | 36 | 0 | 0 | 0 | 36 | 0 |
| Late approval | 36 | 30 | 0 | 6 | 0 | 30 | 2 |
| Deny | 36 | 30 | 6 | 0 | 0 | 30 | 0 |
| Silent server | 36 | 30 | 0 | 6 | 0 | 30 | 0 |
| Outage boundary | 46 | 43 | 0 | 2 | 1 | 43 | 2 |
| Static direct-zone link | 3 | 3 | 0 | 0 | 0 | 3 | 0 |

Result directories (under results/), in table order:

- `access-auth-20261112T183020Z-023d3e1c`
- `access-auth-20261112T183052Z-18d0b87d`
- `access-auth-20261112T183118Z-7881e4f0`
- `access-auth-20261112T183146Z-60e5adb9`
- `access-auth-20261112T183208Z-a3ab44b3`
- `access-auth-20261112T183538Z-54a842f7`

All six late approvals were ignored without payload release. The outage case
retains one observed payload drop and two authorization timeouts. Its trace and
CSV match the prior strict boundary control exactly; the healthy allow case
matches the earlier corrected ECMP control exactly. The old strict auditors still
accept those respective controls, and the strict delivery auditor rejects the
late-approval fixture. Final auditor rechecks are recorded in the recovery receipt.

Validation:71 access tests and21 core checks pass. Tests include missing/duplicate
records, hidden misses, invalid native success, unapproved payload, invalid late
release/timeout, contradictory drop/delivery and changed replay. A real late-reply
fixture is preserved with provenance; corruption tests are not new radio experiments.

## Remaining work

The300-device original remains FAILED:905/906 delivered,901 deadlines met. Its
ARP diagnostic remains incomplete. None of the six small measurement controls
replaces that result or establishes a complete300-device replay. Next run the
300-device configuration explicitly in outcome-measurement mode with a full replay,
and complete focused ARP/MAC diagnosis using bounded logging. No500-device
validation, multi-seed measurement sweep or visualization/report integration is
claimed here. Mobility, cryptography and full upper network protocols remain out
of scope. A retained timeout or loss is measurement evidence, not an automatic fix.

The static direct-zone-link case exercises the no-outage audit branch and also passes the existing strict authorization auditor.
