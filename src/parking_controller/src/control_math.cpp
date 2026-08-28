#include "parking_controller/control_math.hpp"

#include <algorithm>
#include <cmath>

namespace parking_controller
{

namespace
{

double clamp(double value, double lower, double upper)
{
  return std::max(lower, std::min(value, upper));
}

}  // namespace

double normalize_angle(double angle_rad)
{
  return std::atan2(std::sin(angle_rad), std::cos(angle_rad));
}

ParkingCommand calculate_command(
  double relative_x_m,
  double relative_y_m,
  double relative_tag_yaw_rad,
  double desired_distance_m,
  double max_linear_mps,
  double max_angular_rps,
  double distance_gain,
  double heading_gain,
  double lateral_gain)
{
  const double distance_error_m = relative_x_m - desired_distance_m;
  const double lateral_error_m = relative_y_m;
  // A Tag facing the robot has a relative yaw of pi when alignment is correct.
  const double heading_error_rad = normalize_angle(relative_tag_yaw_rad - kPi);
  const double target_heading_rad = std::atan2(lateral_error_m, std::max(relative_x_m, 0.05));

  return ParkingCommand{
    clamp(distance_gain * distance_error_m, 0.0, max_linear_mps),
    clamp(
      heading_gain * heading_error_rad + lateral_gain * target_heading_rad,
      -max_angular_rps, max_angular_rps),
    distance_error_m,
    lateral_error_m,
    heading_error_rad,
  };
}

bool pose_is_within_tolerance(
  const ParkingCommand & command,
  double distance_tolerance_m,
  double lateral_tolerance_m,
  double yaw_tolerance_rad)
{
  return std::abs(command.distance_error_m) <= distance_tolerance_m &&
         std::abs(command.lateral_error_m) <= lateral_tolerance_m &&
         std::abs(command.heading_error_rad) <= yaw_tolerance_rad;
}

double distance_to_tag_m(double relative_x_m, double relative_y_m)
{
  return std::hypot(relative_x_m, relative_y_m);
}

}  // namespace parking_controller
