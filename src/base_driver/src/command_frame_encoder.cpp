#include "base_driver/command_frame_encoder.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

#include "robot_protocol.h"

namespace base_driver
{
namespace
{

constexpr double kMillimetresPerMetre = 1000.0;
constexpr double kMilliradiansPerRadian = 1000.0;
constexpr double kMaximumProtocolSpeed =
  static_cast<double>(std::numeric_limits<std::int16_t>::max()) / kMillimetresPerMetre;

std::int16_t to_protocol_speed(double speed, double scale)
{
  return static_cast<std::int16_t>(std::lround(speed * scale));
}

}  // namespace

CommandFrameEncoder::CommandFrameEncoder(
  double max_linear_speed_mps,
  double max_angular_speed_rps)
: max_linear_speed_mps_(max_linear_speed_mps),
  max_angular_speed_rps_(max_angular_speed_rps)
{
  if (!std::isfinite(max_linear_speed_mps_) || !std::isfinite(max_angular_speed_rps_) ||
    max_linear_speed_mps_ <= 0.0 || max_angular_speed_rps_ <= 0.0 ||
    max_linear_speed_mps_ > kMaximumProtocolSpeed ||
    max_angular_speed_rps_ > kMaximumProtocolSpeed)
  {
    throw std::invalid_argument("command limits must be finite, positive, and protocol-representable");
  }
}

std::optional<Transport::Frame> CommandFrameEncoder::encode(
  double linear_speed_mps,
  double angular_speed_rps,
  std::uint8_t sequence) const
{
  if (!std::isfinite(linear_speed_mps) || !std::isfinite(angular_speed_rps)) {
    return std::nullopt;
  }

  const auto clamped_linear_speed_mps = std::clamp(
    linear_speed_mps, -max_linear_speed_mps_, max_linear_speed_mps_);
  const auto clamped_angular_speed_rps = std::clamp(
    angular_speed_rps, -max_angular_speed_rps_, max_angular_speed_rps_);
  const robot_protocol_twist_command_t command = {
    to_protocol_speed(clamped_linear_speed_mps, kMillimetresPerMetre),
    to_protocol_speed(clamped_angular_speed_rps, kMilliradiansPerRadian),
  };
  std::array<std::uint8_t, ROBOT_PROTOCOL_MAX_FRAME_SIZE> encoded = {};
  std::size_t encoded_length = 0U;

  if (robot_protocol_encode_set_twist(
      sequence, &command, encoded.data(), encoded.size(), &encoded_length) != ROBOT_PROTOCOL_OK)
  {
    return std::nullopt;
  }

  return Transport::Frame(
    encoded.begin(), encoded.begin() + static_cast<std::ptrdiff_t>(encoded_length));
}

}  // namespace base_driver
