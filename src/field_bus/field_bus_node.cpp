#include "field_bus_node.h"
#include "driver/twai.h"

#ifndef FB_TWAI_TX_PIN
#define FB_TWAI_TX_PIN 1
#endif
#ifndef FB_TWAI_RX_PIN
#define FB_TWAI_RX_PIN 2
#endif
#ifndef FB_HEARTBEAT_MS
#define FB_HEARTBEAT_MS 500
#endif

FieldBusNode::FieldBusNode(uint8_t node_id) : node_id_(node_id) {}

bool FieldBusNode::begin() {
  // ESP32 TWAI ↔ Waveshare SN65HVD230 (classic CAN, 500 kbit/s)
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)FB_TWAI_TX_PIN, (gpio_num_t)FB_TWAI_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config  = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config  = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    Serial.println(F("[FieldBus] TWAI install failed"));
    online_ = false;
    return true;   // non-fatal — serial path still works
  }
  if (twai_start() != ESP_OK) {
    Serial.println(F("[FieldBus] TWAI start failed — check SN65HVD230 wiring"));
    online_ = false;
    return true;
  }

  online_ = true;
  Serial.print(F("[FieldBus] online  node=0x"));
  Serial.print(node_id_, HEX);
  Serial.print(F("  classic CAN 500k  TX=GPIO"));
  Serial.print(FB_TWAI_TX_PIN);
  Serial.print(F(" RX=GPIO"));
  Serial.println(FB_TWAI_RX_PIN);
  return true;
}

bool FieldBusNode::transmit(uint32_t can_id, const uint8_t *data, uint8_t len) {
  if (!online_) return false;
  if (len > 8) len = 8;

  twai_message_t msg = {};
  msg.identifier = can_id;
  msg.extd = 1;   // 29-bit
  msg.data_length_code = len;
  memcpy(msg.data, data, len);

  return twai_transmit(&msg, pdMS_TO_TICKS(20)) == ESP_OK;
}

bool FieldBusNode::sendHello(uint16_t firmware_ver, uint8_t capabilities) {
  // 8-byte classic payload (type/src/dst already in CAN ID)
  uint8_t buf[8];
  buf[0] = node_id_;
  buf[1] = FB_PROTOCOL_VERSION;
  buf[2] = (uint8_t)(firmware_ver & 0xFF);
  buf[3] = (uint8_t)(firmware_ver >> 8);
  buf[4] = capabilities;
  buf[5] = 0;
  buf[6] = 0;
  buf[7] = 0;

  bool ok = transmit(fb_id_hello(node_id_), buf, 8);
  if (ok) {
    Serial.print(F("[FieldBus] NODE_HELLO  caps=0x"));
    Serial.println(capabilities, HEX);
  }
  return ok;
}

bool FieldBusNode::sendStatus(uint8_t state, uint16_t error_flags) {
  state_ = state;
  uint8_t buf[8];
  buf[0] = node_id_;
  buf[1] = state;
  buf[2] = (uint8_t)(error_flags & 0xFF);
  buf[3] = (uint8_t)(error_flags >> 8);
  uint32_t up = millis();
  buf[4] = (uint8_t)(up);
  buf[5] = (uint8_t)(up >> 8);
  buf[6] = (uint8_t)(up >> 16);
  buf[7] = (uint8_t)(up >> 24);

  return transmit(fb_id_status(node_id_), buf, 8);
}

void FieldBusNode::handleFrame(uint32_t id, const uint8_t *data, uint8_t len) {
  uint8_t type = fb_id_type(id);
  uint8_t dst  = fb_id_dst(id);

  if (dst != FB_NODE_BROADCAST && dst != node_id_) return;

  switch (type) {
    case FB_MSG_TIME_SYNC:
      if (len >= 6) {
        network_time_us_ = (uint32_t)data[0] |
                           ((uint32_t)data[1] << 8) |
                           ((uint32_t)data[2] << 16) |
                           ((uint32_t)data[3] << 24);
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
}

void FieldBusNode::poll() {
  if (!online_) return;

  twai_message_t rx;
  while (twai_receive(&rx, 0) == ESP_OK) {
    if (rx.extd) {
      handleFrame(rx.identifier, rx.data, rx.data_length_code);
    }
  }

  uint32_t now = millis();
  if (now - last_heartbeat_ms_ >= FB_HEARTBEAT_MS) {
    last_heartbeat_ms_ = now;
    sendStatus(state_);
  }
}
