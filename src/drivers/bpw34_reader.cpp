#include "bpw34_reader.h"
#include "mux_controller.h"

#ifdef OPTICAL_USE_SYNTHETIC
  // pure software fallback — only for board bring-up without detectors
#else
  #include <Adafruit_ADS1X15.h>
  static Adafruit_ADS1115 ads;
  static MuxController mux;
#endif

// ---------------------------------------------------------------------------
// Real path configuration
// ---------------------------------------------------------------------------
// The mux common output is expected on ADS1115 channel 0 (A0).
// Change this if you wire SIG to a different pin.
#ifndef ADS_MUX_CHANNEL
#define ADS_MUX_CHANNEL 0
#endif

// Rough full-scale for normalization. Tune after measuring the actual
// BPW34 + transimpedance front-end (dark / illuminated voltages).
#ifndef BPW34_FULL_SCALE_V
#define BPW34_FULL_SCALE_V 3.3f
#endif

bool BPW34Reader::begin() {
#ifdef OPTICAL_USE_SYNTHETIC
  Serial.println(F("[BPW34] SYNTHETIC mode (OPTICAL_USE_SYNTHETIC)"));
  return true;
#else
  if (!mux.begin()) {
    Serial.println(F("[BPW34] MuxController failed"));
    return false;
  }

  // Real path — ADS1115 on default I2C address 0x48
  if (!ads.begin(0x48)) {
    Serial.println(F("[BPW34] ADS1115 not found at 0x48 — check wiring"));
    return false;
  }
  ads.setGain(GAIN_ONE);          // ±4.096 V — adjust to your front-end
  // Optional: ads.setDataRate(RATE_ADS1115_128SPS);  // default is fine

  Serial.println(F("[BPW34] ADS1115 + CD74HC4067 ready (real values)"));
  Serial.print(F("[BPW34] detectors="));
  Serial.print(num_detectors_);
  Serial.print(F("  mux channels=16  ADS ch="));
  Serial.println(ADS_MUX_CHANNEL);
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

  for (size_t i = 0; i < n; ++i) {
    // Select mux channel (0..15). Extra detectors wrap for now.
    uint8_t ch = (uint8_t)(i % 16);
    mux.select(ch);

    // Read the single ADS channel that the mux SIG feeds into
    int16_t raw = ads.readADC_SingleEnded(ADS_MUX_CHANNEL);
    float volts = ads.computeVolts(raw);

    // Placeholder normalization — replace with calibrated dark/illuminated
    // scale once the analog front-end is characterized.
    float norm = constrain(volts / BPW34_FULL_SCALE_V, 0.0f, 1.0f);
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
