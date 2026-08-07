#pragma once

#include <Arduino.h>

/**
 * EventReader — LM393 fast event layer ("what changed?")
 *
 * Parallel to the ADS1115 analog path.
 * Supports up to 20 channels (extendable to 32).
 *
 * Default implementation uses one MCP23017 at 0x20 for the bulk of the
 * digital inputs. Extra channels can be added via direct GPIO later.
 *
 * Sparse is still fine — any channel left unconnected simply stays low.
 */
class EventReader {
public:
  static constexpr int MAX_EVENT_CHANNELS = 32;
  static constexpr int TARGET_CHANNELS    = 20;   // what we want for Phase 0.5

  bool begin();

  /** Snapshot current event mask (bit i = channel i active). */
  uint32_t readMask();

  /** Fill active_ids with the indices that are currently high. */
  int listActive(uint8_t* active_ids, int max_n);

  int numChannels() const { return num_channels_; }

private:
  int num_channels_ = 0;
  bool mcp_ok_ = false;
};
