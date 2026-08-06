#pragma once

#include <Arduino.h>

/**
 * BPW34Reader
 *
 * Reads the detector bank.
 *
 * Default path (recommended): real values via ADS1115 + CD74HC4067 mux.
 *   - One CD74HC4067 → ADS1115 A0 gives 16 distinct channels.
 *   - Detectors beyond 15 currently wrap the mux (or can be expanded later
 *     with additional muxes / ADS1115s).
 *
 * Optional synthetic path only when OPTICAL_USE_SYNTHETIC is defined
 * (board bring-up without detectors wired).
 *
 * See DETECTOR_ARCHITECTURE.md for the full dual-stream (LM393 + ADS) plan.
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
  // Match OpticalBody::NUM_DETECTORS for now. First 16 are unique via mux;
  // any extra indices wrap the mux until more hardware is added.
  int num_detectors_ = 20;

  bool readReal(float* out, size_t max_n);
  bool readSynthetic(float* out, size_t max_n, uint16_t laser_hint = 0);
};
