#ifndef PARKING_CONTROLLER__CONTROL_MATH_HPP_
#define PARKING_CONTROLLER__CONTROL_MATH_HPP_

namespace parking_controller
{

constexpr double kPi = 3.14159265358979323846;

struct ParkingCommand
{
  double linear_mps;
  double angular_rps;
  double distance_error_m;
  double lateral_error_m;
  double heading_error_rad;
};

double normalize_angle(double angle_rad);

ParkingCommand calculate_command(
  double relative_x_m,
  double relative_y_m,
  double relative_tag_yaw_rad,
  double desired_distance_m,
  double max_linear_mps,
  double max_angular_rps,
  double distance_gain,
  double heading_gain,
  double lateral_gain);

bool pose_is_within_tolerance(
  const ParkingCommand & command,
  double distance_tolerance_m,
  double lateral_tolerance_m,
  double yaw_tolerance_rad);

double distance_to_tag_m(double relative_x_m, double relative_y_m);

}  // namespace parking_controller

#endif  // PARKING_CONTROLLER__CONTROL_MATH_HPP_
