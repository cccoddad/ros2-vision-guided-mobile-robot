#include "robot_protocol.h"

enum {
  VERSION_OFFSET = 2,
  MESSAGE_TYPE_OFFSET = 3,
  SEQUENCE_OFFSET = 4,
  PAYLOAD_LENGTH_OFFSET = 5,
  PAYLOAD_OFFSET = 6,
};

static uint16_t read_u16_le(const uint8_t * input)
{
  return (uint16_t)input[0] | ((uint16_t)input[1] << 8);
}

static void write_u16_le(uint8_t * output, uint16_t value)
{
  output[0] = (uint8_t)(value & 0xFFU);
  output[1] = (uint8_t)(value >> 8);
}

uint16_t robot_protocol_crc16_ccitt(const uint8_t * data, size_t length)
{
  uint16_t crc = 0xFFFFU;
  size_t index;

  if (data == NULL && length != 0U) {
    return 0U;
  }

  for (index = 0U; index < length; ++index) {
    uint8_t bit;
    crc ^= (uint16_t)data[index] << 8;
    for (bit = 0U; bit < 8U; ++bit) {
      crc = (crc & 0x8000U) != 0U ? (uint16_t)((crc << 1) ^ 0x1021U) : (uint16_t)(crc << 1);
    }
  }
  return crc;
}

robot_protocol_result_t robot_protocol_encode(
  const robot_protocol_frame_t * frame,
  uint8_t * output,
  size_t output_capacity,
  size_t * output_length)
{
  size_t frame_length;
  uint16_t crc;
  uint8_t index;

  if (frame == NULL || output == NULL || output_length == NULL) {
    return ROBOT_PROTOCOL_INVALID_ARGUMENT;
  }
  if (frame->payload_length > ROBOT_PROTOCOL_MAX_PAYLOAD_SIZE) {
    return ROBOT_PROTOCOL_INVALID_LENGTH;
  }

  frame_length = ROBOT_PROTOCOL_MIN_FRAME_SIZE + (size_t)frame->payload_length;
  if (output_capacity < frame_length) {
    return ROBOT_PROTOCOL_BUFFER_TOO_SMALL;
  }

  output[0] = ROBOT_PROTOCOL_SOF0;
  output[1] = ROBOT_PROTOCOL_SOF1;
  output[VERSION_OFFSET] = ROBOT_PROTOCOL_VERSION;
  output[MESSAGE_TYPE_OFFSET] = frame->message_type;
  output[SEQUENCE_OFFSET] = frame->sequence;
  output[PAYLOAD_LENGTH_OFFSET] = frame->payload_length;
  for (index = 0U; index < frame->payload_length; ++index) {
    output[PAYLOAD_OFFSET + index] = frame->payload[index];
  }

  crc = robot_protocol_crc16_ccitt(&output[VERSION_OFFSET], 4U + frame->payload_length);
  write_u16_le(&output[PAYLOAD_OFFSET + frame->payload_length], crc);
  *output_length = frame_length;
  return ROBOT_PROTOCOL_OK;
}

robot_protocol_result_t robot_protocol_decode(
  const uint8_t * input,
  size_t input_length,
  robot_protocol_frame_t * frame)
{
  size_t expected_length;
  uint16_t received_crc;
  uint16_t computed_crc;
  uint8_t index;
  uint8_t payload_length;

  if (input == NULL || frame == NULL) {
    return ROBOT_PROTOCOL_INVALID_ARGUMENT;
  }
  if (input_length < ROBOT_PROTOCOL_MIN_FRAME_SIZE) {
    return ROBOT_PROTOCOL_INVALID_LENGTH;
  }
  if (input[0] != ROBOT_PROTOCOL_SOF0 || input[1] != ROBOT_PROTOCOL_SOF1) {
    return ROBOT_PROTOCOL_INVALID_SOF;
  }
  if (input[VERSION_OFFSET] != ROBOT_PROTOCOL_VERSION) {
    return ROBOT_PROTOCOL_UNSUPPORTED_VERSION;
  }

  payload_length = input[PAYLOAD_LENGTH_OFFSET];
  if (payload_length > ROBOT_PROTOCOL_MAX_PAYLOAD_SIZE) {
    return ROBOT_PROTOCOL_INVALID_LENGTH;
  }
  expected_length = ROBOT_PROTOCOL_MIN_FRAME_SIZE + (size_t)payload_length;
  if (input_length != expected_length) {
    return ROBOT_PROTOCOL_INVALID_LENGTH;
  }

  received_crc = read_u16_le(&input[PAYLOAD_OFFSET + payload_length]);
  computed_crc = robot_protocol_crc16_ccitt(&input[VERSION_OFFSET], 4U + payload_length);
  if (received_crc != computed_crc) {
    return ROBOT_PROTOCOL_CRC_MISMATCH;
  }

  frame->message_type = input[MESSAGE_TYPE_OFFSET];
  frame->sequence = input[SEQUENCE_OFFSET];
  frame->payload_length = payload_length;
  for (index = 0U; index < payload_length; ++index) {
    frame->payload[index] = input[PAYLOAD_OFFSET + index];
  }
  return ROBOT_PROTOCOL_OK;
}

robot_protocol_result_t robot_protocol_encode_set_twist(
  uint8_t sequence,
  const robot_protocol_twist_command_t * command,
  uint8_t * output,
  size_t output_capacity,
  size_t * output_length)
{
  robot_protocol_frame_t frame = {0};

  if (command == NULL) {
    return ROBOT_PROTOCOL_INVALID_ARGUMENT;
  }

  frame.message_type = ROBOT_PROTOCOL_MESSAGE_SET_TWIST;
  frame.sequence = sequence;
  frame.payload_length = 4U;
  write_u16_le(&frame.payload[0], (uint16_t)command->linear_speed_mmps);
  write_u16_le(&frame.payload[2], (uint16_t)command->angular_speed_mradps);
  return robot_protocol_encode(&frame, output, output_capacity, output_length);
}

robot_protocol_result_t robot_protocol_decode_set_twist(
  const robot_protocol_frame_t * frame,
  robot_protocol_twist_command_t * command)
{
  if (frame == NULL || command == NULL) {
    return ROBOT_PROTOCOL_INVALID_ARGUMENT;
  }
  if (frame->message_type != ROBOT_PROTOCOL_MESSAGE_SET_TWIST) {
    return ROBOT_PROTOCOL_UNEXPECTED_MESSAGE_TYPE;
  }
  if (frame->payload_length != 4U) {
    return ROBOT_PROTOCOL_INVALID_LENGTH;
  }

  command->linear_speed_mmps = (int16_t)read_u16_le(&frame->payload[0]);
  command->angular_speed_mradps = (int16_t)read_u16_le(&frame->payload[2]);
  return ROBOT_PROTOCOL_OK;
}
