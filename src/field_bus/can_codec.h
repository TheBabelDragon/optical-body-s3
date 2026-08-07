#pragma once

#include <string.h>
#include "can_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline size_t fb_pack_header(uint8_t *buf, uint8_t type,
                                    uint8_t src, uint8_t dst,
                                    uint16_t seq, uint16_t payload_len)
{
    FieldBusHeader *h = (FieldBusHeader *)buf;
    h->version  = FB_PROTOCOL_VERSION;
    h->type     = type;
    h->source   = src;
    h->target   = dst;
    h->sequence = seq;
    h->length   = payload_len;
    return sizeof(FieldBusHeader);
}

static inline bool fb_unpack_header(const uint8_t *buf, size_t len,
                                    FieldBusHeader *out)
{
    if (len < sizeof(FieldBusHeader)) return false;
    memcpy(out, buf, sizeof(FieldBusHeader));
    return out->version == FB_PROTOCOL_VERSION;
}

static inline size_t fb_pack_hello(uint8_t *payload, uint8_t node_id,
                                   uint16_t fw_ver, uint8_t caps)
{
    FbNodeHello *p = (FbNodeHello *)payload;
    p->node_id       = node_id;
    p->protocol_ver  = FB_PROTOCOL_VERSION;
    p->firmware_ver  = fw_ver;
    p->capabilities  = caps;
    memset(p->reserved, 0, sizeof(p->reserved));
    return sizeof(FbNodeHello);
}

static inline size_t fb_pack_status(uint8_t *payload, uint8_t node_id,
                                    uint8_t state, uint16_t err,
                                    uint32_t uptime_ms,
                                    int16_t temp_c, uint16_t supply_mv)
{
    FbNodeStatus *p = (FbNodeStatus *)payload;
    p->node_id       = node_id;
    p->state         = state;
    p->error_flags   = err;
    p->uptime_ms     = uptime_ms;
    p->temperature_c = temp_c;
    p->supply_mv     = supply_mv;
    return sizeof(FbNodeStatus);
}

static inline size_t fb_pack_time_sync(uint8_t *payload,
                                       uint32_t network_time_us,
                                       uint16_t sync_seq)
{
    FbTimeSync *p = (FbTimeSync *)payload;
    p->network_time_us = network_time_us;
    p->sync_sequence   = sync_seq;
    p->reserved        = 0;
    return sizeof(FbTimeSync);
}

static inline size_t fb_pack_ack(uint8_t *payload, uint16_t ref_seq, uint8_t result)
{
    FbAck *p = (FbAck *)payload;
    p->ref_sequence = ref_seq;
    p->result       = result;
    p->reserved     = 0;
    return sizeof(FbAck);
}

#ifdef __cplusplus
}
#endif
