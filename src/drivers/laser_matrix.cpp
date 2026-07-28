#include "laser_matrix.h"

// TODO: replace with the real GPIO / MCP23017 pin map once the board is wired.
// For now we just print the intended action so the self-map sequence is visible.

bool LaserMatrix::begin() {
  // Example: pinMode(LASER_BASE_PIN + i, OUTPUT) or MCP23017 setup
  Serial.println(F("[Laser] matrix ready (pin map still TODO)"));
  allOff();
  return true;
}

void LaserMatrix::allOff() {
  // Drive every control line low / safe
  // digitalWrite(...) or mcp.digitalWrite(...)
}

void LaserMatrix::fire(uint16_t laser_id) {
  if (laser_id >= (uint16_t)num_lasers_) return;
  allOff();
  // digitalWrite(pin_for(laser_id), HIGH);
  // For visibility during early bring-up:
  Serial.print(F("[Laser] fire "));
  Serial.println(laser_id);
}
