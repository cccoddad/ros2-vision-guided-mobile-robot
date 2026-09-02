#ifndef BASE_DRIVER__COMMAND_FRAME_ENCODER_HPP_
#define BASE_DRIVER__COMMAND_FRAME_ENCODER_HPP_

#include <cstdint>
#include <optional>

#include "base_driver/transport.hpp"

namespace base_driver
{

class CommandFrameEncoder
{
public:
  CommandFrameEncoder(double max_linear_speed_mps, double max_angular_speed_rps);

  std::optional<Transport::Frame> encode(
    double linear_speed_mps,
    double angular_speed_rps,
    std::uint8_t sequence) const;

private:
  double max_linear_speed_mps_;
  double max_angular_speed_rps_;
};

}  // namespace base_driver

#endif  // BASE_DRIVER__COMMAND_FRAME_ENCODER_HPP_
