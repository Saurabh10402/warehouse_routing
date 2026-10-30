# NetAnim visualization

This adapter and return-to-charge animation belong to the optional **legacy** ns-3 workflow.
The stationary primary core does not call `animateReturnToCharge` or generate NetAnim XML.
It exports fixed coordinates in nodes.csv and validated PNG comparison charts instead.

`NetAnimVisualization` owns an ns-3 `AnimationInterface` and enables packet metadata for any packet transmission occurring on the installed ns-3 devices. It labels the private gNB and each robot, and maps robot state to colours:

- Green — `Idle` or `Working`
- Yellow — `LowBattery`
- Red — `ReturningToCharge`

When energy logic transitions a robot to `ReturningToCharge`, call `updateRobot()` and `animateReturnToCharge()` with the configured charging-station position. The latter schedules visible mobility updates at the supplied interval; callers should ensure the robot is no longer driven by a competing mobility policy.
