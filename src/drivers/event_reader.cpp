#include "event_reader.h"

bool EventReader::begin() {
  // Until LM393 outputs are wired, report zero event channels.
  // Analog path (ADS1115) remains the sole characterization stream.
  num_channels_ = 0;
  Serial.println(F("[Event] LM393 stream present (0 channels wired — sparse OK)"));
  return true;  // non-fatal
}

uint32_t EventReader::readMask() {
  // TODO: read GPIO bank or MCP23017 for LM393 digital outputs
  return 0;
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
