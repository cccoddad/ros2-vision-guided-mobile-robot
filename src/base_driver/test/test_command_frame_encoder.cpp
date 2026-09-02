#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

#include "gtest/gtest.h"

#include "robot_protocol.h"

#include "base_driver/command_frame_encoder.hpp"

namespace
{

robot_protocol_twist_command_t decode_twist(const base_driver::Transport::Frame & encoded)
{
  robot_protocol_frame_t frame = {};
  robot_protocol_twist_command_t command = {};
  EXPECT_EQ(robot_protocol_decode(encoded.data(), encoded.size(), &frame), ROBOT_PROTOCOL_OK);
  EXPECT_EQ(frame.message_type, ROBOT_PROTOCOL_MESSAGE_SET_TWIST);
  EXPECT_EQ(robot_protocol_decode_set_twist(&frame, &command), ROBOT_PROTOCOL_OK);
  return command;
}

TEST(CommandFrameEncoder, ConvertsRosUnitsAndPreservesSequence)
{
  base_driver::CommandFrameEncoder encoder(0.20, 0.80);

  const auto encoded = encoder.encode(0.1234, -0.4567, 251U);

  ASSERT_TRUE(encoded.has_value());
  const auto command = decode_twist(*encoded);
  EXPECT_EQ(command.linear_speed_mmps, 123);
  EXPECT_EQ(command.angular_speed_mradps, -457);
  EXPECT_EQ((*encoded)[4], 251U);
}

TEST(CommandFrameEncoder, ClampsCommandsToConfiguredLimits)
{
  base_driver::CommandFrameEncoder encoder(0.20, 0.80);

  const auto encoded = encoder.encode(3.0, -2.0, 3U);

  ASSERT_TRUE(encoded.has_value());
  const auto command = decode_twist(*encoded);
  EXPECT_EQ(command.linear_speed_mmps, 200);
  EXPECT_EQ(command.angular_speed_mradps, -800);
}

TEST(CommandFrameEncoder, RejectsNonFiniteCommands)
{
  base_driver::CommandFrameEncoder encoder(0.20, 0.80);

  EXPECT_FALSE(encoder.encode(std::numeric_limits<double>::quiet_NaN(), 0.0, 0U).has_value());
  EXPECT_FALSE(encoder.encode(0.0, std::numeric_limits<double>::infinity(), 0U).has_value());
}

TEST(CommandFrameEncoder, RejectsInvalidLimits)
{
  EXPECT_THROW(base_driver::CommandFrameEncoder(0.0, 0.80), std::invalid_argument);
  EXPECT_THROW(base_driver::CommandFrameEncoder(0.20, 40.0), std::invalid_argument);
}

}  // namespace
