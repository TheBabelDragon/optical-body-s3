#pragma once

#include <Arduino.h>

/**
 * LaserMatrix
 *
 * Controls the laser bank via GPIO (or later MCP23017).
 *
 * Phase 0: simple one-hot fire.
 * Define the actual pins with build flags, e.g.:
 *
 *   -D LASER_PIN_0=10 -D LASER_PIN_1=11 ... etc.
 *
 * Any pin left at -1 is treated as "not wired yet" and only logged.
 */
class LaserMatrix {
public:
  bool begin();

  /** Turn all lasers off (safe state). */
  void allOff();

  /**
   * Fire a single laser (0-based index).
   * Others are turned off first.
   */
  void fire(uint16_t laser_id);

  int numLasers() const { return num_lasers_; }

private:
  static constexpr int MAX_LASERS = 16;
  int num_lasers_ = 12;

  // Pin map – override with -D LASER_PIN_n=xx
  // Default -1 means "not wired, just log"
  int pins_[MAX_LASERS];

  void initPinMap();
};
