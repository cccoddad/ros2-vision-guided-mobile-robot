#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "gtest/gtest.h"

#include "robot_protocol.h"

#include "base_driver/base_status_receiver.hpp"
#include "base_driver/fake_transport.hpp"

namespace
{

void write_u16_le(std::uint8_t * output, std::uint16_t value)
{
  output[0] = static_cast<std::uint8_t>(value & 0xFFU);
  output[1] = static_cast<std::uint8_t>(value >> 8U);
}

void write_u32_le(std::uint8_t * output, std::uint32_t value)
{
  output[0] = static_cast<std::uint8_t>(value & 0xFFU);
  output[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
  output[2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
  output[3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

std::vector<std::uint8_t> encode_base_status(
  std::uint8_t sequence,
  std::int16_t left_wheel_speed_mmps,
  std::int16_t right_wheel_speed_mmps,
  std::uint16_t battery_voltage_mv,
  std::uint32_t fault_flags,
  bool estop_active)
{
  robot_protocol_frame_t frame = {};
  frame.message_type = ROBOT_PROTOCOL_MESSAGE_BASE_STATUS;
  frame.sequence = sequence;
  frame.payload_length = 11U;
  write_u16_le(&frame.payload[0], static_cast<std::uint16_t>(left_wheel_speed_mmps));
  write_u16_le(&frame.payload[2], static_cast<std::uint16_t>(right_wheel_speed_mmps));
  write_u16_le(&frame.payload[4], battery_voltage_mv);
  write_u32_le(&frame.payload[6], fault_flags);
  frame.payload[10] = estop_active ? 1U : 0U;

  std::array<std::uint8_t, ROBOT_PROTOCOL_MAX_FRAME_SIZE> encoded = {};
  std::size_t encoded_length = 0U;
  EXPECT_EQ(
    robot_protocol_encode(&frame, encoded.data(), encoded.size(), &encoded_length),
    ROBOT_PROTOCOL_OK);
  return {encoded.begin(), encoded.begin() + static_cast<std::ptrdiff_t>(encoded_length)};
}

TEST(BaseStatusReceiver, MapsFakeProtocolFrameToExistingBaseStatusMessage)
{
  const auto start = base_driver::BaseStatusReceiver::Clock::time_point{};
  base_driver::FakeTransport transport;
  base_driver::BaseStatusReceiver receiver(std::chrono::milliseconds(300));
  robot_interfaces::msg::BaseStatus status;
  transport.push_received(encode_base_status(17U, -125, 250, 12340U, 0x12U, false));

  EXPECT_EQ(
    receiver.receive(transport, start, status), base_driver::StatusReceiveResult::kAccepted);
  EXPECT_FLOAT_EQ(status.left_wheel_speed_mps, -0.125F);
  EXPECT_FLOAT_EQ(status.right_wheel_speed_mps, 0.250F);
  EXPECT_FLOAT_EQ(status.battery_voltage, 12.340F);
  EXPECT_EQ(status.fault_flags, 0x12U);
  EXPECT_FALSE(status.estop_active);
  EXPECT_EQ(status.sequence, 17U);
  EXPECT_FALSE(receiver.communication_timed_out(start + std::chrono::milliseconds(300)));
}

TEST(BaseStatusReceiver, RejectsCorruptedCrcFrame)
{
  const auto start = base_driver::BaseStatusReceiver::Clock::time_point{};
  base_driver::FakeTransport transport;
  base_driver::BaseStatusReceiver receiver(std::chrono::milliseconds(300));
  robot_interfaces::msg::BaseStatus status;
  auto encoded = encode_base_status(1U, 0, 0, 12000U, 0U, false);
  encoded.back() ^= 0xFFU;
  transport.push_received(std::move(encoded));

  EXPECT_EQ(
    receiver.receive(transport, start, status), base_driver::StatusReceiveResult::kCrcMismatch);
  EXPECT_TRUE(receiver.communication_timed_out(start));
}

TEST(BaseStatusReceiver, RejectsOutOfOrderSequence)
{
  const auto start = base_driver::BaseStatusReceiver::Clock::time_point{};
  base_driver::FakeTransport transport;
  base_driver::BaseStatusReceiver receiver(std::chrono::milliseconds(300));
  robot_interfaces::msg::BaseStatus status;
  transport.push_received(encode_base_status(3U, 0, 0, 12000U, 0U, false));
  transport.push_received(encode_base_status(5U, 0, 0, 12000U, 0U, false));

  EXPECT_EQ(
    receiver.receive(transport, start, status), base_driver::StatusReceiveResult::kAccepted);
  EXPECT_EQ(
    receiver.receive(transport, start + std::chrono::milliseconds(1), status),
    base_driver::StatusReceiveResult::kOutOfOrderSequence);
  EXPECT_FALSE(receiver.communication_timed_out(start + std::chrono::milliseconds(300)));
}

TEST(BaseStatusReceiver, ReportsCommunicationTimeoutAfterLastValidFrame)
{
  const auto start = base_driver::BaseStatusReceiver::Clock::time_point{};
  base_driver::FakeTransport transport;
  base_driver::BaseStatusReceiver receiver(std::chrono::milliseconds(300));
  robot_interfaces::msg::BaseStatus status;
  transport.push_received(encode_base_status(9U, 0, 0, 12000U, 0U, false));

  EXPECT_EQ(
    receiver.receive(transport, start, status), base_driver::StatusReceiveResult::kAccepted);
  EXPECT_FALSE(receiver.communication_timed_out(start + std::chrono::milliseconds(300)));
  EXPECT_TRUE(receiver.communication_timed_out(start + std::chrono::milliseconds(301)));
}

TEST(BaseStatusReceiver, MapsEstopStateFromFakeProtocolFrame)
{
  const auto start = base_driver::BaseStatusReceiver::Clock::time_point{};
  base_driver::FakeTransport transport;
  base_driver::BaseStatusReceiver receiver(std::chrono::milliseconds(300));
  robot_interfaces::msg::BaseStatus status;
  transport.push_received(encode_base_status(8U, 0, 0, 12000U, 0x40U, true));

  EXPECT_EQ(
    receiver.receive(transport, start, status), base_driver::StatusReceiveResult::kAccepted);
  EXPECT_TRUE(status.estop_active);
  EXPECT_EQ(status.fault_flags, 0x40U);
}

}  // namespace
