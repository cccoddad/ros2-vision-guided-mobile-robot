#include <cmath>

#include "gtest/gtest.h"

#include "parking_controller/control_math.hpp"

namespace
{

parking_controller::ParkingCommand command(double x, double y, double yaw)
{
  return parking_controller::calculate_command(x, y, yaw, 0.35, 0.18, 0.70, 0.60, 1.40, 1.20);
}

TEST(ControlMath, AlignedTagDrivesForward)
{
  const auto result = command(1.20, 0.0, parking_controller::kPi);
  EXPECT_GT(result.linear_mps, 0.0);
  EXPECT_NEAR(result.angular_rps, 0.0, 1e-6);
}

TEST(ControlMath, LateralErrorTurnsTowardTag)
{
  EXPECT_GT(command(1.20, 0.20, parking_controller::kPi).angular_rps, 0.0);
}

TEST(ControlMath, HeadingErrorIsCorrected)
{
  EXPECT_LT(command(1.20, 0.0, parking_controller::kPi - 0.30).angular_rps, 0.0);
}

TEST(ControlMath, InToleranceStops)
{
  EXPECT_TRUE(parking_controller::pose_is_within_tolerance(
    command(0.37, 0.01, parking_controller::kPi + 0.02), 0.04, 0.03, 0.05));
}

}  // namespace
