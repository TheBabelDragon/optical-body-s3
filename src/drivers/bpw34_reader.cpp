#include "bpw34_reader.h"

#ifdef OPTICAL_USE_SYNTHETIC
  // pure software fallback — only for board bring-up without detectors
#else
  #include <Adafruit_ADS1X15.h>
  // TODO: also wire CD74HC4067 mux select pins + multiple ADS1115 if needed
  static Adafruit_ADS1115 ads;
#endif

bool BPW34Reader::begin() {
#ifdef OPTICAL_USE_SYNTHETIC
  Serial.println(F("[BPW34] SYNTHETIC mode (OPTICAL_USE_SYNTHETIC)"));
  return true;
#else
  // Real path — ADS1115 on default I2C address 0x48
  if (!ads.begin(0x48)) {
    Serial.println(F("[BPW34] ADS1115 not found at 0x48 — check wiring"));
    return false;
  }
  ads.setGain(GAIN_ONE);          // ±4.096 V — adjust to your front-end
  Serial.println(F("[BPW34] ADS1115 ready (real values)"));
  return true;
#endif
}

bool BPW34Reader::readAll(float* out, size_t max_n) {
  if (!out || max_n == 0) return false;

#ifdef OPTICAL_USE_SYNTHETIC
  return readSynthetic(out, max_n);
#else
  return readReal(out, max_n);
#endif
}

bool BPW34Reader::readReal(float* out, size_t max_n) {
#ifndef OPTICAL_USE_SYNTHETIC
  size_t n = min(max_n, (size_t)num_detectors_);

  // Phase-0 simplification: read the four differential / single-ended
  // channels of one ADS1115 and replicate / pad for the rest.
  // Real system will walk the CD74HC4067 mux and possibly multiple ADS1115s.
  for (size_t i = 0; i < n; ++i) {
    // Map detector index onto available ADC channels for now
    int16_t raw = ads.readADC_SingleEnded(i % 4);
    // Convert to volts then normalize roughly into 0..1
    // (exact scaling depends on the analog front-end around each BPW34)
    float volts = ads.computeVolts(raw);
    // Placeholder normalization — tune once the front-end is characterized
    float norm = constrain(volts / 3.3f, 0.0f, 1.0f);
    out[i] = norm;
  }
  return true;
#else
  return false;
#endif
}

bool BPW34Reader::readSynthetic(float* out, size_t max_n, uint16_t laser_hint) {
  size_t n = min(max_n, (size_t)num_detectors_);
  for (size_t i = 0; i < n; ++i) {
    // Deterministic-ish coupling so replay looks stable
    float dist = abs((int)i - (int)(laser_hint % num_detectors_));
    float coupling = expf(-0.35f * dist);
    float noise = ((float)(random(0, 1000)) / 1000.0f - 0.5f) * 0.03f;
    out[i] = constrain(coupling * 0.7f + 0.15f + noise, 0.0f, 1.0f);
  }
  return true;
}
