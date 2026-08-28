#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "robot_protocol.h"

static void test_crc_reference_vector(void)
{
  static const uint8_t input[] = "123456789";
  assert(robot_protocol_crc16_ccitt(input, sizeof(input) - 1U) == 0x29B1U);
}

static void test_set_twist_round_trip(void)
{
  const robot_protocol_twist_command_t sent = {-120, 314};
  robot_protocol_twist_command_t received = {0};
  robot_protocol_frame_t frame = {0};
  uint8_t wire[ROBOT_PROTOCOL_MAX_FRAME_SIZE] = {0};
  size_t wire_length = 0U;

  assert(robot_protocol_encode_set_twist(17U, &sent, wire, sizeof(wire), &wire_length) ==
    ROBOT_PROTOCOL_OK);
  assert(wire_length == 12U);
  assert(wire[0] == ROBOT_PROTOCOL_SOF0 && wire[1] == ROBOT_PROTOCOL_SOF1);
  assert(wire[2] == ROBOT_PROTOCOL_VERSION);
  assert(wire[3] == ROBOT_PROTOCOL_MESSAGE_SET_TWIST);
  assert(wire[4] == 17U && wire[5] == 4U);
  assert(robot_protocol_decode(wire, wire_length, &frame) == ROBOT_PROTOCOL_OK);
  assert(robot_protocol_decode_set_twist(&frame, &received) == ROBOT_PROTOCOL_OK);
  assert(received.linear_speed_mmps == sent.linear_speed_mmps);
  assert(received.angular_speed_mradps == sent.angular_speed_mradps);
}

static void test_rejects_corruption_and_bad_lengths(void)
{
  const robot_protocol_twist_command_t command = {100, -200};
  uint8_t wire[ROBOT_PROTOCOL_MAX_FRAME_SIZE] = {0};
  size_t wire_length = 0U;
  robot_protocol_frame_t frame = {0};

  assert(robot_protocol_encode_set_twist(1U, &command, wire, sizeof(wire), &wire_length) ==
    ROBOT_PROTOCOL_OK);
  wire[6] ^= 0x01U;
  assert(robot_protocol_decode(wire, wire_length, &frame) == ROBOT_PROTOCOL_CRC_MISMATCH);
  wire[6] ^= 0x01U;
  assert(robot_protocol_decode(wire, wire_length - 1U, &frame) == ROBOT_PROTOCOL_INVALID_LENGTH);
  wire[2] = ROBOT_PROTOCOL_VERSION + 1U;
  assert(robot_protocol_decode(wire, wire_length, &frame) == ROBOT_PROTOCOL_UNSUPPORTED_VERSION);
}

int main(void)
{
  test_crc_reference_vector();
  test_set_twist_round_trip();
  test_rejects_corruption_and_bad_lengths();
  puts("PASS: robot protocol codec tests passed.");
  return 0;
}
