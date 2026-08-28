# 第 5 步：验证仿真 AprilTag 自动泊车

## 本步骤完成什么

本步骤把已有 Gazebo 差速底盘接入 `ParkToTag` Action：控制器读取 `/sim/tag_pose`，对齐 Tag 后向前靠近，并在 Tag 前 `0.35 m` 处停车。Action 客户端会打印反馈和最终误差，以结构化结果判定通过或失败。

这是**软件在环控制验证**，不是相机视觉验证：`/sim/tag_pose` 根据 Gazebo `/odom` 与场景中 Tag 的已知位置计算，代表“理想的模拟传感器”。它不使用相机、没有图像、没有 AprilTag 检测算法，也不能证明真实镜头、光照、标定或识别率。

## 启动前重置场景

请在当前 Gazebo 启动终端按 `Ctrl+C`，再按下面步骤重启。重启会让机器人回到原点，并加载新增的 Tag 板与模拟 Tag 位姿节点；不会删除项目文件。

## 终端 A：编译并启动 Gazebo 与模拟 Tag 位姿

```bash
source /opt/ros/jazzy/setup.bash
cd /mnt/hgfs/robot_project

BUILD_ROOT="$HOME/robot_ws_build"

colcon --log-base "$BUILD_ROOT/log" build \
  --build-base "$BUILD_ROOT/build" \
  --install-base "$BUILD_ROOT/install" \
  --packages-up-to robot_simulation parking_controller \
  --event-handlers console_direct+ && \
source "$BUILD_ROOT/install/robot_interfaces/share/robot_interfaces/local_setup.bash" && \
source "$BUILD_ROOT/install/robot_simulation/share/robot_simulation/local_setup.bash" && \
source "$BUILD_ROOT/install/parking_controller/share/parking_controller/local_setup.bash" && \
ros2 launch robot_simulation gazebo.launch.py
```

`--packages-up-to` 的含义是“编译指定包及其依赖”，因此新消息 `TagPose` 会先于控制器编译。Gazebo 启动后，终端应显示：

```text
Publishing synthetic TagPose on /sim/tag_pose from Gazebo odometry.
```

保持此终端运行。

## 终端 B：启动泊车控制器

新开终端标签页后执行：

```bash
source /opt/ros/jazzy/setup.bash

BUILD_ROOT="$HOME/robot_ws_build"
source "$BUILD_ROOT/install/robot_interfaces/share/robot_interfaces/local_setup.bash"
source "$BUILD_ROOT/install/parking_controller/share/parking_controller/local_setup.bash"

ros2 launch parking_controller parking_controller.launch.py
```

`ParkToTag` 是 **Action**：与一次性消息不同，它能在任务进行中返回状态、允许取消，并在结束时给出成功或失败结果。控制器启动时应显示：

```text
Parking controller ready; waiting for TagPose on /sim/tag_pose.
```

## 终端 C：提交并验证泊车任务

再新开一个终端标签页，执行：

```bash
source /opt/ros/jazzy/setup.bash
cd /mnt/hgfs/robot_project

BUILD_ROOT="$HOME/robot_ws_build"
source "$BUILD_ROOT/install/robot_interfaces/share/robot_interfaces/local_setup.bash"

python3 scripts/run_simulated_parking.py
```

成功时末尾应显示：

```text
PASS: simulated ParkToTag action completed within the requested tolerances.
```

## 安全行为和失败代码

控制器始终将零速度作为任务结束、取消、目标丢失或任务超时后的最后命令。当前失败代码为：

- `1`：`TagPose` 超过 `0.5 s` 没有更新；
- `2`：任务超过目标中的 `30 s` 时限；
- `3`：客户端取消任务。

术语说明：

- **TagPose**：单个 Tag 的编号和相对于机器人的位置、朝向；坐标系为 `base_link`。
- **横向误差**：Tag 在机器人左右方向上的偏差；接近零表示机器人正对 Tag。
- **航向误差**：机器人朝向与目标停车朝向的夹角；接近零表示车头方向正确。
- **容差**：允许的最终误差范围。此次分别要求横向不超过 `0.06 m`、航向不超过 `0.10 rad`。
