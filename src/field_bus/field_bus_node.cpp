#include "field_bus_node.h"

/*
 * Transport: MCP2518FD over SPI (CAN-FD).
 *
 * When the modules arrive, wire the driver (e.g. ACAN2517FD which
 * supports MCP2518FD) in begin()/transmit()/poll(). Until then this
 * file packs correct FD frames and stays offline if the controller
 * is not present — serial path still works.
 *
 * Nominal 500 kbit/s, data phase 2 Mbit/s (raise later).
 */

#ifndef FB_SPI_SCK
#define FB_SPI_SCK  12
#endif
#ifndef FB_SPI_MOSI
#define FB_SPI_MOSI 11
#endif
#ifndef FB_SPI_MISO
#define FB_SPI_MISO 13
#endif
#ifndef FB_SPI_CS
#define FB_SPI_CS   10
#endif
#ifndef FB_SPI_INT
#define FB_SPI_INT  14
#endif
#ifndef FB_MCP_OSC
#define FB_MCP_OSC  20000000UL
#endif
#ifndef FB_HEARTBEAT_MS
#define FB_HEARTBEAT_MS 500
#endif

// Uncomment when MCP2518FD library is added and boards are wired:
// #define FB_USE_MCP2518FD 1

#if defined(FB_USE_MCP2518FD)
  #include <SPI.h>
  #include <ACAN2517FD.h>
  static ACAN2517FD can(FB_SPI_CS, SPI, FB_SPI_INT);
#endif

FieldBusNode::FieldBusNode(uint8_t node_id) : node_id_(node_id) {}

bool FieldBusNode::begin() {
#if defined(FB_USE_MCP2518FD)
  SPI.begin(FB_SPI_SCK, FB_SPI_MISO, FB_SPI_MOSI, FB_SPI_CS);

  ACAN2517FDSettings settings(
      ACAN2517FDSettings::OSC_20MHz,   // match FB_MCP_OSC / module crystal
      500 * 1000UL,                    // arbitration
      DataBitRateFactor::x4);          // 2 Mbit/s data phase

  const uint32_t err = can.begin(settings, [] { can.isr(); });
  if (err != 0) {
    Serial.print(F("[FieldBus] MCP2518FD begin error 0x"));
    Serial.println(err, HEX);
    online_ = false;
    return true;   // non-fatal
  }

  online_ = true;
  Serial.print(F("[FieldBus] online  CAN-FD  MCP2518FD  node=0x"));
  Serial.println(node_id_, HEX);
  Serial.print(F("[FieldBus] SPI CS=GPIO"));
  Serial.print(FB_SPI_CS);
  Serial.print(F(" INT=GPIO"));
  Serial.println(FB_SPI_INT);
#else
  // Boards ordered — enable FB_USE_MCP2518FD in platformio.ini when wired
  online_ = false;
  Serial.println(F("[FieldBus] MCP2518FD path ready (define FB_USE_MCP2518FD when hardware is on the desk)"));
  Serial.print(F("[FieldBus] planned SPI  SCK="));
  Serial.print(FB_SPI_SCK);
  Serial.print(F(" MOSI="));
  Serial.print(FB_SPI_MOSI);
  Serial.print(F(" MISO="));
  Serial.print(FB_SPI_MISO);
  Serial.print(F(" CS="));
  Serial.print(FB_SPI_CS);
  Serial.print(F(" INT="));
  Serial.println(FB_SPI_INT);
#endif
  return true;
}

bool FieldBusNode::transmit(uint32_t can_id, const uint8_t *data, uint8_t len) {
  if (!online_) return false;
  if (len > MAX_FRAME) len = MAX_FRAME;

#if defined(FB_USE_MCP2518FD)
  CANFDMessage msg;
  msg.id = can_id;
  msg.ext = true;
  msg.len = len;
  memcpy(msg.data, data, len);
  return can.tryToSend(msg);
#else
  (void)can_id;
  (void)data;
  (void)len;
  return false;
#endif
}

bool FieldBusNode::sendHello(uint16_t firmware_ver, uint8_t capabilities) {
  uint8_t buf[MAX_FRAME];
  size_t hdr = fb_pack_header(buf, FB_MSG_NODE_HELLO,
                              node_id_, FB_NODE_BROADCAST,
                              seq_++, sizeof(FbNodeHello));
  size_t pl  = fb_pack_hello(buf + hdr, node_id_, firmware_ver, capabilities);
  size_t total = hdr + pl;

  bool ok = transmit(fb_id_hello(node_id_), buf, (uint8_t)total);
  if (ok || !online_) {
    Serial.print(F("[FieldBus] NODE_HELLO  caps=0x"));
    Serial.print(capabilities, HEX);
    if (!online_) Serial.print(F(" (queued offline)"));
    Serial.println();
  }
  return ok;
}

bool FieldBusNode::sendStatus(uint8_t state, uint16_t error_flags) {
  state_ = state;
  uint8_t buf[MAX_FRAME];
  size_t hdr = fb_pack_header(buf, FB_MSG_NODE_STATUS,
                              node_id_, FB_NODE_BROADCAST,
                              seq_++, sizeof(FbNodeStatus));
  size_t pl  = fb_pack_status(buf + hdr, node_id_, state, error_flags,
                              millis(), 0, 0);
  return transmit(fb_id_status(node_id_), buf, (uint8_t)(hdr + pl));
}

void FieldBusNode::handleFrame(uint32_t id, const uint8_t *data, uint8_t len) {
  FieldBusHeader hdr;
  if (len >= sizeof(FieldBusHeader) && fb_unpack_header(data, len, &hdr)) {
    if (hdr.target != FB_NODE_BROADCAST && hdr.target != node_id_) return;

    switch (hdr.type) {
      case FB_MSG_TIME_SYNC:
        if (len >= sizeof(FieldBusHeader) + sizeof(FbTimeSync)) {
          const FbTimeSync *ts =
              (const FbTimeSync *)(data + sizeof(FieldBusHeader));
          network_time_us_ = ts->network_time_us;
          time_synced_ = true;
          Serial.print(F("[FieldBus] TIME_SYNC  t="));
          Serial.println(network_time_us_);
        }
        break;
      case FB_MSG_EMERGENCY_STOP:
        Serial.println(F("[FieldBus] EMERGENCY_STOP"));
        state_ = FB_STATE_DEGRADED;
        break;
      default:
        break;
    }
    return;
  }

  // ID-only fallback
  uint8_t type = fb_id_type(id);
  uint8_t dst  = fb_id_dst(id);
  if (dst != FB_NODE_BROADCAST && dst != node_id_) return;
  if (type == FB_MSG_EMERGENCY_STOP) state_ = FB_STATE_DEGRADED;
}

void FieldBusNode::poll() {
#if defined(FB_USE_MCP2518FD)
  if (!online_) return;

  CANFDMessage msg;
  while (can.receive(msg)) {
    if (msg.ext) {
      handleFrame(msg.id, msg.data, msg.len);
    }
  }

  uint32_t now = millis();
  if (now - last_heartbeat_ms_ >= FB_HEARTBEAT_MS) {
    last_heartbeat_ms_ = now;
    sendStatus(state_);
  }
#else
  // No controller yet — nothing to poll
  (void)0;
#endif
}
