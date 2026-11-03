# Fresh Windows installation audit (in progress)

Test root: C:\Users\ermec\WarehouseTest-20261103
Source: public main ZIP at f9649a86c11bdbc76668471cb30996e2a8d6c4dc; SHA256 554819733c8be74e43a83081e09c98b3ab8f894fd48271c5ebde517668137beb.
Environment: normal user/machine PATH only for build commands; no inherited coding-environment compiler/Python/Git. Fresh compiler, Boost and ns-3 archives downloaded; no existing project build/tree copied. pip may reuse its normal download cache.

- Official signed Python 3.12.10 MSI installation failed with 0x80070003/2203 opening cached core.msi. Logs retained at test root. No standard launcher installation success is claimed.
- Official NuGet Python 3.12.10 extracted to sibling python312; created source .venv; pip works. SHA256 0eb85c2dfccccf1b17352de4c397f69194035b7d37149eacc16f1147d93de3b8.
- README portable setup --plots PASS: pinned GCC14.2.0 and Boost1.85 checksums; plotting installed.
- README first_run.py PASS:21/21 tests;180/180 delivered; zero misses. Batch first-run-20261103T100022Z-b798f5b1.
- Demo downloaded separately, SHA256052f2eebbc78a2aaf2be1fc4da7946dba4726eeeaeaeddaafe37a5eb8e57ed9f. Fresh extraction integrity PASS4424 hashes/6 links/pages.
- Browser tested menu plus all5 demo pages through documented loopback HTTP option. In-app file URLs unsupported. Baseline120/120, allrobots2410/2410, comparison4 cases, zone packet2 no_route then4 recovery and B297 delivered during A outage; spine packets6/7/9 reroute/recovery. Next-hop/show-full/toggle work. No console errors observed.
- Native ns-3 setup building; completion not yet established.
- Native smoke/new layout/allrobot/failures/comparison/export pending.

README edit in original repo documents exact fallback, explicit Python release/launcher, optional Git ZIP route and removes stale report-pending sentence. No simulator code changes. Do not publish completion claims until native steps finish. Preserve failed installer logs and all raw batches. Next: inspect ns3-setup.log and session18509, then smoke and demos using fresh .venv and normal user PATH.

## Update after full core validation

Full core batch `experiments-20261103T100328Z-2e24473e` PASS132 cases, all artifact checks, replay and plots. Separate post-run comparison:133 packet CSVs identical to `experiments-20261102T120006Z-a802bc3c`; receipt `core-regression.json`.

First native setup attempt stopped at mobility compilation because GCC could not launch `as.exe` (CreateProcess no such file). The actual file existed and `--version` succeeded afterward. Failure log retained as `ns3-setup-first-attempt.log`. Root cause unestablished. Retrying the README's `setup_ns3.py --jobs 2` after pinned-source verification; log at test root `ns3-setup-retry.log`, active session70785. Retry passed that compilation step and is building upstream tests. Native setup not yet PASS.

Main README and draft docs/windows-installation-check.md updated, uncommitted. Helper command sequence for smoke/four layouts/exports/comparison is in `.recovery/readme-native-check.ps1`; do not start until setup receipt says PASS. Current public source itself remains unmodified in fresh test folder. Demo HTTP server session47769 on127.0.0.1:8000 serves only demo folder. Browser tab8 had previously checked all5 pages; later screenshot attempt returned browser.user unavailable, so reacquire only if needed. Preserve original pending failed setup log and original source archive. Core sessions completed; original native setup session18509 failed, retry70785 is active.
