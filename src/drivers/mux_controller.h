#pragma once

#include <Arduino.h>

/**
 * MuxController — CD74HC4067 select lines.
 * Select channel 0..15 before reading the corresponding ADS1115 input.
 */
class MuxController {
public:
  bool begin();
  void select(uint8_t channel);   // 0..15
private:
  // TODO: map S0..S3 to GPIO or MCP23017 pins
};
