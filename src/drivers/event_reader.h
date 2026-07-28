#pragma once

#include <Arduino.h>

/**
 * EventReader — LM393 fast event layer ("what changed?")
 *
 * Parallel to BPW34Reader (ADS1115 analog layer).
 * Not a replacement. Sparse: only high-value channels need comparators.
 *
 * Events are threshold crossings / edges, not intensity.
 */
class EventReader {
public:
  static constexpr int MAX_EVENT_CHANNELS = 32;

  bool begin();

  /**
   * Snapshot current event mask.
   * bit i set => channel i is above threshold (or edge latched).
   * Returns number of active channels written into active_ids (optional).
   */
  uint32_t readMask();
  int listActive(uint8_t* active_ids, int max_n);

  int numChannels() const { return num_channels_; }

private:
  int num_channels_ = 0;  // populated once pin map / MCP23017 is wired
  // TODO: GPIO pins or MCP23017 port mapping for LM393 outputs
};
