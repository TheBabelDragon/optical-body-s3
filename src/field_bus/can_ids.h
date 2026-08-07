#pragma once

#include <stdint.h>
#include "can_message_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FB_PRIO_EMERGENCY   0
#define FB_PRIO_CONTROL     1
#define FB_PRIO_TIME_CONFIG 2
#define FB_PRIO_SENSOR_EVT  3
#define FB_PRIO_TELEMETRY   4
#define FB_PRIO_DISCOVERY   5
#define FB_PRIO_DEBUG       6

#define FB_NODE_HOST        0x01
#define FB_NODE_OPTICAL     0x02
#define FB_NODE_SENSOR      0x03
#define FB_NODE_ACTUATOR    0x04
#define FB_NODE_COMPUTE     0x05
#define FB_NODE_EXPANSION   0x06
#define FB_NODE_BROADCAST   0x00

static inline uint32_t fb_make_id(uint8_t prio, uint8_t type,
                                  uint8_t src, uint8_t dst)
{
    return ((uint32_t)(prio  & 0x07) << 26) |
           ((uint32_t)(type  & 0xFF) << 18) |
           ((uint32_t)(src   & 0xFF) << 10) |
           ((uint32_t)(dst   & 0xFF) <<  2);
}

static inline uint8_t fb_id_prio(uint32_t id)  { return (id >> 26) & 0x07; }
static inline uint8_t fb_id_type(uint32_t id)  { return (id >> 18) & 0xFF; }
static inline uint8_t fb_id_src (uint32_t id)  { return (id >> 10) & 0xFF; }
static inline uint8_t fb_id_dst (uint32_t id)  { return (id >>  2) & 0xFF; }

static inline uint32_t fb_id_hello(uint8_t src) {
    return fb_make_id(FB_PRIO_DISCOVERY, FB_MSG_NODE_HELLO, src, FB_NODE_BROADCAST);
}
static inline uint32_t fb_id_status(uint8_t src) {
    return fb_make_id(FB_PRIO_TELEMETRY, FB_MSG_NODE_STATUS, src, FB_NODE_BROADCAST);
}
static inline uint32_t fb_id_time_sync(uint8_t src) {
    return fb_make_id(FB_PRIO_TIME_CONFIG, FB_MSG_TIME_SYNC, src, FB_NODE_BROADCAST);
}

#ifdef __cplusplus
}
#endif
