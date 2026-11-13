# Wireless integration dependency build in progress

User explicitly restored primary NR/5G-LENA and Wi-Fi access requirements with
stationary robots. Powered fabric and legacy wired results remain separate.
Official ns-3.46 and NR v4.1.1 downloaded, pinned revisions documented in
docs/wireless-integration.md. Eigen 3.4.0 downloaded. Isolated CMake configuration
passed: NR, Wi-Fi, Eigen, examples, tests enabled. Optional SQLite calibration
examples unavailable. Two-worker warnings-as-errors build is running; last
observed progress 346/1588, no compiler failure yet. Not a completed build.

Build log: .recovery/radio-build.log. Configuration log:
.recovery/radio-configure.log. Active process session in this chat: 73842.
Local helpers: .recovery/configure_radio.py, build_radio.py, check_nr_example.py.
The example checker has only passed syntax compilation; it has not executed.
No radio measurements or warehouse wireless completion claim.

Checkpoint026 source comparison differs only by the new wireless design document;
this is an intentional addition, not corruption. Previous manifests retained.
Next: inspect build completion/errors, resolve actual failures, execute upstream
tests and the official NR example; then implement uplink and Wi-Fi controls.

## Build failure and diagnostic continuation

Release compilation exited 1 at step 503/1588 in Wi-Fi mgt-headers.cc. GCC 14.2
reports maybe-uninitialized inside the inlined optional NonInheritance/set
destructor. No warning suppression or upstream source modification applied.
Whether this is a false positive remains unestablished. Separate debug build
started with assertions, logging and warnings-as-errors; session 69324, log
.recovery/radio-debug.log, helper .recovery/debug_radio.py. Original release
outputs retained. No packet tests run. Diagnostic build success must not be
reported as resolution of the release failure.

## Windows linker diagnosis

Debug failed at step 652 linking Wi-Fi with export ordinal too large: 68031.
Upstream ns-3 issue 1225 describes this Windows DLL export limit for Wi-Fi/NR.
The local ns-3 macros explicitly reject static Windows builds. WSL status/list
check launched in session 37430; result pending. Linux toolchain setup is the
next candidate supported route, not a radio abstraction. No warning suppression,
model deletion or dependency source patch has been applied.

## WSL repair

WSL launcher status and version failed REGDB_E_CLASSNOTREG. Existing Store
package 2.4.11 registration refresh succeeded but version still failed. Windows
Installer COM activation succeeds. Downloaded official Microsoft WSL 3.0.3 x64
MSI, SHA256 ae977286974255bd26bf925f00f40f5692c6b2084155cbc0563c09aea091f211,
Authenticode Valid Microsoft Corporation. Installation launched with /passive
/norestart and verbose log .recovery/wsl-msi-install.log, session 8062.
Installation outcome pending; do not claim WSL repaired or reboot automatically.

WSL MSI completed exit 0; wsl --version verified 3.0.3 and kernel 6.18.54.1-1.
No Linux distributions installed. Enabled VirtualMachinePlatform using DISM
/NoRestart; exit 3010 and log explicitly confirms reboot required. HypervisorPresent
True on TIMI Mi NoteBook 14. Await user-managed Windows restart, then verify WSL2
and install Ubuntu 24.04. Logs retained; no automatic restart issued.

After restart, WSL status no longer reports WSL2 virtualization unavailable.
Ubuntu 24.04 install launched with --no-launch, session 12222, outcome pending.
Prepared Linux build helper .recovery/linux-radio-build.sh with exact source
revision checks, radio modules/tests/examples, assertions and warnings-as-errors.
Helper not executed; Ubuntu startup and package installation remain pending.

Ubuntu installation completed exit 0. Verified /etc/os-release Ubuntu 24.04.5
and kernel 6.18.54.1-microsoft-standard-WSL2. Linux dependency apt setup running
in session 1879. Radio compilation and tests not yet started on Linux.

