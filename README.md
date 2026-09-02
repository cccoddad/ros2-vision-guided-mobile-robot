# ROS2 视觉引导自主泊车机器人

这是一个在硬件到货前即可开发的 ROS 2 软件工程。当前优先目标是让**仿真底盘**、视觉泊车状态机和安全逻辑形成可测试闭环；硬件到货后再替换为真实串口驱动并完成标定。

## 当前阶段：软件在环（SIL）

SIL（Software-in-the-Loop，软件在环）是指用软件模拟底盘、传感器和故障，而不是连接真实电机。在这个阶段可以安全验证：

- ROS 2 节点、话题和 Action 接口；
- AprilTag 泊车状态机与速度限幅；
- Tag 丢失、消息超时、任务取消和通信故障后的停车逻辑；
- URDF/TF 机器人模型和 Gazebo 仿真场景；
- 通信协议的 CRC、帧同步和错误处理。

不能在此阶段声称已验证的内容包括 PID 参数、真实里程计、相机标定、供电安全和实车成功率。

## 规划的 ROS 2 包

| 包 | 责任 | 无硬件阶段的实现 |
| --- | --- | --- |
| `robot_description` | 机器人 URDF/xacro 与 TF | 名义轮距、轮径和相机位置的模型 |
| `robot_interfaces` | 自定义消息与 Action | `BaseStatus` 与 `ParkToTag` 接口 |
| `base_driver` | 真实 STM32 通信、里程计 | C++ 接口骨架与 SIL FakeTransport 状态适配测试已实现；当前仅发布禁用状态，不访问硬件 |
| `base_driver_mock` | 模拟底盘与故障 | 发布模拟 `/odom`、`/base_status` |
| `parking_controller` | 搜索、对齐、接近与停车 | C++ `ParkToTag` Action；兼容模拟 Tag 位姿，真实检测器适配后续实现 |
| `robot_bringup` | 启动编排与参数 | 仿真/硬件配置分离 |
| `robot_simulation` | Gazebo 世界、插件、故障注入 | 已提供虚拟差速车、停车线、障碍物、模拟 Tag 板和 ROS 2 桥接 |

## 从这里开始

开发环境已验证后，先按 [第 2 步：编译并验证模拟底盘](docs/02_编译并验证模拟底盘.md) 启动无硬件的 ROS 2 底盘，再按 [第 3 步：在 RViz 查看模拟机器人](docs/03_RViz查看模拟机器人.md) 查看模型与运动，按 [第 4 步：在 Gazebo 运行物理仿真](docs/04_Gazebo物理仿真.md) 验证物理运动，最后按 [第 5 步：验证仿真 AprilTag 自动泊车](docs/05_仿真AprilTag自动泊车.md) 验证泊车控制闭环。

第 6 步 [C++ 底盘驱动接口骨架](docs/06_C++底盘驱动接口骨架.md) 是实机驱动迁移的起点；它不能替代 `base_driver_mock` 或 Gazebo，也不能连接真实硬件。

第 7 步 [阶段实施记录与下一步](docs/07_阶段实施记录与下一步.md) 汇总当前已验证的成果、复现命令和下一轮开发的起点。遇到过的问题可分别查看[面试版复盘](docs/项目问题汇总-面试版.md)与[通俗版复盘](docs/项目问题汇总-通俗版.md)。

## 目录说明

- `src/`：ROS 2 软件包源码。
- `config/`：硬件与仿真参数。
- `launch/`：启动文件。
- `protocol/`：上位机与 STM32 通信约定。
- `hardware/`：到货后的接线图、数据表和资产记录。
- `test_results/`：正式测试的 CSV、日志索引和报告。
- `docs/`：开发和复现说明。
