#pragma once

/**
 * Field Bus — message type identifiers
 * Vendored from TheBabelDragon/field-bus
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FB_MSG_NODE_HELLO        0x01
#define FB_MSG_NODE_ANNOUNCE     0x02
#define FB_MSG_NODE_STATUS       0x03
#define FB_MSG_NODE_GOODBYE      0x04

#define FB_MSG_TIME_SYNC         0x10
#define FB_MSG_CONFIG_SET        0x11
#define FB_MSG_CONFIG_GET        0x12
#define FB_MSG_CONFIG_RESP       0x13

#define FB_MSG_ACTUATOR_SET      0x20
#define FB_MSG_GPIO_SET          0x21
#define FB_MSG_CALIBRATION_START 0x22
#define FB_MSG_EMERGENCY_STOP    0x2F

#define FB_MSG_ACK               0x30
#define FB_MSG_NACK              0x31
#define FB_MSG_ERROR             0x32

#define FB_MSG_SENSOR_DATA       0x40
#define FB_MSG_FIELD_OBSERVATION 0x41
#define FB_MSG_TELEMETRY         0x42

#define FB_MSG_SENSOR_EVENT      0x50

#define FB_MSG_DEBUG             0xF0

#ifdef __cplusplus
}
#endif