Linux apt setup completed exit 0. Created unprivileged warehousebuild account
(no password assigned). Started .recovery/linux-radio-build.sh in session 44086.
Sources matched pinned revisions; configure detects GCC 13.3, Linux Git/Python.
Work directory /home/warehousebuild/warehouse-radio; build.log retained there.
Helper now sets Linux-only PATH for subsequent invocations. Initial configure
warned inherited Windows PATH but selected /usr/bin compiler/Git/Python.
Build completion and upstream tests not yet verified.

Configuration passed with SQLite and Eigen ON. Initial shell stopped after
configuration because helper was edited during execution, shifting its read
position. Re-ran finalized helper, successful configure and compile started in
session 97650 (1609 steps, NR object files compiling). No source model changes.

Linux compilation completed 1609/1609. Official cttc-nr-demo two-UE downlink
and replay PASS, results/nr-dependency-20261103T221554Z-dcb74a7a. Received
5998/6000 and 5990/6000; means 0.276044 and 0.900967 ms. Unreceived counts
retained; aggregate FlowMonitor smoke only, no warehouse/uplink claim.
Upstream test.py QUICK tests/examples launched session 40283 with two workers,
retained temporary data, text/XML/log under /home/warehousebuild/warehouse-radio.
Outcome pending. Initial Linux executable discovery corrected to use executable
permission because ns3.46 in the filename is not a file extension boundary.

Stationary Wi-Fi smoke compiled as Linux scratch target. One 802.11ax STA,
one AP, 5 GHz 20 MHz HeMcs0, fixed positions, AP-to-server 1 Gbps/0.1 ms.
UdpClient sends 10 x 256 bytes after 1 s association warm-up; receiver reports
10 packets, Tx counter 2560 bytes. Two runs exit 0 and logs match exactly.
Source .recovery/warehouse-wifi-smoke.cc; logs under Linux warehouse-radio.
This is a small access control, not the full warehouse. NR uplink remains next.
Upstream suites still active at last check (NR CSI/RI-PMI CPU-intensive suites).

Derived NR uplink control retains original demo channel/core and changes sinks
to external host, senders to UE, TFT to uplink remote destination ports. Ten
256-byte packets per UE, 20 ms spacing, original 1 s simulation. Build passed;
execution generated 10 per flow but received zero, checker correctly FAILED.
Failed results retained. UE default route is installed by AssignUeIpv4Address
(confirmed helper source), so do not blindly add duplicate routes. Next enable
radio/core traces and diagnose; full warehouse integration remains pending.

Uplink diagnosis: MAC/SGW/PGW trace confirmed packets cross radio into PGW.
Pinned SetupRemoteHost returns internetIpIfaces.GetAddress(0), which belongs to
PGW (Install(pgw, remoteHost)); derived control mistakenly targeted that address.
Corrected control resolves remoteHost IPv4 interface 1 address. Rebuild passed;
results/nr-dependency-20261103T222632Z-c4b5d0fe PASS, 10/10 each flow and identical
replay. No upstream source patch or radio parameter retuning. Diagnostic log
/home/warehousebuild/warehouse-radio/uplink-diagnosis/diagnosis.log retained.

Mixed-access control prepared: existing NR UE/core plus static 802.11ax station/AP,
AP routed point-to-point attachment to external-host/fabric router, explicit PGW
return route to Wi-Fi subnet. Adds UE-to-Wi-Fi and Wi-Fi-to-UE flows to two NR
server flows; ten packets each. Source .recovery/warehouse-mixed-access.cc and
generator prepare_mixed_access.py; build session 37089. Validation helper mixed
mode requires four flows and 10/10 each with identical replay. Execution pending.

Mixed build and execution completed: results/nr-dependency-20261103T222938Z-6a10d7eb
PASS, four flows 10/10 each, exact aggregate replay. Small combined access
topology only; not yet maintained JSON runner, eight zones or audited hop paths.

