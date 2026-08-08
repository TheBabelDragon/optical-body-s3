#include "event_reader.h"

#ifdef OPTICAL_USE_SYNTHETIC
#else
  #include <Adafruit_MCP23X17.h>
  static Adafruit_MCP23X17 mcp;
#endif

#ifndef EVENT_MCP_ADDR
#define EVENT_MCP_ADDR 0x20
#endif

bool EventReader::begin() {
#ifdef OPTICAL_USE_SYNTHETIC
  num_channels_ = 0;
  Serial.println(F("[Event] synthetic — 0 channels"));
  return true;
#else
  if (!mcp.begin_I2C(EVENT_MCP_ADDR)) {
    Serial.println(F("[Event] MCP23017 not found — LM393 stream disabled (sparse OK)"));
    num_channels_ = 0;
    mcp_ok_ = false;
    return true;
  }

  mcp_ok_ = true;
  for (uint8_t p = 0; p < 16; ++p) {
    mcp.pinMode(p, INPUT_PULLUP);
  }
  num_channels_ = 16;

  Serial.print(F("[Event] LM393 stream ready  channels="));
  Serial.print(num_channels_);
  Serial.print(F("  (MCP23017 @0x"));
  Serial.print(EVENT_MCP_ADDR, HEX);
  Serial.println(F(")"));
  return true;
#endif
}

uint32_t EventReader::readMask() {
#ifdef OPTICAL_USE_SYNTHETIC
  return 0;
#else
  if (!mcp_ok_ || num_channels_ == 0) return 0;

  uint32_t mask = 0;
  for (uint8_t p = 0; p < 16 && p < (uint8_t)num_channels_; ++p) {
    if (mcp.digitalRead(p) == LOW) {
      mask |= (1u << p);
    }
  }
  return mask;
#endif
}

int EventReader::listActive(uint8_t* active_ids, int max_n) {
  if (!active_ids || max_n <= 0) return 0;
  uint32_t mask = readMask();
  int n = 0;
  for (int i = 0; i < 32 && n < max_n; ++i) {
    if (mask & (1u << i)) {
      active_ids[n++] = (uint8_t)i;
    }
  }
  return n;
}
