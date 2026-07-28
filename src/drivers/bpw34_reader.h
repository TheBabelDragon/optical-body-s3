#pragma once

#include <Arduino.h>

/**
 * BPW34Reader
 *
 * Reads the detector bank.
 * Default path: real values via ADS1115 + mux.
 * Optional synthetic path only when OPTICAL_USE_SYNTHETIC is defined
 * (board bring-up without detectors wired).
 */
class BPW34Reader {
public:
  bool begin();

  /**
   * Read all detectors into out[] (size == numDetectors()).
   * Values are normalized roughly 0..1 (dark → illuminated).
   * Returns true on success.
   */
  bool readAll(float* out, size_t max_n);

  int numDetectors() const { return num_detectors_; }

private:
  int num_detectors_ = 20;   // match OpticalBody for now

  bool readReal(float* out, size_t max_n);
  bool readSynthetic(float* out, size_t max_n, uint16_t laser_hint = 0);
};
