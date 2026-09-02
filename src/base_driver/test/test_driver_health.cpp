#include "gtest/gtest.h"

#include "base_driver/driver_health.hpp"

namespace
{

TEST(DriverHealth, RequiresConfigurationAndHealthyStatusBeforeActivation)
{
  base_driver::DriverHealth health;

  EXPECT_FALSE(health.activate());
  EXPECT_TRUE(health.configure());
  EXPECT_FALSE(health.activate());

  health.observe(base_driver::StatusReceiveResult::kAccepted, false, false);
  EXPECT_TRUE(health.activate());
  EXPECT_EQ(health.lifecycle_state(), base_driver::DriverLifecycleState::kActive);
  EXPECT_TRUE(health.deactivate());
  EXPECT_EQ(health.lifecycle_state(), base_driver::DriverLifecycleState::kInactive);
}

TEST(DriverHealth, ReportsProtocolErrorsAsWarnings)
{
  base_driver::DriverHealth health;
  ASSERT_TRUE(health.configure());

  health.observe(base_driver::StatusReceiveResult::kCrcMismatch, false, false);
  const auto crc_diagnostic = health.diagnostic();
  EXPECT_EQ(crc_diagnostic.level, base_driver::DriverDiagnosticLevel::kWarning);
  EXPECT_EQ(crc_diagnostic.summary, "base status CRC mismatch");
  EXPECT_FALSE(health.activate());

  health.observe(base_driver::StatusReceiveResult::kOutOfOrderSequence, false, false);
  const auto sequence_diagnostic = health.diagnostic();
  EXPECT_EQ(sequence_diagnostic.level, base_driver::DriverDiagnosticLevel::kWarning);
  EXPECT_EQ(sequence_diagnostic.summary, "base status sequence out of order");
}

TEST(DriverHealth, TimeoutTransitionsToErrorAndRequiresHealthyRecovery)
{
  base_driver::DriverHealth health;
  ASSERT_TRUE(health.configure());
  health.observe(base_driver::StatusReceiveResult::kAccepted, false, false);
  ASSERT_TRUE(health.activate());

  health.observe(base_driver::StatusReceiveResult::kNoFrame, true, false);
  const auto timeout_diagnostic = health.diagnostic();
  EXPECT_EQ(health.lifecycle_state(), base_driver::DriverLifecycleState::kError);
  EXPECT_EQ(timeout_diagnostic.level, base_driver::DriverDiagnosticLevel::kError);
  EXPECT_EQ(timeout_diagnostic.summary, "base status communication timeout");
  EXPECT_FALSE(health.recover());

  health.observe(base_driver::StatusReceiveResult::kAccepted, false, false);
  EXPECT_TRUE(health.recover());
  EXPECT_EQ(health.lifecycle_state(), base_driver::DriverLifecycleState::kInactive);
}

TEST(DriverHealth, EstopHasErrorPrecedence)
{
  base_driver::DriverHealth health;
  ASSERT_TRUE(health.configure());
  health.observe(base_driver::StatusReceiveResult::kAccepted, false, true);

  const auto diagnostic = health.diagnostic();
  EXPECT_EQ(health.lifecycle_state(), base_driver::DriverLifecycleState::kError);
  EXPECT_EQ(diagnostic.level, base_driver::DriverDiagnosticLevel::kError);
  EXPECT_EQ(diagnostic.summary, "STM32 emergency stop active");
}

}  // namespace
