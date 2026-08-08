#pragma once

#include <stdint.h>
#include "can_message_types.h"
#include "can_ids.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FB_PROTOCOL_VERSION   1

#define FB_CAP_SENSOR      (1u << 0)
#define FB_CAP_ADC         (1u << 1)
#define FB_CAP_GPIO        (1u << 2)
#define FB_CAP_ACTUATOR    (1u << 3)
#define FB_CAP_OPTICAL     (1u << 4)
#define FB_CAP_COMPUTE     (1u << 5)
#define FB_CAP_STORAGE     (1u << 6)
#define FB_CAP_TIME_MASTER (1u << 7)

#define FB_STATE_BOOT      0
#define FB_STATE_READY     1
#define FB_STATE_DEGRADED  2
#define FB_STATE_ERROR     3
#define FB_STATE_OFFLINE   4

typedef struct __attribute__((packed)) {
    uint8_t  version;
    uint8_t  type;
    uint8_t  source;
    uint8_t  target;
    uint16_t sequence;
    uint16_t length;
} FieldBusHeader;

typedef struct __attribute__((packed)) {
    uint8_t  node_id;
    uint8_t  protocol_ver;
    uint16_t firmware_ver;
    uint8_t  capabilities;
    uint8_t  reserved[3];
} FbNodeHello;

typedef struct __attribute__((packed)) {
    uint8_t  node_id;
    uint8_t  state;
    uint16_t error_flags;
    uint32_t uptime_ms;
    int16_t  temperature_c;
    uint16_t supply_mv;
} FbNodeStatus;

typedef struct __attribute__((packed)) {
    uint32_t network_time_us;
    uint16_t sync_sequence;
    uint16_t reserved;
} FbTimeSync;

typedef struct __attribute__((packed)) {
    uint16_t ref_sequence;
    uint8_t  result;
    uint8_t  reserved;
} FbAck;

#ifdef __cplusplus
}
#endif
