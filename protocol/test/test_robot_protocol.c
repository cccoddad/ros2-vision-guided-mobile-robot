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

static uint32_t next_deterministic_random(uint32_t * state)
{
  uint32_t value = *state;

  value ^= value << 13U;
  value ^= value >> 17U;
  value ^= value << 5U;
  *state = value;
  return value;
}

static void test_rejects_all_single_bit_frame_mutations(void)
{
  const robot_protocol_twist_command_t command = {100, -200};
  uint8_t wire[ROBOT_PROTOCOL_MAX_FRAME_SIZE] = {0};
  uint8_t mutated[ROBOT_PROTOCOL_MAX_FRAME_SIZE] = {0};
  size_t wire_length = 0U;
  size_t index;

  assert(robot_protocol_encode_set_twist(1U, &command, wire, sizeof(wire), &wire_length) ==
    ROBOT_PROTOCOL_OK);
  for (index = 0U; index < wire_length; ++index) {
    uint8_t bit;
    for (bit = 0U; bit < 8U; ++bit) {
      robot_protocol_frame_t frame = {0};
      memcpy(mutated, wire, wire_length);
      mutated[index] ^= (uint8_t)(1U << bit);
      assert(robot_protocol_decode(mutated, wire_length, &frame) != ROBOT_PROTOCOL_OK);
    }
  }
}

static void test_deterministic_fuzzed_frames_are_rejected_or_canonical(void)
{
  uint32_t random_state = 0xC0DEC0DEU;
  size_t iteration;

  for (iteration = 0U; iteration < 10000U; ++iteration) {
    uint8_t input[ROBOT_PROTOCOL_MAX_FRAME_SIZE + 1U] = {0};
    uint8_t reencoded[ROBOT_PROTOCOL_MAX_FRAME_SIZE] = {0};
    robot_protocol_frame_t frame = {0};
    size_t input_length = (size_t)(next_deterministic_random(&random_state) %
      (ROBOT_PROTOCOL_MAX_FRAME_SIZE + 1U));
    size_t reencoded_length = 0U;
    size_t index;
    robot_protocol_result_t decode_result;

    for (index = 0U; index < input_length; ++index) {
      input[index] = (uint8_t)next_deterministic_random(&random_state);
    }

    decode_result = robot_protocol_decode(input, input_length, &frame);
    if (decode_result == ROBOT_PROTOCOL_OK) {
      assert(robot_protocol_encode(&frame, reencoded, sizeof(reencoded), &reencoded_length) ==
        ROBOT_PROTOCOL_OK);
      assert(reencoded_length == input_length);
      assert(memcmp(reencoded, input, input_length) == 0);
    }
  }
}

int main(void)
{
  test_crc_reference_vector();
  test_set_twist_round_trip();
  test_rejects_corruption_and_bad_lengths();
  test_rejects_all_single_bit_frame_mutations();
  test_deterministic_fuzzed_frames_are_rejected_or_canonical();
  puts("PASS: robot protocol codec tests passed.");
  return 0;
}
