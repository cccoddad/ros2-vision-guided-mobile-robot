#include <chrono>

#include "gtest/gtest.h"

#include "base_driver/command_watchdog.hpp"

namespace
{

TEST(CommandWatchdog, StartsTimedOutUntilACommandArrives)
{
  const auto start = base_driver::CommandWatchdog::Clock::time_point{};
  base_driver::CommandWatchdog watchdog(std::chrono::milliseconds(300));

  EXPECT_TRUE(watchdog.timed_out(start));
  EXPECT_DOUBLE_EQ(watchdog.command_age_seconds(start), 1'000'000.0);
}

TEST(CommandWatchdog, TimesOutAfterTheConfiguredDuration)
{
  const auto start = base_driver::CommandWatchdog::Clock::time_point{};
  base_driver::CommandWatchdog watchdog(std::chrono::milliseconds(300));
  watchdog.record_command(start);

  EXPECT_FALSE(watchdog.timed_out(start + std::chrono::milliseconds(300)));
  EXPECT_TRUE(watchdog.timed_out(start + std::chrono::milliseconds(301)));
}

TEST(CommandWatchdog, ReportsCommandAge)
{
  const auto start = base_driver::CommandWatchdog::Clock::time_point{};
  base_driver::CommandWatchdog watchdog(std::chrono::milliseconds(300));
  watchdog.record_command(start);

  EXPECT_DOUBLE_EQ(
    watchdog.command_age_seconds(start + std::chrono::milliseconds(125)), 0.125);
}

}  // namespace
