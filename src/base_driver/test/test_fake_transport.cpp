#include <cstdint>

#include "gtest/gtest.h"

#include "base_driver/fake_transport.hpp"

namespace
{

TEST(FakeTransport, StartsWithNoReceivedFrame)
{
  base_driver::FakeTransport transport;

  EXPECT_FALSE(transport.read().has_value());
}

TEST(FakeTransport, ReceivesFramesInFirstInFirstOutOrder)
{
  base_driver::FakeTransport transport;
  transport.push_received({0xAAU, 0x55U, 0x01U});
  transport.push_received({0xAAU, 0x55U, 0x02U});

  const auto first = transport.read();
  const auto second = transport.read();

  ASSERT_TRUE(first.has_value());
  ASSERT_TRUE(second.has_value());
  EXPECT_EQ(*first, (base_driver::Transport::Frame{0xAAU, 0x55U, 0x01U}));
  EXPECT_EQ(*second, (base_driver::Transport::Frame{0xAAU, 0x55U, 0x02U}));
  EXPECT_FALSE(transport.read().has_value());
}

TEST(FakeTransport, CapturesWrittenFrames)
{
  base_driver::FakeTransport transport;

  EXPECT_TRUE(transport.write({0xAAU, 0x55U, 0x04U}));
  const auto written = transport.take_written();

  ASSERT_TRUE(written.has_value());
  EXPECT_EQ(*written, (base_driver::Transport::Frame{0xAAU, 0x55U, 0x04U}));
  EXPECT_FALSE(transport.take_written().has_value());
}

}  // namespace
