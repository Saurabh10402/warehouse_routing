# Stationary wireless warehouse integration

The primary requested architecture uses stationary radio endpoints initially,
with explicit NR or Wi-Fi access per robot. Mobility and handover experiments
are later work. Wired endpoint results remain baseline results, not wireless
measurements. EALR remains legacy.

## Dependency gate

The Ubuntu 24.04 WSL2 build uses official ns-3.46 revision
ea50b72ab79b1ec5912c66d5b384effa829f18bc and 5G-LENA v4.1.1 revision
f29ebd33450c49af934ea5dde8606f855c22c2a6. Eigen 3.4.0 revision
3147391d946bb4b6c68edd901f2add6ac1f31f8c supplies matrix support.
The pairing is specified in the NR release notes. Windows is not among that
release's tested platforms; compilation and execution must be established.
Existing wired dependency trees are retained separately.

## Acceptance sequence

1. Configure and compile NR, Wi-Fi, their dependencies, examples and tests.
2. Run upstream radio tests and an NR UDP example through UE, gNB and EPC
   gateway. Record received packets, offered load and delay. The EPC helper
   is the implemented core model, not a claim of a complete standalone 5G core.
3. Validate Wi-Fi station-to-AP-to-server traffic with actual Wi-Fi devices.
4. Implement explicit per-robot access selection and fixed physical positions.
   Connect access and core gateways to the warehouse fabric with valid IP routes.
5. Verify NR-to-NR, Wi-Fi-to-Wi-Fi, mixed-access peer and edge-service traffic.
   Do not assume cellular same-zone traffic bypasses the core gateway.
6. Measure offered/delivered packets, radio and end-to-end delays, loss and
   deadline misses under declared channel, scheduler and traffic settings.
   Preserve losses in denominators and distinguish independent RNG runs from replay.
7. Scale to the configurable eight-zone example only after small controls pass.
   Export measured paths and update the report after the wireless results exist.

Access assignment is explicit configuration initially. Selecting a radio by a
deadline does not establish that it meets that deadline. Channel calibration,
interference, resource scheduling and offered load affect the outcome.

Historical Windows attempts: dependency configuration passed, but both native Windows builds
failed. Release failed on GCC's maybe-uninitialized diagnostic in Wi-Fi management
headers. Debug compiled that file but failed linking Wi-Fi with "export ordinal
too large: 68031". This matches upstream issue 1225:
https://gitlab.com/nsnam/ns-3-dev/-/issues/1225
The checked-out ns-3 build explicitly rejects NS3_STATIC on Windows, so simply
enabling static linkage is not a supported solution in this version.

## Linux controls and test corrections (4 November 2026)

The Linux build completed with GCC 13.3, Eigen and SQLite. Small NR downlink,
NR uplink, stationary Wi-Fi and mixed NR/Wi-Fi controls passed with repeated
aggregate results. These are dependency controls, not the configurable warehouse.
The full unmodified upstream QUICK run recorded 725 passes, two intentional
skips, three failures and one crash. Preserve this historical result.

Diagnosis isolated the four Wi-Fi failures from NR. Three tests selected a
newly assembled PSDU packet identifier for deliberate loss, while the receiver
checked the contained MPDU identifier. The groupcast test mistook the new
A-MSDU wrapper identifier for a new message during retries. Correcting these
test identifiers made all four isolated QUICK suites pass, including EMLSR
which previously crashed while checking a missing expected exchange.

The reviewed patch is `scripts/patches/ns3-3.46-test-packet-identity.patch`.
It changes only four test files: no simulation models, assertions, expected
counts or test selections. It is a local correction, not an upstream release.
Apply it before building the pinned Linux dependency:

```sh
python3 scripts/apply_radio_test_patch.py /path/to/ns-3
cmake --build /path/to/build -j 2
```

The installer verifies the exact revision, rejects unrelated tracked edits,
and verifies exact patched contents when run again. Retain its printed patch
hash alongside results. Rebuild and run upstream validation after application;
isolated passes alone do not establish a passing combined run.

Raw diagnosis and original failed reports remain in the local recovery/WSL
directories listed in `ACCOUNT_SWITCH_HANDOFF.md`. The combined rerun is a
separate batch; its completion status must be checked before claiming PASS.
Full warehouse radio JSON, routing/path audits, scale evaluation and viewer
integration remain pending. The 5G-LENA EPC helper is not a full 5G standalone
core implementation.
