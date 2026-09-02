#ifndef BASE_DRIVER__DRIVER_CONFIGURATION_HPP_
#define BASE_DRIVER__DRIVER_CONFIGURATION_HPP_

#include <cmath>
#include <stdexcept>
#include <string>

namespace base_driver
{

struct DriverConfiguration
{
  double update_rate_hz{20.0};
  double command_timeout_s{0.3};
  double max_linear_speed_mps{0.20};
  double max_angular_speed_rps{0.80};
  double battery_voltage{0.0};
  std::string base_frame{"base_link"};
  std::string transport_mode{"disabled"};

  void validate() const
  {
    if (!std::isfinite(update_rate_hz) || !std::isfinite(command_timeout_s) ||
      !std::isfinite(max_linear_speed_mps) || !std::isfinite(max_angular_speed_rps) ||
      !std::isfinite(battery_voltage) || update_rate_hz <= 0.0 || command_timeout_s <= 0.0 ||
      max_linear_speed_mps <= 0.0 || max_angular_speed_rps <= 0.0 || battery_voltage < 0.0)
    {
      throw std::invalid_argument("base driver numeric parameters must be finite and valid");
    }
    if (base_frame.empty()) {
      throw std::invalid_argument("base_frame must not be empty");
    }
    if (transport_mode != "disabled") {
      throw std::invalid_argument("only the disabled transport mode is implemented in this stage");
    }
  }
};

}  // namespace base_driver

#endif  // BASE_DRIVER__DRIVER_CONFIGURATION_HPP_
