# Test status — checkpoint_006

| Check | Result | Evidence |
|---|---|---|
| Bootstrap core CTest | PASS 19/19 | evidence/checkpoint006-bootstrap-tests.txt |
| Default core-only CTest | PASS 19/19 | evidence/checkpoint006-default-tests.txt |
| Protected full study | PASS 144 runs, 3 seeds | evidence/checkpoint006-protected-status.json |
| Preserved baseline regression | PASS 132 runs, matching inputs and byte-identical packet traces | evidence/checkpoint006-baseline-status.json |
| Queue admission/occupancy conservation | PASS all runs | Per-direction/class queue_stats.csv; runner validation |
| Protected deterministic mechanisms | PASS | Reservation blocking; token refill/idle/full-duplex; byte fairness; overload progress; failure accounting |
| Invalid inputs / real CLI | PASS | Protected JSON, bad rates/weights, unsupported combinations and queue exports |
| Replay | PASS both batches | Byte-identical selected traces |
| Plots | PASS 40 protected / 55 baseline | PNGs; protected overload scope/class chart visually inspected |
| ns-3/NR runtime | BLOCKED / NOT RUN | Prior external Wi-Fi compiler failure unchanged |
| Mac/Linux execution | NOT RUN | Current host is Windows |

Executed scripts/verify.py; scripts/run_all_experiments.py --suite protected-qos;
scripts/run_all_experiments.py --suite warehouse --preset default --baseline
results/experiments-20261030T140941Z-dd164497. No tests or warnings were disabled.
The protected policy is not universally better: the fixed P0 admission ceiling worsens
emergency-overload results. Experimental failure counts are retained, not test failures
or an excuse to adjust denominators.
