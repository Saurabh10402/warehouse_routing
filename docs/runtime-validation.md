# Runtime and first-run validation - checkpoint_008

This milestone improves dependency installation, first-run diagnostics and real wired ns-3
prerequisites. It does not replace the independent warehouse simulator with an ns-3 backend.

| Executed check | Result |
|---|---|
| Windows core verification | 20/20 CTest PASS |
| Fresh source copy, no `.git` or old build, path containing spaces | Fresh compile, 20/20 CTest and 180/180 first-run delivery PASS |
| Full warehouse regression | 132 runs, three seeds, exact input/packet-trace baseline comparison PASS |
| Regression replay / charts | PASS / 55 PNGs |
| Required ns-3 with missing dependency or empty explicit prefix despite another valid package | Explicit configuration failure PASS |
| Wired ns-3 prerequisite build and upstream tests | Release build and 150/150 UNIT QUICK suites PASS |
| Project linked ns-3 packet tests and replay | 22/22 CTest PASS; 3/3 UDP packets, 4.416 ms, byte-identical replay |
| Mac M4 / Ubuntu 24.04 ARM64 execution | NOT RUN |
| Schema-v2 ns-3 forwarding/QoS and 5G-LENA NR | NOT IMPLEMENTED / NOT VALIDATED |

The fresh-source check used the same Windows host's installed compiler and an external
verified Boost source tree through `EALR_BOOST_SOURCE`. No compiler or stale build was copied
into the extracted source. The environment still supplied Python and the compiler: this is
evidence of source/build-path independence, not a clean Mac/Linux machine installation.

An initial extracted-source run exposed enclosing-repository Git provenance. `git_commit()`
now accepts a commit only when the repository root is this source root; archive execution
records `unavailable (source archive)` and still hashes every source file. A regression test
covers this. The overnight interrupted corrected run recorded two long timeouts; the retry
passed all 20 tests without increasing timeouts or changing tested implementation. Both logs
remain in `checkpoints/evidence/clean-source-first-run-*.txt`.

## Preserved result batches

- `results/ns3-smoke-20261031T102542Z-fd5d9232`: actual two-hop IPv4/UDP measurements,
  replay, source/executable/library hashes and successful dependency setup receipt.
- `results/first-run-20261031T101929Z-5481bba4`: corrected extracted-source first run, copied
  unchanged into the main results folder for recovery packaging.
- `results/experiments-20261031T094745Z-723bc3c8`: full warehouse regression against
  `experiments-20261030T180019Z-2441a69a`; all 132 inputs and packet traces match exactly.

Implementation commit: `a2bf99f` (strict ns-3 verification, optional NetAnim separation,
pinned prerequisite setup, packet fixtures, portable first-run workflow and archive provenance).
Installer corrections are committed as `39ff2af` and `47088f1`; final extracted-code hashes match `47088f1`.
The linked packet build/runtime was tested from `47088f1`; the 132-run core regression
preceded the installer-only fixes, from `a2bf99f`, with no simulator changes between them.
Subsequent documentation changes are recorded in the final checkpoint manifest. Each batch
keeps its own exact source hashes; do not replace those with the later documentation hash.

The first bulk upstream test invocation was rejected by ns-3 (one suite per process is
required). After fixing that, all 150 suites passed, but noisy subcase PASS messages exposed
an overly broad installer parser. It now matches complete suite summaries, with regression
coverage. A full final setup rerun passed all 150 suites and installed successfully. The slow
threaded suite passed in 274.530 seconds initially and 178.567 seconds in the final run; no
test was skipped or timeout increased. Diagnostic attempts and original logs are preserved.

## Interpretation and next gate

Core regressions preserve the previous controlled comparisons, including adverse congestion
results. A successful artifact check does not mean every stress packet met its deadline.
The first-run example's zero misses and the packet fixture's analytical delay are bounded
fixture observations, not deployment guarantees.

The [first-run guide](first-run.md) supplies the intended Mac/Ubuntu workflow and expected
outputs. The [ns-3 integration notes](ns3-integration.md) separate wired prerequisites from
the unresolved Wi-Fi/NetAnim failure and absent NR dependency. The next implementation is
the [staged schema-v2 warehouse contract](ns3-warehouse-contract.md), followed by actual
target-platform and radio validation. The whole requested product is not yet complete.
