# Language and Runtime Boundary

## Purpose

The project demonstrates C++ Linux/ROS2 engineering and STM32 embedded control, not a Python-only robotics demo. This document sets the language boundary so the repository remains credible for embedded robotics and robot software roles.

## Architecture mapping

| Layer | Language | Responsibility |
|---|---|---|
| STM32 lower controller | C | Encoder sampling, wheel-speed PID, PWM, communication parser, watchdog, emergency stop, battery/fault checks |
| ROS2 production nodes | C++ / rclcpp | Serial/CAN hardware driver, odometry, TF, AprilTag parking Action, state machine, diagnostics, task manager |
| ROS2 simulation and tooling | Python when useful | Gazebo/SIL mocks, launch orchestration, test fixtures, calibration helpers, rosbag analysis, result plots |
| Robot description/configuration | URDF/Xacro, SDF, YAML, XML | Geometry, transforms, launch parameters, simulation and hardware variants |

## Production C++ packages

The planned C++ packages are:

- `base_driver`: physical serial/CAN transport, binary protocol, `/cmd_vel`, `/odom`, `/base_status`, and command timeout reporting.
- `parking_controller`: AprilTag TF consumption, `ParkToTag` Action server, finite-state machine, velocity limits, target loss handling, and result metrics.
- `robot_hardware` (V2.5): a `ros2_control` SystemInterface that retains the STM32 protocol while exposing wheel command/state interfaces.
- `robot_diagnostics` and `task_manager`: health aggregation, fault handling, rosbag/test metadata, and later Nav2/behavior-tree integration.

## Current migration state

The existing Python packages are intentionally retained as software-in-the-loop references:

- `base_driver_mock`: mock base feedback and timeout behavior.
- `parking_controller`: C++ `ParkToTag` Action runtime; retained Python source is a migration reference and is not installed as a runtime node.
- `robot_simulation`: Gazebo bridging and simulated Tag pose helper.

They may be improved only for simulation, testing, or migration support. Real UART/CAN access, real motor control, physical safety behavior, and final parking logic must move to C++ before hardware delivery.

## Migration contract

The C++ replacement must preserve these contracts unless versioned deliberately:

- Topic names and message types.
- `ParkToTag.action`, `BaseStatus.msg`, and `TagPose.msg` semantics.
- TF names: at minimum `odom`, `base_link`, `camera_link`, and `camera_optical_frame`.
- Parameter names and default safety limits.
- Fault precedence: STM32 safety fault > hardware driver > parking task state.

## Permitted Python examples

- `*.launch.py` launch files.
- Calibration capture/check scripts.
- Parsing rosbag2 exports and generating CSV/plots.
- Gazebo mock nodes and deterministic integration-test fixtures.
- Build, formatting, and developer helper scripts.

## Prohibited Python in the physical robot path

- Serial or CAN control of the physical STM32 base.
- Wheel PID, command watchdog, emergency-stop decision, or PWM logic.
- Production parking state machine or velocity controller.
- Any final safety-critical runtime decision.

## Review checklist

Before merging a new module, ask:

1. Does it run on the physical robot and affect perception, motion, safety, or task execution? If yes, implement it in C++ unless it is purely declarative configuration.
2. Is it a simulation, analysis, launch, or temporary calibration tool? Python is acceptable.
3. Does the change preserve the ROS interface and STM32 safety boundary?
4. Does the README explain whether the package is mock, simulation, or production hardware code?
