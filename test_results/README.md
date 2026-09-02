# 测试证据索引

本目录只保存可审阅的测试索引、结构化摘要和脱敏 CSV。不要提交 `build/`、`install/`、`log/`、原始 rosbag、视频、密钥、令牌或个人数据。大体积原始资料应保存在经批准的外部存储，并在此记录可追溯的标识或受控链接。

## 证据记录规则

每条证据必须包含：测试编号、日期、Git 提交、环境、执行命令、输入/故障注入、期望结果、实际结果、产物位置和审核人。没有硬件的记录必须明确标注为 SIL；SIL 通过不能替代 HIL 或实车结论。

建议文件名：`YYYYMMDD_<test-id>_<summary>.md` 或 `YYYYMMDD_<test-id>_<summary>.csv`。

## 当前已记录的 SIL 证据

| 证据编号 | 范围 | 类型 | 当前状态 | 已知结果 | 参考命令/产物 | 硬件结论 |
| --- | --- | --- | --- | --- | --- | --- |
| `SIL-PROTO-001` | V1 C 编解码、CRC、长度/版本拒绝、单比特翻转与确定性模糊输入 | C11 单元测试 | 通过 | `PASS: robot protocol codec tests passed.` | `protocol/test/test_robot_protocol.c` | 无 |
| `SIL-BASE-001` | FakeTransport、状态映射、CRC、乱序、超时、急停、命令编码与配置边界 | C++ GTest | 通过 | `base_driver` 当前汇总 33 项测试，0 错误/失败 | `src/base_driver/test/` | 无 |
| `SIL-PARK-001` | Gazebo 正常 Tag 泊车 | Gazebo/ROS 2 | 通过 | 模拟停车距离 `0.390 m` | `scripts/run_simulated_parking.py` | 无 |
| `SIL-PARK-002` | 模拟 Tag 丢失后停止 | Gazebo/ROS 2 | 通过 | `failure_code=1`，线/角速度均 `0.0000` | `scripts/verify_tag_loss_stop.py` | 无 |
| `SIL-PARK-003` | 正常泊车 + Tag 丢失回归编排 | Gazebo/ROS 2 | 通过 | `PASS: SIL parking regression completed normal parking and Tag-loss stop checks.` | `scripts/run_sil_parking_regression.py` | 无 |

以上记录是当前源码和已报告验证的摘要；每次关键版本验收应按下方模板新增带日期和提交哈希的独立证据文件。

## 待硬件的证据计划

| 证据编号 | 前置条件 | 验收目标 | 当前状态 | 不可替代的硬件证据 |
| --- | --- | --- | --- | --- |
| `HIL-PROTO-001` | `BASE_STATUS` 评审模板冻结、STM32 固件可编译 | 真实帧字段、缩放、CRC、序号与 ROS 映射一致 | 阻塞：无硬件规格/固件 | 逻辑分析仪/CAN 抓帧、固件提交、C++ 测试结果 |
| `HIL-SAFETY-001` | 急停、驱动器、电池与 PWM 硬件到位 | 断开 ROS 2 后 STM32 仍独立急停、命令超时并禁用 PWM | 阻塞：无硬件 | 台架流程、示波器/驱动器状态、审核记录 |
| `HIL-ODOM-001` | 编码器、轮径/轮距实测和低速支架 | 编码器方向、里程计和 TF 与实测运动一致 | 阻塞：无硬件 | 标定数据、受控运动记录、误差摘要 |
| `HIL-VISION-001` | 相机、标定板、AprilTag 场景 | 内外参、检测率、延迟和失效停车行为 | 阻塞：无相机 | 标定报告、光照场景数据、受控停车记录 |

## 单次证据模板

复制以下内容到一个新的日期文件中；未填写项保留 `TBD`，不得自行编造。

```markdown
# <证据编号> — <简短标题>

- 日期：`YYYY-MM-DD`
- 类型：`SIL` / `HIL` / `实车`
- Git 提交：`<full commit hash>`
- 审核人：`TBD`
- 环境：OS、ROS 2、Gazebo/板卡/固件版本为 `TBD`

## 前置条件

- `TBD`

## 命令或台架步骤

```bash
# 只填写实际执行过的命令
```

## 输入与故障注入

- `TBD`

## 期望结果

- `TBD`

## 实际结果

- `TBD`

## 产物与完整性

- 摘要/CSV：`TBD`
- 原始资料受控位置：`TBD`
- 文件校验或设备记录：`TBD`

## 结论与限制

- `TBD`
```
