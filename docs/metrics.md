# Metrics and FlowMonitor

`MetricsCollector` owns experiment-level counters and values independent of ns-3. It records packet/byte totals, end-to-end delay, battery observations, charging events, routing removals, and route recomputations. `summarize()` calculates average delay, throughput, packet delivery ratio, average/minimum battery, and network lifetime. Network lifetime is the first robot-removal time, or the configured simulation duration if no robot is removed.

`writeCsv()` produces a stable `metric,value` CSV file and `writeSummary()` creates the human-readable simulation report.

When ns-3 is enabled, `Ns3FlowMonitorCollector` calls `CheckForLostPackets()` and aggregates every FlowMonitor flow's transmit, receive, loss, byte, and delay counters into a `MetricsCollector`. Traffic scenarios should invoke it after `Simulator::Run()` and before `Simulator::Destroy()`.
