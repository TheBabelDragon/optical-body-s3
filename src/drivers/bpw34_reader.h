#pragma once

#include <Arduino.h>

/**
 * BPW34Reader
 *
 * Analog path: up to 4× ADS1115 + up to 4× CD74HC4067.
 * No synthetic ADC. If the converter is missing, begin() fails.
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
};
