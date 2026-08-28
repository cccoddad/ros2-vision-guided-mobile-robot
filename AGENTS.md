# ROS2 C++ Language Policy

This project is a C++-first ROS2 and embedded robotics project. Follow these rules for every implementation change unless the user explicitly requests an exception.

## Required language boundaries

- STM32 firmware, motor PID, encoder processing, safety watchdogs, and the MCU protocol implementation: C. C++ is allowed only when it does not compromise deterministic behavior or the existing firmware conventions.
- Physical-robot ROS2 nodes: C++ using `ament_cmake` and `rclcpp`.
- The following production packages must be C++: `base_driver`, `parking_controller`, future `ros2_control` hardware interfaces, diagnostics, task management, and real sensor-control integration.
- Python is limited to launch files, Gazebo/SIL mocks, offline rosbag/data analysis, plotting, test fixtures, one-off calibration tools, and short developer utilities.
- URDF/Xacro, SDF, YAML, XML, CMake, and Markdown remain the appropriate declarative/build/documentation formats.

## Current Python packages

`base_driver_mock`, `parking_controller`, and `robot_simulation` are current SIL/prototype packages. Do not extend their Python runtime logic into the physical robot path. Keep them working as reference/mocks until a corresponding C++ package is implemented and tested.

When replacing a prototype:

1. Preserve public ROS interfaces: topic names/types, action definitions, TF frame names, parameter names, and failure semantics.
2. Create the production package with `ament_cmake`, `rclcpp`, standard install rules, launch/config directories, and C++ tests where practical.
3. Keep Python only for launch orchestration, test data, plotting, or mock behavior.
4. Update README and package documentation with the migration status and how to select simulation versus physical hardware.

## Engineering expectations

- Use C++17 or the ROS2 Jazzy-supported C++ standard configured by the workspace.
- Prefer `rclcpp` lifecycle/action/diagnostic APIs and typed messages over shelling out or ad hoc scripts.
- Never place serial-port access, motor-control logic, safety decisions, or real-time control loops in a Python production node.
- The STM32 remains the final authority for emergency stop, communication timeout, PWM disable, and hardware fault handling.
- Do not add Python merely because an existing mock package is Python. New production ROS2 code belongs in C++.

## Validation

- Run the relevant `colcon build` and package tests after C++ changes.
- For interface migrations, run the SIL/Gazebo path and verify topics, actions, TF, command timeout, and fault-stop behavior remain compatible.
- Report any intentional policy exception in the change summary.
