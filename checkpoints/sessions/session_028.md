# checkpoint_032 — Seven missing packets accounted for by ARP drops

Added native ARP Drop tracing in wireless-fabric.h, preserving radio settings,
workload, queue defaults and failure status. Rebuild PASS. Default diagnostic
reproduces 153/160 with exactly seven drops at zone-A-ap-0, UDP9003 sequence
3..9 (1.26837..1.38837s), source7.0.0.3 destination192.168.0.2. ARP log shows
reply only around2.20916s following retry; pending queue default3. Exact loss
location/cause established; why initial ARP exchange failed remains unestablished.
Separate command-line --ns3::ArpCache::PendingQueueSize=10 diagnostic returns0,
160/160 delivered. NOT adopted as default: delayed packets may miss deadlines.
No parameter suppression, warm-up exclusion or all-delivered baseline claim.

Logs copied to .recovery/fabric-drop.log, fabric-arp.log and
fabric-arp-capacity10.log; Linux originals at warehouse-radio root. Audits retained.
Project verification21/21 PASS. Source manifest checkpoint_032.
No background process remains. No Git upload.
Next: add packet TX/RX timing and MAC/PHY/ARP exchange traces, diagnose initial
ARP failure and assess cold-start vs explicitly defined operational conditions.
Then independent path audit, replay, and scaling. No full wireless completion claim.
