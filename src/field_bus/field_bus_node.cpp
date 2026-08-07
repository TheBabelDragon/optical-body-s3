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
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
      (gpio_num_t)FB_TWAI_TX_PIN, (gpio_num_t)FB_TWAI_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config  = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config  = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
    Serial.println(F("[FieldBus] TWAI driver install failed"));
    online_ = false;
    return true;   // non-fatal
  }
  if (twai_start() != ESP_OK) {
    Serial.println(F("[FieldBus] TWAI start failed — no transceiver?"));
    online_ = false;
    return true;   // non-fatal
  }

  online_ = true;
  Serial.print(F("[FieldBus] online  node=0x"));
  Serial.print(node_id_, HEX);
  Serial.print(F("  TX=GPIO"));
  Serial.print(FB_TWAI_TX_PIN);
  Serial.print(F(" RX=GPIO"));
  Serial.println(FB_TWAI_RX_PIN);
  return true;
}

bool FieldBusNode::transmit(uint32_t can_id, const uint8_t *data, uint8_t len) {
  if (!online_) return false;
  if (len > 8) len = 8;   // classic CAN for now

  twai_message_t msg = {};
  msg.identifier = can_id;
  msg.extd = 1;             // 29-bit
  msg.data_length_code = len;
  memcpy(msg.data, data, len);

  esp_err_t err = twai_transmit(&msg, pdMS_TO_TICKS(20));
  return err == ESP_OK;
}

bool FieldBusNode::sendHello(uint16_t firmware_ver, uint8_t capabilities) {
  firmware_ver_ = firmware_ver;
  capabilities_ = capabilities;

  uint8_t buf[16];
  size_t hdr = fb_pack_header(buf, FB_MSG_NODE_HELLO,
                              node_id_, FB_NODE_BROADCAST,
                              seq_++, sizeof(FbNodeHello));
  size_t pl  = fb_pack_hello(buf + hdr, node_id_, firmware_ver, capabilities);

  bool ok = transmit(fb_id_hello(node_id_), buf, (uint8_t)(hdr + pl));
  if (ok) {
    Serial.print(F("[FieldBus] NODE_HELLO sent  caps=0x"));
    Serial.println(capabilities, HEX);
  }
  return ok;
}

bool FieldBusNode::sendStatus(uint8_t state, uint16_t error_flags) {
  state_ = state;
  uint8_t buf[16];
  size_t hdr = fb_pack_header(buf, FB_MSG_NODE_STATUS,
                              node_id_, FB_NODE_BROADCAST,
                              seq_++, sizeof(FbNodeStatus));
  size_t pl  = fb_pack_status(buf + hdr, node_id_, state, error_flags,
                              millis(), 0, 0);

  return transmit(fb_id_status(node_id_), buf, (uint8_t)(hdr + pl));
}

void FieldBusNode::handleFrame(uint32_t id, const uint8_t *data, uint8_t len) {
  FieldBusHeader hdr;
  if (!fb_unpack_header(data, len, &hdr)) return;

  // Ignore frames not for us (unless broadcast)
  if (hdr.target != FB_NODE_BROADCAST && hdr.target != node_id_) return;

  switch (hdr.type) {
    case FB_MSG_TIME_SYNC:
      if (len >= sizeof(FieldBusHeader) + sizeof(FbTimeSync)) {
        const FbTimeSync *ts = (const FbTimeSync *)(data + sizeof(FieldBusHeader));
        network_time_us_ = ts->network_time_us;
        time_synced_ = true;
        Serial.print(F("[FieldBus] TIME_SYNC  t="));
        Serial.println(network_time_us_);
      }
      break;

    case FB_MSG_EMERGENCY_STOP:
      Serial.println(F("[FieldBus] EMERGENCY_STOP received"));
      state_ = FB_STATE_DEGRADED;
      // Application layer should also react (lasers off, etc.)
      break;

    default:
      break;
  }
}

void FieldBusNode::poll() {
  if (!online_) return;

  // Drain RX queue
  twai_message_t rx;
  while (twai_receive(&rx, 0) == ESP_OK) {
    if (rx.extd) {
      handleFrame(rx.identifier, rx.data, rx.data_length_code);
    }
  }

  // Heartbeat
  uint32_t now = millis();
  if (now - last_heartbeat_ms_ >= FB_HEARTBEAT_MS) {
    last_heartbeat_ms_ = now;
    sendStatus(state_);
  }
}
