# Fresh Windows installation check

This check uses the public GitHub source ZIP at commit `f9649a86c11bdbc76668471cb30996e2a8d6c4dc`, extracted separately from the development checkout. No project toolchain, build tree or simulator binary was copied from the existing workspace. Build commands use the normal Windows user/machine PATH, without the coding application's bundled Python or Git. This is a fresh project installation on the same Windows PC, not a pristine virtual machine or a macOS/Linux test.

The normal signed Python 3.12.10 installer failed with Windows Installer `0x80070003`; the core MSI log reports 2203 while opening the cached package. The underlying Windows Installer cause is not established. The README now documents the tested official Python NuGet fallback. That interpreter created a new `.venv`, and the pinned compiler/Boost downloads and plotting setup passed. Keep its sibling interpreter folder because the virtual environment depends on it. Normal pip download-cache reuse is possible; no old build artifacts were substituted.

The separate public demonstration ZIP matched its published checksum and passed 4,424 file hashes and 6 local link/page checks. All five demonstrations were inspected through the README's loopback HTTP option. In-app browser file URLs were unsupported, so this check does not claim new direct-file browser validation. Zone failure/recovery and unaffected traffic matched the README packet examples; spine routing changed with availability. Baseline/all-robot counts and four routing comparison cases displayed correctly. Hop controls and robot visibility worked; no console errors were observed.

The independent core passed 21 tests, the 180/180 first-run fixture, and the full 132-case three-seed study with plots and replay. A subsequent comparison found 133 packet CSV files (132 cases plus replay) byte-identical to the preserved earlier full run. This is distinct from native ns-3 validation.

Native installation and fresh native demonstration results will be recorded after completion; see checkpoint 026 evidence. The published report remains the dated checkpoint 024 evidence snapshot, and the demonstration ZIP remains checkpoint 022. This maintained README contains later installation corrections.

## Final native results

The resumed native smoke test passed 25/25 project tests and exact replay: 3/3 UDP packets, 4.416 ms mean delay. The freshly built ns-3 dependency passed all 150 upstream UNIT QUICK suites with warnings treated as errors.

| Fresh case | Offered | Delivered | No-route losses | Result |
|---|---:|---:|---:|---|
| Representative baseline | 120 | 120 | 0 | PASS and exact replay |
| All 482 robot sources | 2,410 | 2,410 | 0 | PASS and exact replay |
| Spine outage/recovery | 120 | 120 | 0 | PASS and exact replay |
| Zone A switch outage/recovery | 2,410 | 2,252 | 158 | PASS and exact replay; losses expected |

All four fresh viewer exports passed their independent audits. The routing comparison passed one Dijkstra and three ECMP cases with identical offered traffic, 2,410/2,410 delivery and exact replay in each case. Observed cross-zone packets through spine 0/1: Dijkstra 805/0; ECMP seeds 1–3 respectively 435/370, 390/415 and 375/430. The comparison HTML was generated. The native checks are finite, synchronized wired examples, not deployment or radio performance claims.

Fresh batch identities and statuses: `checkpoints/evidence/checkpoint_026/native-results.json`. Logs from each README step, the failed initial setup and successful upstream installation are retained in the same evidence folder. Original raw result directories remain in the separate Windows test workspace and should not be deleted. The new exported HTML pages passed the exporter audit; interactive browser verification of those new pages was unavailable after browser control stopped, although the downloaded demonstration pages were checked earlier.
