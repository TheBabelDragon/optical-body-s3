#pragma once

#include <Arduino.h>

/**
 * OpticalFingerprint — the first physical memory of the body.
 *
 * {
 *   dark_frame[],
 *   laser_response_matrix[][],
 *   timestamp,
 *   geometry_version
 * }
 *
 * Written to FRAM after a successful calibration sequence.
 * On boot: load → compare current measurements → "body unchanged" or drift.
 */
struct OpticalFingerprint {
  static constexpr int MAX_LASERS    = 40;
  static constexpr int MAX_DETECTORS = 100;

  float dark_frame[MAX_DETECTORS];
  float matrix[MAX_LASERS][MAX_DETECTORS];  // already dark-corrected preferred

  uint8_t  num_lasers    = 0;
  uint8_t  num_detectors = 0;
  uint32_t timestamp_ms  = 0;
  char     geometry_version[32] = "uninitialized";
  bool     valid = false;

  void clear();
  void setDark(const float* d, uint8_t n);
  void setRow(uint8_t laser, const float* row, uint8_t n);
  float expected(uint8_t laser, uint8_t detector) const;
};
