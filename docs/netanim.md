# NetAnim visualization

`NetAnimVisualization` owns an ns-3 `AnimationInterface` and enables packet metadata for any packet transmission occurring on the installed ns-3 devices. It labels the private gNB and each robot, and maps robot state to colours:

- Green — `Idle` or `Working`
- Yellow — `LowBattery`
- Red — `ReturningToCharge`

When energy logic transitions a robot to `ReturningToCharge`, call `updateRobot()` and `animateReturnToCharge()` with the configured charging-station position. The latter schedules visible mobility updates at the supplied interval; callers should ensure the robot is no longer driven by a competing mobility policy.
