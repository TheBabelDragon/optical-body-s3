#pragma once

#include <Arduino.h>

/**
 * EventReader — LM393 fast event layer
 *
 * Up to 16 channels via MCP23017 @ 0x20 (Phase-0 address).
 * Target 20; extra 4 via second MCP or GPIO later.
 * Sparse OK if MCP missing.
 */
class EventReader {
public:
  static constexpr int MAX_EVENT_CHANNELS = 32;
  static constexpr int TARGET_CHANNELS    = 20;

  bool begin();
  uint32_t readMask();
  int listActive(uint8_t* active_ids, int max_n);
  int numChannels() const { return num_channels_; }

private:
  int  num_channels_ = 0;
  bool mcp_ok_ = false;
};
