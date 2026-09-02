# STM32 底盘通信协议

此目录保存上位机与 STM32 之间的二进制协议。协议先于硬件实现：上位机、模拟器和固件可共享相同的帧格式，而不需要连接开发板。

## V1 帧格式

每帧均为下列顺序，整数采用小端序（低字节在前）：

| 字段 | 字节 | 值 / 含义 |
| --- | ---: | --- |
| SOF | 2 | 固定 `0xAA 0x55`，用于串口流中的帧同步 |
| version | 1 | 当前固定为 `1` |
| message type | 1 | 消息类型 |
| sequence | 1 | 发送方递增的序号；用于诊断丢帧或乱序 |
| payload length | 1 | `0` 至 `48` |
| payload | 0–48 | 由消息类型定义 |
| CRC16 | 2 | CRC-16/CCITT-FALSE，小端；覆盖 `version` 至 payload，不覆盖 SOF |

解码器要求输入长度恰好等于长度字段推导的帧长；错误 SOF、版本、长度或 CRC 的帧必须丢弃，绝不执行其中的控制命令。

## 消息类型与单位

| 名称 | 值 | V1 负载 |
| --- | ---: | --- |
| `SET_TWIST` | `0x01` | `int16 linear_speed_mmps`、`int16 angular_speed_mradps`；均为小端、有符号整数 |
| `E_STOP` | `0x02` | 空；STM32 必须立即关闭 PWM 并锁存故障 |
| `CLEAR_FAULT` | `0x03` | 空；仅在急停释放、通信恢复且硬件检查允许时由 STM32 接受 |
| `PING` | `0x04` | 保留给链路存活检测 |
| `BASE_STATUS` | `0x80` | 预留给 STM32 状态；字段将在编码器、电池和驱动器规格确定后冻结 |

**`SET_TWIST` 只表达期望车体速度，绝不表达裸 PWM。** STM32 仍是急停、通信超时、PWM 禁用和硬件故障处理的最终权威。

### SIL 专用 `BASE_STATUS` 测试负载

`base_driver` 的 C++ FakeTransport 单元测试使用以下 11 字节负载验证 C 帧解码、状态映射和故障处理。这**不是** STM32 V1 硬件协议的冻结字段；硬件规格、编码器和驱动器故障定义确定后，必须经接口评审后再替换。

| 偏移 | 字节 | SIL 测试含义 |
| ---: | ---: | --- |
| 0 | 2 | `int16 left_wheel_speed_mmps`，小端、有符号 |
| 2 | 2 | `int16 right_wheel_speed_mmps`，小端、有符号 |
| 4 | 2 | `uint16 battery_voltage_mv`，小端 |
| 6 | 4 | `uint32 fault_flags`，小端，原样映射到 `BaseStatus` |
| 10 | 1 | `0` 为未急停，`1` 为急停，映射到 `BaseStatus.estop_active` |

FakeTransport 仅在进程内存中提供测试帧，不访问串口、CAN、STM32 或电机。C++ 接收器必须拒绝 CRC 损坏帧、非连续序号和非法负载；仅有效状态帧会刷新通信超时计时器。

## 纯 C 编解码器和测试

- [include/robot_protocol.h](include/robot_protocol.h)：无动态内存的 C API；
- [src/robot_protocol.c](src/robot_protocol.c)：CRC、完整帧编码和严格解码；
- [test/test_robot_protocol.c](test/test_robot_protocol.c)：CRC 参考向量、速度帧往返、损坏 CRC、长度与版本拒绝，以及确定性模糊输入和全帧单比特翻转回归。

在 Ubuntu 或 STM32 工具链可用的 Linux 环境中，可在项目根目录运行：

```bash
cd /mnt/hgfs/robot_project

PROTOCOL_BUILD="$HOME/robot_ws_build/protocol"
mkdir -p "$PROTOCOL_BUILD"

cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I protocol/include \
  protocol/src/robot_protocol.c \
  protocol/test/test_robot_protocol.c \
  -o "$PROTOCOL_BUILD/test_robot_protocol"

"$PROTOCOL_BUILD/test_robot_protocol"
```

这是独立 C 测试，不是 ROS 2 包，因此不使用 `colcon`，也不访问串口或任何硬件。成功标志为：

```text
PASS: robot protocol codec tests passed.
```
