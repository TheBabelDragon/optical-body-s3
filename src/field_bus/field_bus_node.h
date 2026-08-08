#pragma once

#include <Arduino.h>
#include "can_protocol.h"
#include "can_codec.h"

/**
 * FieldBusNode — CAN-FD bus layer for every ESP32 on the Field Bus.
 *
 * Hardware target:
 *   ESP32-S3  --SPI-->  MCP2518FD module  -->  CANH/CANL
 *   Host PC   -------->  DSD TECH SH-C31G (CAN-FD mode)
 *
 * Optical node ID = 0x02.
 *
 * SPI pins (override to match your MCP2518FD breakout):
 *   -D FB_SPI_SCK=12
 *   -D FB_SPI_MOSI=11
 *   -D FB_SPI_MISO=13
 *   -D FB_SPI_CS=10
 *   -D FB_SPI_INT=14
 *
 * Crystal on the module (Hz):
 *   -D FB_MCP_OSC=20000000
 */
class FieldBusNode {
public:
  static constexpr size_t MAX_FRAME = 64;

  explicit FieldBusNode(uint8_t node_id = FB_NODE_OPTICAL);

  bool begin();
  void poll();   // RX + 500 ms heartbeat

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

  bool transmit(uint32_t can_id, const uint8_t *data, uint8_t len);
  void handleFrame(uint32_t id, const uint8_t *data, uint8_t len);
};
