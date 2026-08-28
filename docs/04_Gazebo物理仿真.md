# 第 4 步：在 Gazebo 运行物理仿真

## 本步骤完成什么

本步骤创建一个 Gazebo Harmonic 场景：地面、停车线、静态障碍物和一台可差速驱动的虚拟底盘。ROS 2 的标准 `/cmd_vel` 话题会通过桥接发送给 Gazebo，Gazebo 再把仿真里程计发布回 ROS 2 的 `/odom`。

**Gazebo** 是带物理引擎的仿真器，会计算重力、碰撞和车轮运动；它比 RViz 更接近运行环境，但模型尺寸、质量、摩擦和电机能力仍是名义参数，不能代表真实车的性能或安全性。

## 先停止第 2 步的模拟底盘

在仍显示 `Mock base ready` 的终端 A 按 `Ctrl+C`。这只停止软件模拟节点，不会关闭 Ubuntu，也不会删除文件。

这样做是因为 `base_driver_mock` 和 Gazebo 都会使用 `/cmd_vel`、`/odom`：

- `/cmd_vel`：速度命令；
- `/odom`：里程计；
- **话题冲突**：两个节点同时发布或消费同一个话题，会让数据来源混在一起，无法判断看到的是哪一个模拟器的结果。

RViz 也可按 `Ctrl+C` 退出，Gazebo 会自行显示机器人和场景。

## 编译并启动 Gazebo

新开一个终端标签页，完整执行：

```bash
source /opt/ros/jazzy/setup.bash
cd /mnt/hgfs/robot_project

BUILD_ROOT="$HOME/robot_ws_build"

colcon --log-base "$BUILD_ROOT/log" build \
  --build-base "$BUILD_ROOT/build" \
  --install-base "$BUILD_ROOT/install" \
  --packages-select robot_simulation \
  --event-handlers console_direct+ && \
source "$BUILD_ROOT/install/robot_simulation/share/robot_simulation/local_setup.bash" && \
ros2 pkg prefix robot_simulation && \
ros2 launch robot_simulation gazebo.launch.py
```

`ros2 pkg prefix robot_simulation` 应先显示其安装目录，随后 Gazebo 图形窗口会出现。保持这个终端运行；按 `Ctrl+C` 会同时停止 Gazebo 和 ROS 2 桥接。

术语说明：

- **SDF**：Gazebo 用来描述世界、模型、碰撞体和插件的 XML 格式。
- **DiffDrive 插件**：Gazebo 的差速底盘控制插件。它根据左右轮速度差让底盘前进或转向。
- **ROS-Gazebo 桥接**：`ros_gz_bridge` 在 ROS 2 消息与 Gazebo 消息之间转换；本项目只桥接 `/cmd_vel`、`/odom` 和 `/clock`。
- **`/clock`**：仿真时间话题。需要以仿真时间运行的节点可使用它；本步骤不把它当作真实时间。

## 发送测试运动

Gazebo 打开后，再新开一个终端并执行：

```bash
source /opt/ros/jazzy/setup.bash

timeout 5s ros2 topic pub --rate 20 /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.10}, angular: {z: 0.25}}"
```

机器人应在 Gazebo 中前进并向左转 5 秒。`timeout` 自动停止发消息；Gazebo 中没有额外的超时停车插件，因此这一步只用于受控演示，不能作为真实底盘安全功能的验证。
