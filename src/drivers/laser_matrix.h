#pragma once

#include <Arduino.h>

/**
 * LaserMatrix
 *
 * Controls the laser bank via GPIO → buffer → MOSFET.
 * Phase 0: simple one-hot or single-laser fire.
 */
class LaserMatrix {
public:
  bool begin();

  /** Turn all lasers off. */
  void allOff();

  /**
   * Fire a single laser (0-based index).
   * Others are turned off.
   * Duration is left to the caller (hold high, then allOff).
   */
  void fire(uint16_t laser_id);

  int numLasers() const { return num_lasers_; }

private:
  int num_lasers_ = 12;
  // TODO: map laser_id → GPIO or MCP23017 pin
};
