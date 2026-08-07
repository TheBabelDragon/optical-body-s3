#include "event_reader.h"

#ifdef OPTICAL_USE_SYNTHETIC
  // no hardware
#else
  #include <Adafruit_MCP23X17.h>
  static Adafruit_MCP23X17 mcp;
#endif

// Default MCP23017 address (A0/A1/A2 tied to GND)
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
    return true;   // non-fatal
  }

  mcp_ok_ = true;
  // Configure first 16 pins as inputs with pull-ups (LM393 open-collector style)
  for (uint8_t p = 0; p < 16; ++p) {
    mcp.pinMode(p, INPUT_PULLUP);
  }

  // We target 20 channels; the remaining 4 can be direct GPIOs later.
  // For now expose the 16 MCP pins + note the headroom.
  num_channels_ = 16;   // practical limit with one MCP23017
  // If you wire a second MCP23017, raise this and add another instance.

  Serial.print(F("[Event] LM393 stream ready  channels="));
  Serial.print(num_channels_);
  Serial.print(F("  (MCP23017 @0x"));
  Serial.print(EVENT_MCP_ADDR, HEX);
  Serial.println(F(")"));
  Serial.println(F("[Event] note: 20-channel target — add 2nd MCP or GPIOs for the last 4"));
  return true;
#endif
}

uint32_t EventReader::readMask() {
#ifdef OPTICAL_USE_SYNTHETIC
  return 0;
#else
  if (!mcp_ok_ || num_channels_ == 0) return 0;

  uint32_t mask = 0;
  // MCP23017 active-low with pull-ups → invert so 1 = event
  for (uint8_t p = 0; p < 16 && p < num_channels_; ++p) {
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
