#ifndef ROBOT_PROTOCOL_H
#define ROBOT_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Wire layout: SOF[2], version, message type, sequence, payload length, payload, CRC16-CCITT LE. */
#define ROBOT_PROTOCOL_SOF0 ((uint8_t)0xAA)
#define ROBOT_PROTOCOL_SOF1 ((uint8_t)0x55)
#define ROBOT_PROTOCOL_VERSION ((uint8_t)1)
#define ROBOT_PROTOCOL_MAX_PAYLOAD_SIZE ((uint8_t)48)
#define ROBOT_PROTOCOL_HEADER_SIZE ((size_t)6)
#define ROBOT_PROTOCOL_CRC_SIZE ((size_t)2)
#define ROBOT_PROTOCOL_MIN_FRAME_SIZE (ROBOT_PROTOCOL_HEADER_SIZE + ROBOT_PROTOCOL_CRC_SIZE)
#define ROBOT_PROTOCOL_MAX_FRAME_SIZE \
  (ROBOT_PROTOCOL_MIN_FRAME_SIZE + (size_t)ROBOT_PROTOCOL_MAX_PAYLOAD_SIZE)

typedef enum robot_protocol_message_type {
  ROBOT_PROTOCOL_MESSAGE_SET_TWIST = 0x01,
  ROBOT_PROTOCOL_MESSAGE_ESTOP = 0x02,
  ROBOT_PROTOCOL_MESSAGE_CLEAR_FAULT = 0x03,
  ROBOT_PROTOCOL_MESSAGE_PING = 0x04,
  ROBOT_PROTOCOL_MESSAGE_BASE_STATUS = 0x80,
} robot_protocol_message_type_t;

typedef enum robot_protocol_result {
  ROBOT_PROTOCOL_OK = 0,
  ROBOT_PROTOCOL_INVALID_ARGUMENT,
  ROBOT_PROTOCOL_BUFFER_TOO_SMALL,
  ROBOT_PROTOCOL_INVALID_SOF,
  ROBOT_PROTOCOL_UNSUPPORTED_VERSION,
  ROBOT_PROTOCOL_INVALID_LENGTH,
  ROBOT_PROTOCOL_CRC_MISMATCH,
  ROBOT_PROTOCOL_UNEXPECTED_MESSAGE_TYPE,
} robot_protocol_result_t;

typedef struct robot_protocol_frame {
  uint8_t message_type;
  uint8_t sequence;
  uint8_t payload_length;
  uint8_t payload[ROBOT_PROTOCOL_MAX_PAYLOAD_SIZE];
} robot_protocol_frame_t;

typedef struct robot_protocol_twist_command {
  int16_t linear_speed_mmps;
  int16_t angular_speed_mradps;
} robot_protocol_twist_command_t;

uint16_t robot_protocol_crc16_ccitt(const uint8_t * data, size_t length);

robot_protocol_result_t robot_protocol_encode(
  const robot_protocol_frame_t * frame,
  uint8_t * output,
  size_t output_capacity,
  size_t * output_length);

robot_protocol_result_t robot_protocol_decode(
  const uint8_t * input,
  size_t input_length,
  robot_protocol_frame_t * frame);

robot_protocol_result_t robot_protocol_encode_set_twist(
  uint8_t sequence,
  const robot_protocol_twist_command_t * command,
  uint8_t * output,
  size_t output_capacity,
  size_t * output_length);

robot_protocol_result_t robot_protocol_decode_set_twist(
  const robot_protocol_frame_t * frame,
  robot_protocol_twist_command_t * command);

#ifdef __cplusplus
}
#endif

#endif  /* ROBOT_PROTOCOL_H */
