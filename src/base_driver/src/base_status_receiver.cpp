#include "base_driver/base_status_receiver.hpp"

#include <cstddef>
#include <cstdint>

#include "robot_protocol.h"

namespace base_driver
{
namespace
{

constexpr std::uint8_t kBaseStatusPayloadSize = 11U;
constexpr std::size_t kLeftWheelSpeedOffset = 0U;
constexpr std::size_t kRightWheelSpeedOffset = 2U;
constexpr std::size_t kBatteryVoltageOffset = 4U;
constexpr std::size_t kFaultFlagsOffset = 6U;
constexpr std::size_t kEstopOffset = 10U;

std::uint16_t read_u16_le(const std::uint8_t * input)
{
  return static_cast<std::uint16_t>(input[0]) |
         static_cast<std::uint16_t>(static_cast<std::uint16_t>(input[1]) << 8U);
}

std::uint32_t read_u32_le(const std::uint8_t * input)
{
  return static_cast<std::uint32_t>(input[0]) |
         (static_cast<std::uint32_t>(input[1]) << 8U) |
         (static_cast<std::uint32_t>(input[2]) << 16U) |
         (static_cast<std::uint32_t>(input[3]) << 24U);
}

}  // namespace

BaseStatusReceiver::BaseStatusReceiver(std::chrono::nanoseconds communication_timeout)
: communication_timeout_(communication_timeout)
{
}

StatusReceiveResult BaseStatusReceiver::receive(
  Transport & transport,
  TimePoint now,
  robot_interfaces::msg::BaseStatus & status)
{
  const auto encoded_frame = transport.read();
  if (!encoded_frame.has_value()) {
    return StatusReceiveResult::kNoFrame;
  }

  robot_protocol_frame_t frame = {};
  const auto decode_result = robot_protocol_decode(
    encoded_frame->data(), encoded_frame->size(), &frame);
  if (decode_result == ROBOT_PROTOCOL_CRC_MISMATCH) {
    return StatusReceiveResult::kCrcMismatch;
  }
  if (decode_result != ROBOT_PROTOCOL_OK) {
    return StatusReceiveResult::kDecodeError;
  }
  if (frame.message_type != ROBOT_PROTOCOL_MESSAGE_BASE_STATUS) {
    return StatusReceiveResult::kUnexpectedMessageType;
  }
  if (frame.payload_length != kBaseStatusPayloadSize || frame.payload[kEstopOffset] > 1U) {
    return StatusReceiveResult::kInvalidPayload;
  }
  if (last_sequence_.has_value() &&
    frame.sequence != static_cast<std::uint8_t>(*last_sequence_ + 1U))
  {
    return StatusReceiveResult::kOutOfOrderSequence;
  }

  status.left_wheel_speed_mps =
    static_cast<float>(static_cast<std::int16_t>(read_u16_le(&frame.payload[kLeftWheelSpeedOffset]))) /
    1000.0F;
  status.right_wheel_speed_mps =
    static_cast<float>(static_cast<std::int16_t>(read_u16_le(&frame.payload[kRightWheelSpeedOffset]))) /
    1000.0F;
  status.battery_voltage =
    static_cast<float>(read_u16_le(&frame.payload[kBatteryVoltageOffset])) / 1000.0F;
  status.fault_flags = read_u32_le(&frame.payload[kFaultFlagsOffset]);
  status.estop_active = frame.payload[kEstopOffset] != 0U;
  status.sequence = frame.sequence;

  last_sequence_ = frame.sequence;
  last_received_ = now;
  return StatusReceiveResult::kAccepted;
}

bool BaseStatusReceiver::communication_timed_out(TimePoint now) const
{
  return !last_received_.has_value() || now - *last_received_ > communication_timeout_;
}

}  // namespace base_driver
