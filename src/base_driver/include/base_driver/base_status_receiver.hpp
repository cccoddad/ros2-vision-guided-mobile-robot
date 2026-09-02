#ifndef BASE_DRIVER__BASE_STATUS_RECEIVER_HPP_
#define BASE_DRIVER__BASE_STATUS_RECEIVER_HPP_

#include <chrono>
#include <cstdint>
#include <optional>

#include "robot_interfaces/msg/base_status.hpp"

#include "base_driver/transport.hpp"

namespace base_driver
{

enum class StatusReceiveResult
{
  kNoFrame,
  kAccepted,
  kCrcMismatch,
  kDecodeError,
  kUnexpectedMessageType,
  kInvalidPayload,
  kOutOfOrderSequence,
};

class BaseStatusReceiver
{
public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  explicit BaseStatusReceiver(std::chrono::nanoseconds communication_timeout);

  StatusReceiveResult receive(
    Transport & transport,
    TimePoint now,
    robot_interfaces::msg::BaseStatus & status);

  bool communication_timed_out(TimePoint now) const;

private:
  std::chrono::nanoseconds communication_timeout_;
  std::optional<std::uint8_t> last_sequence_;
  std::optional<TimePoint> last_received_;
};

}  // namespace base_driver

#endif  // BASE_DRIVER__BASE_STATUS_RECEIVER_HPP_
