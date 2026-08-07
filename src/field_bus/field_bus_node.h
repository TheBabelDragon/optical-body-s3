#pragma once

#include <Arduino.h>
#include "can_protocol.h"
#include "can_codec.h"

/**
 * FieldBusNode — common bus layer for every ESP32 on the Field Bus.
 *
 * Protocol is CAN-FD native (full 8-byte header + payload, up to 64 bytes).
 *
 * Transport:
 *   - Current bring-up path uses ESP32 TWAI (classic CAN, 8-byte limit).
 *   - Production path: external CAN-FD controller (MCP2518FD / TCAN4550 / …)
 *     over SPI. The pack/unpack API is already FD-sized; only the transmit
 *     path needs to change when the FD hardware is wired.
 *
 * Optical body uses node_id = FB_NODE_OPTICAL (0x02).
 * Lifecycle: begin() → sendHello() → periodic sendStatus() + poll().
 *
 * TWAI pins (bring-up only):
 *   -D FB_TWAI_TX_PIN=1  -D FB_TWAI_RX_PIN=2
 */
class FieldBusNode {
public:
  static constexpr size_t MAX_FRAME = 64;   // CAN-FD

  explicit FieldBusNode(uint8_t node_id = FB_NODE_OPTICAL);

  bool begin();
  void poll();

  bool sendHello(uint16_t firmware_ver, uint8_t capabilities);
  bool sendStatus(uint8_t state, uint16_t error_flags = 0);

  bool isOnline() const { return online_; }
  uint8_t nodeId() const { return node_id_; }
  uint32_t networkTimeUs() const { return network_time_us_; }
  bool hasTimeSync() const { return time_synced_; }

  void setState(uint8_t state) { state_ = state; }
  uint8_t state() const { return state_; }

private:
  uint8_t  node_id_;
  uint8_t  state_ = FB_STATE_BOOT;
  uint16_t seq_ = 0;
  bool     online_ = false;
  bool     time_synced_ = false;
  uint32_t network_time_us_ = 0;
  uint32_t last_heartbeat_ms_ = 0;
  uint16_t firmware_ver_ = 1;
  uint8_t  capabilities_ = 0;

  bool transmit(uint32_t can_id, const uint8_t *data, uint8_t len);
  void handleFrame(uint32_t id, const uint8_t *data, uint8_t len);
};
