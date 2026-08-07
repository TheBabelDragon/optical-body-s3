#include "bpw34_reader.h"
#include "mux_controller.h"

#ifdef OPTICAL_USE_SYNTHETIC
  // pure software fallback
#else
  #include <Adafruit_ADS1X15.h>
  static Adafruit_ADS1115 ads;
  static MuxController mux;          // default pins from build flags
#endif

#ifndef ADS_MUX_CHANNEL
#define ADS_MUX_CHANNEL 0            // A0 receives the mux SIG
#endif

#ifndef BPW34_FULL_SCALE_V
#define BPW34_FULL_SCALE_V 3.3f      // tune after front-end characterization
#endif

bool BPW34Reader::begin() {
#ifdef OPTICAL_USE_SYNTHETIC
  Serial.println(F("[BPW34] SYNTHETIC mode"));
  return true;
#else
  if (!mux.begin()) {
    Serial.println(F("[BPW34] MuxController failed"));
    return false;
  }

  if (!ads.begin(0x48)) {
    Serial.println(F("[BPW34] ADS1115 not found at 0x48 — check wiring / ADDR pin"));
    return false;
  }
  ads.setGain(GAIN_ONE);             // ±4.096 V

  Serial.println(F("[BPW34] ADS1115 + CD74HC4067 ready (real values)"));
  Serial.print(F("[BPW34] detectors="));
  Serial.print(num_detectors_);
  Serial.print(F("  first 16 unique via mux  ADS ch="));
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
    uint8_t ch = (uint8_t)(i % 16);
    mux.select(ch);

    int16_t raw = ads.readADC_SingleEnded(ADS_MUX_CHANNEL);
    float volts = ads.computeVolts(raw);
    out[i] = constrain(volts / BPW34_FULL_SCALE_V, 0.0f, 1.0f);
  }
  return true;
#else
  return false;
#endif
}

void BPW34Reader::dumpRaw(uint8_t count) {
#ifdef OPTICAL_USE_SYNTHETIC
  Serial.println(F("[BPW34] dumpRaw skipped (synthetic mode)"));
#else
  if (count > 16) count = 16;
  Serial.println(F("[BPW34] raw volts (first channels):"));
  for (uint8_t i = 0; i < count; ++i) {
    mux.select(i);
    int16_t raw = ads.readADC_SingleEnded(ADS_MUX_CHANNEL);
    float v = ads.computeVolts(raw);
    Serial.print(F("  ch"));
    if (i < 10) Serial.print('0');
    Serial.print(i);
    Serial.print(F(": "));
    Serial.print(v, 4);
    Serial.print(F(" V  (raw="));
    Serial.print(raw);
    Serial.println(F(")"));
  }
#endif
}

bool BPW34Reader::readSynthetic(float* out, size_t max_n, uint16_t laser_hint) {
  size_t n = min(max_n, (size_t)num_detectors_);
  for (size_t i = 0; i < n; ++i) {
    float dist = abs((int)i - (int)(laser_hint % num_detectors_));
    float coupling = expf(-0.35f * dist);
    float noise = ((float)(random(0, 1000)) / 1000.0f - 0.5f) * 0.03f;
    out[i] = constrain(coupling * 0.7f + 0.15f + noise, 0.0f, 1.0f);
  }
  return true;
}
