#pragma once

#include <Arduino.h>

/**
 * BPW34Reader
 *
 * Analog path: up to 4× ADS1115 + up to 4× CD74HC4067.
 *
 * Mapping (when fully populated):
 *   detectors  0-15 → ADS0 (0x48) + mux0
 *   detectors 16-31 → ADS1 (0x49) + mux1
 *   detectors 32-47 → ADS2 (0x4A) + mux2
 *   detectors 48-63 → ADS3 (0x4B) + mux3
 *
 * OpticalBody asks for 20 detectors → first two pairs are enough.
 * Phase-0 pin map: WIRING_PHASE0.md
 */
class BPW34Reader {
public:
  static constexpr int MAX_ADS = 4;
  static constexpr int CHANNELS_PER_MUX = 16;

  bool begin();
  bool readAll(float* out, size_t max_n);
  void dumpRaw(uint8_t count = 16);

  int numDetectors() const { return num_detectors_; }
  int numAdsFound() const { return num_ads_; }

private:
  int num_detectors_ = 20;
  int num_ads_ = 0;

  bool readReal(float* out, size_t max_n);
  bool readSynthetic(float* out, size_t max_n, uint16_t laser_hint = 0);
};
