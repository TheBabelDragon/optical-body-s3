#pragma once

#include <Arduino.h>

/**
 * BPW34Reader
 *
 * Reads the detector bank via ADS1115 + one or more CD74HC4067 muxes.
 *
 * Default (Phase 0): one mux → ADS1115 A0 → 16 unique channels.
 * Detectors 16-19 currently wrap the first mux (matches OpticalBody::NUM_DETECTORS=20).
 *
 * You already have multiple blue mux + ADS1115 boards.
 * The MuxController constructor accepts explicit pins, so adding a second
 * mux later is just another instance + a different ADS channel or EN pin.
 *
 * Synthetic path remains available with -D OPTICAL_USE_SYNTHETIC=1.
 */
class BPW34Reader {
public:
  bool begin();

  /**
   * Read all detectors into out[] (size == numDetectors()).
   * Values normalized roughly 0..1.
   */
  bool readAll(float* out, size_t max_n);

  /**
   * Bring-up helper: print raw volts for the first `count` channels.
   * Call this from setup() or a serial command to verify wiring
   * before running dark-frame / self-map.
   */
  void dumpRaw(uint8_t count = 8);

  int numDetectors() const { return num_detectors_; }

private:
  int num_detectors_ = 20;

  bool readReal(float* out, size_t max_n);
  bool readSynthetic(float* out, size_t max_n, uint16_t laser_hint = 0);
};
