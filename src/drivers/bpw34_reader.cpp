#include "bpw34_reader.h"
#include "mux_controller.h"

#include <Adafruit_ADS1X15.h>

static Adafruit_ADS1115 ads[BPW34Reader::MAX_ADS];
static MuxController mux[BPW34Reader::MAX_ADS] = {
  MuxController(MUX_S0_PIN, MUX_S1_PIN, MUX_S2_PIN, MUX_S3_PIN, MUX_EN_PIN),
  MuxController(MUX_S0_PIN, MUX_S1_PIN, MUX_S2_PIN, MUX_S3_PIN, 16),
  MuxController(MUX_S0_PIN, MUX_S1_PIN, MUX_S2_PIN, MUX_S3_PIN, 17),
  MuxController(MUX_S0_PIN, MUX_S1_PIN, MUX_S2_PIN, MUX_S3_PIN, 18)
};

#ifndef BPW34_FULL_SCALE_V
#define BPW34_FULL_SCALE_V 3.3f
#endif

static const uint8_t ADS_ADDR[4] = { 0x48, 0x49, 0x4A, 0x4B };

bool BPW34Reader::begin() {
  num_ads_ = 0;

  for (int i = 0; i < MAX_ADS; ++i) {
    if (!mux[i].begin()) {
      Serial.print(F("[BPW34] mux"));
      Serial.print(i);
      Serial.println(F(" init failed"));
      continue;
    }

    if (!ads[i].begin(ADS_ADDR[i])) {
      Serial.print(F("[BPW34] ADS1115 not found at 0x"));
      Serial.println(ADS_ADDR[i], HEX);
      continue;
    }
    ads[i].setGain(GAIN_ONE);
    num_ads_++;
    Serial.print(F("[BPW34] ADS"));
    Serial.print(i);
    Serial.print(F(" @0x"));
    Serial.print(ADS_ADDR[i], HEX);
    Serial.println(F(" ready"));
  }

  if (num_ads_ == 0) {
    Serial.println(F("[BPW34] no ADS1115 — detector reads disabled"));
    return false;
  }

  for (int j = 1; j < MAX_ADS; ++j) mux[j].disable();
  mux[0].enable();

  Serial.print(F("[BPW34] "));
  Serial.print(num_ads_);
  Serial.print(F(" ADS1115 active, detectors="));
  Serial.println(num_detectors_);
  return true;
}

bool BPW34Reader::readAll(float* out, size_t max_n) {
  if (!out || max_n == 0) return false;
  if (num_ads_ == 0) return false;
  return readReal(out, max_n);
}

bool BPW34Reader::readReal(float* out, size_t max_n) {
  if (num_ads_ == 0) return false;

  size_t n = min(max_n, (size_t)num_detectors_);

  for (size_t i = 0; i < n; ++i) {
    int ads_idx = (int)(i / CHANNELS_PER_MUX);
    uint8_t ch  = (uint8_t)(i % CHANNELS_PER_MUX);

    if (ads_idx >= num_ads_ || ads_idx >= MAX_ADS) {
      ads_idx = 0;
      ch = (uint8_t)(i % CHANNELS_PER_MUX);
    }

    for (int j = 0; j < MAX_ADS; ++j) {
      if (j == ads_idx) mux[j].enable();
      else              mux[j].disable();
    }
    mux[ads_idx].select(ch);

    int16_t raw = ads[ads_idx].readADC_SingleEnded(0);
    float volts = ads[ads_idx].computeVolts(raw);
    out[i] = constrain(volts / BPW34_FULL_SCALE_V, 0.0f, 1.0f);
  }

  for (int j = 1; j < MAX_ADS; ++j) mux[j].disable();
  mux[0].enable();
  return true;
}

void BPW34Reader::dumpRaw(uint8_t count) {
  if (num_ads_ == 0) {
    Serial.println(F("[BPW34] no ADS1115 — dump skipped"));
    return;
  }
  if (count > 32) count = 32;
  Serial.println(F("[BPW34] raw volts:"));
  for (uint8_t i = 0; i < count; ++i) {
    int ads_idx = i / CHANNELS_PER_MUX;
    uint8_t ch  = i % CHANNELS_PER_MUX;
    if (ads_idx >= num_ads_) break;

    for (int j = 0; j < MAX_ADS; ++j) {
      if (j == ads_idx) mux[j].enable();
      else              mux[j].disable();
    }
    mux[ads_idx].select(ch);

    int16_t raw = ads[ads_idx].readADC_SingleEnded(0);
    float v = ads[ads_idx].computeVolts(raw);
    Serial.print(F("  d"));
    if (i < 10) Serial.print('0');
    Serial.print(i);
    Serial.print(F(" (ADS"));
    Serial.print(ads_idx);
    Serial.print(F(" ch"));
    Serial.print(ch);
    Serial.print(F("): "));
    Serial.print(v, 4);
    Serial.print(F(" V  raw="));
    Serial.println(raw);
  }
  for (int j = 1; j < MAX_ADS; ++j) mux[j].disable();
  mux[0].enable();
}
