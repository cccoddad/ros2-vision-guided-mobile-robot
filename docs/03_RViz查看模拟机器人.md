# 第 3 步：在 RViz 查看模拟机器人

## 本步骤完成什么

本步骤在已有的模拟底盘数据上显示一个简化的机器人模型、`odom -> base_link` 坐标系和里程计箭头。模型的长宽、轮距和相机位置均为**名义仿真尺寸**，只用于软件在环验证，不能替代真实硬件的测量、相机标定或碰撞安全测试。

**RViz** 是 ROS 2 的数据可视化工具：它显示 ROS 2 话题和坐标系，但不计算重力、碰撞或轮胎物理。Gazebo 是带物理引擎的仿真器；本步骤先用 RViz 验证数据与模型坐标一致，下一阶段再加入 Gazebo 场景。

## 终端 A：保持模拟底盘运行

第 2 步的终端 A 应持续显示：

```text
Mock base ready: publishing /odom and /base_status
```

这表示模拟底盘仍在发布运动数据。不要关闭它。

## 终端 B：编译并打开 RViz

按 `Ctrl+Shift+T` 新开一个终端标签页，完整执行：

```bash
source /opt/ros/jazzy/setup.bash
cd /mnt/hgfs/robot_project

BUILD_ROOT="$HOME/robot_ws_build"
source "$BUILD_ROOT/install/setup.bash" && \
colcon --log-base "$BUILD_ROOT/log" build \
  --build-base "$BUILD_ROOT/build" \
  --install-base "$BUILD_ROOT/install" \
  --packages-select robot_description \
  --event-handlers console_direct+ && \
source "$BUILD_ROOT/install/setup.bash" && \
ros2 launch robot_description rviz.launch.py
```

成功后会打开 RViz 窗口。蓝色车体、黑色车轮与橙色相机块是模型；网格是 `odom` 平面；坐标轴表示 TF；红色箭头来自 `/odom`。

术语说明：

- **URDF**：用 XML 描述机器人连杆、关节和外观的文件格式。
- **`robot_state_publisher`**：读取 URDF 并发布机器人各部件之间固定坐标关系的 ROS 2 节点。
- **`odom`**：里程计坐标系，表示模拟底盘从启动位置开始的相对位姿。
- **`base_link`**：机器人本体的中心坐标系；模拟底盘发布 `odom -> base_link`，RViz 因此能让模型移动。

## 终端 C：让模型运动

保持终端 A 与 B 都运行，再新开一个终端标签页并执行：

```bash
source /opt/ros/jazzy/setup.bash
source "$HOME/robot_ws_build/install/setup.bash"

timeout 4s ros2 topic pub --rate 20 /cmd_vel geometry_msgs/msg/Twist "{linear: {x: 0.10}, angular: {z: 0.30}}"
```

这会发送 4 秒速度命令：每秒前进 `0.10` 米并以每秒 `0.30` 弧度左转。`timeout 4s` 会在 4 秒后自动停止命令；之后模拟底盘的 `0.3` 秒通信超时保护会让速度归零。

RViz 中车体和红色里程计箭头应随之向前并向左转。结束时关闭 RViz 可按 `Ctrl+C`，模拟底盘终端同样使用 `Ctrl+C` 停止。
