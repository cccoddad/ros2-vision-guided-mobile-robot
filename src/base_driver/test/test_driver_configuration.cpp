#include <limits>
#include <stdexcept>

#include "gtest/gtest.h"

#include "base_driver/driver_configuration.hpp"

namespace
{

TEST(DriverConfiguration, AcceptsDefaultDisabledConfiguration)
{
  base_driver::DriverConfiguration configuration;

  EXPECT_NO_THROW(configuration.validate());
}

TEST(DriverConfiguration, RejectsInvalidNumericParameters)
{
  base_driver::DriverConfiguration configuration;
  configuration.update_rate_hz = 0.0;
  EXPECT_THROW(configuration.validate(), std::invalid_argument);

  configuration = base_driver::DriverConfiguration{};
  configuration.command_timeout_s = -0.1;
  EXPECT_THROW(configuration.validate(), std::invalid_argument);

  configuration = base_driver::DriverConfiguration{};
  configuration.max_linear_speed_mps = std::numeric_limits<double>::infinity();
  EXPECT_THROW(configuration.validate(), std::invalid_argument);

  configuration = base_driver::DriverConfiguration{};
  configuration.battery_voltage = -0.1;
  EXPECT_THROW(configuration.validate(), std::invalid_argument);
}

TEST(DriverConfiguration, RejectsInvalidFrameAndTransportMode)
{
  base_driver::DriverConfiguration configuration;
  configuration.base_frame = "";
  EXPECT_THROW(configuration.validate(), std::invalid_argument);

  configuration = base_driver::DriverConfiguration{};
  configuration.transport_mode = "serial";
  EXPECT_THROW(configuration.validate(), std::invalid_argument);
}

}  // namespace