Upstream suite finished exit 1: 725 passed, 2 intentionally skipped, 3 failed
(wifi-gcr, wifi-mlo, wifi-txop), 1 crashed (wifi-emlsr-basic-exchanges).
Final upstream-quick.log/.txt/.xml retained under Linux warehouse-radio.
No test runner active. Diagnose individual cases; no overall PASS claim.

Resumed diagnosis: targeted suites via .recovery/diagnose_wifi.sh reproduce
wifi-gcr/wifi-mlo failures, wifi-txop/emlsr segmentation faults without the full
runner's stop-on-failure. Logs at Linux warehouse-radio/wifi-diagnosis. GDB
installation started session 30740. ns-3.46.1 fetched for inspection only;
checkout unchanged; Wi-Fi delta removes unused constants, not a relevant fix.

## Wi-Fi isolation completed (4 November)
GDB is installed. emlsr-backtrace.log locates SIGSEGV in
EmlsrDlTxopTest::CheckResults, wifi-emlsr-basic-exchanges-test.cc:901,
where the test dereferences an expected second frame exchange. This identifies
the crash location, not why the exchange is absent.

.recovery/isolate_wifi.sh builds a separate diagnostic runner using existing
Wi-Fi test/core libraries. ldd confirms neither NR nor LTE is loaded. All four
failures reproduce: wifi-gcr/wifi-mlo/wifi-txop exit 1; emlsr exit 139.
Logs and library inventory: /home/warehousebuild/warehouse-radio/wifi-diagnosis/isolated.
Thus NR library initialization is not required to reproduce these failures.
No dependency model edits, warning suppression or test expectation changes.
Next: inspect Wi-Fi frame/queue behavior around the first failed exchange;
compare against upstream fixes or a controlled build before selecting a fix.
No diagnostic process remains running. Warehouse wireless integration pending.

## Confirmed MLO test loss-injection defect (4 November)
Debugger trace wifi-diagnosis/uid-trace.log: ListErrorModel checked received
packet UID 123229 against configured drop UID 123305. The failing MLO test uses
psdu->GetPacket()->GetUid(), which can identify a newly assembled aggregate,
while PHY post-reception error checking uses the contained MPDU packet.
An isolated diagnostic copy changes this one injection to
(*psdu->begin())->GetPacket()->GetUid(). Whole wifi-mlo QUICK suite then PASS,
exit 0, including the ADDBA Response timeout case. Expectations unchanged.
Helper: .recovery/wifi_uid_control.py. Linux evidence:
/home/warehousebuild/warehouse-radio/wifi-diagnosis/uid-control/{build.log,run.log,status.txt}.
Original dependency source/binaries untouched; original full suite still failed.
wifi-txop and emlsr contain similar aggregate-UID injections but have NOT yet
been validated with a correction. wifi-gcr cause remains undiagnosed.
Next: isolated test-only controls for txop/emlsr; then diagnose gcr, package
reviewable dependency test patches and rerun validation. No background run active.

## All four Wi-Fi diagnostic suites pass (4 November)
Isolated test-only corrections now pass wifi-mlo, wifi-txop,
wifi-emlsr-basic-exchanges and wifi-gcr QUICK suites, exit 0 each.
MLO/TXOP/EMLSR loss injection now targets the contained MPDU packet rather
than a newly assembled PSDU. GCR tracks the original contained MSDU when
concealment wraps it as A-MSDU, avoiding counting a retry as a new message.
Assertions and simulation model code unchanged. Original dependency untouched.
Exact four-file patch, logs and SHA256 manifest copied to Windows:
.recovery/wifi-test-diagnosis/. Linux diagnostic folders: uid-control,
uid-extended, gcr-control under warehouse-radio/wifi-diagnosis.
Helpers: wifi_uid_extended.py, wifi_gcr_control.py, save_wifi_diagnosis.py.
Next: review/apply the test-only patch in a reproducible dependency workflow,
then rerun combined upstream validation before warehouse integration.
Original full-suite outcome remains 725 pass, 2 skip, 3 fail, 1 crash;
isolated corrected passes do not replace that historical outcome.
No background process remains active.
