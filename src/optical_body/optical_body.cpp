#include "optical_body.h"
#include "../drivers/laser_matrix.h"
#include "../drivers/bpw34_reader.h"
#include "../protocol/json_encoder.h"
#include <math.h>

static LaserMatrix lasers;
static BPW34Reader detectors;

OpticalBody::OpticalBody(const char* node_id) : node_id_(node_id) {
  clearTransfer();
  fingerprint_.clear();
  for (int i = 0; i < NUM_DETECTORS; ++i) dark_[i] = 0.0f;
}

void OpticalBody::clearTransfer() {
  for (int l = 0; l < NUM_LASERS; ++l)
    for (int d = 0; d < NUM_DETECTORS; ++d)
      transfer_[l][d] = 0.0f;
}

bool OpticalBody::begin() {
  identity_.begin(0x50);
  archive_.begin(5);

  if (identity_.hasIdentity()) {
    Serial.print(F("[OpticalBody] loaded identity  geometry_version="));
    Serial.println(identity_.geometryVersion());
    calibrated_ = true;
  }

  if (!lasers.begin()) {
    Serial.println(F("[OpticalBody] laser matrix init failed"));
    return false;
  }
  if (!detectors.begin()) {
    Serial.println(F("[OpticalBody] BPW34 reader init failed"));
    return false;
  }
  Serial.print(F("[OpticalBody] emitters="));
  Serial.print(lasers.numLasers());
  Serial.print(F("  detectors="));
  Serial.println(detectors.numDetectors());
  return true;
}

bool OpticalBody::readAveraged(float* out, size_t n, uint16_t samples) {
  if (!out || n == 0 || samples == 0) return false;

  float acc[NUM_DETECTORS];
  for (size_t i = 0; i < n && i < (size_t)NUM_DETECTORS; ++i) acc[i] = 0.0f;

  for (uint16_t s = 0; s < samples; ++s) {
    float tmp[NUM_DETECTORS];
    if (!detectors.readAll(tmp, n)) return false;
    for (size_t i = 0; i < n && i < (size_t)NUM_DETECTORS; ++i)
      acc[i] += tmp[i];
    if (samples > 1) delay(2);
  }

  for (size_t i = 0; i < n && i < (size_t)NUM_DETECTORS; ++i)
    out[i] = acc[i] / (float)samples;
  return true;
}

bool OpticalBody::acquireDarkFrame() {
  Serial.println(F("[OpticalBody] Dark frame — all emitters OFF"));
  lasers.allOff();
  delay(30);  // settle ambient + electrical

  bool ok = readAveraged(dark_, NUM_DETECTORS, 4);
  if (!ok) {
    Serial.println(F("[OpticalBody] dark frame FAILED"));
    return false;
  }

  float mean = 0.0f;
  for (int i = 0; i < NUM_DETECTORS; ++i) mean += dark_[i];
  mean /= (float)NUM_DETECTORS;
  Serial.print(F("[OpticalBody] dark mean="));
  Serial.println(mean, 4);

  fingerprint_.setDark(dark_, NUM_DETECTORS);
  return true;
}

void OpticalBody::runSelfMap() {
  Serial.println(F("[OpticalBody] === Clean calibration (whatsinthebox) ==="));
  calibrated_ = false;
  clearTransfer();
  fingerprint_.clear();

  // 1. Dark frame — isolate electrical / ambient floor
  if (!acquireDarkFrame()) {
    Serial.println(F("[OpticalBody] aborting self-map — no dark frame"));
    return;
  }

  // 2. Formal one-hot excitation sequence
  sequence_.buildOneHot((uint8_t)NUM_LASERS, /*settle*/ 15, /*samples*/ 2, /*repeats*/ 1);
  Serial.print(F("[OpticalBody] sequence="));
  Serial.print(sequence_.label());
  Serial.print(F("  steps="));
  Serial.println(sequence_.numSteps());

  // 3. Run each step: fire → settle → average → dark-correct → store
  for (int si = 0; si < sequence_.numSteps(); ++si) {
    const ExcitationStep& step = sequence_.step(si);
    uint8_t laser = step.source_id;

    Serial.print(F("  source "));
    Serial.print(laser);
    Serial.print(F(" → isolating…"));

    float accum[NUM_DETECTORS];
    for (int d = 0; d < NUM_DETECTORS; ++d) accum[d] = 0.0f;
    bool any_ok = false;

    for (uint8_t r = 0; r < step.repeats; ++r) {
      lasers.fire(laser);
      delay(step.settle_time_ms);

      float buf[NUM_DETECTORS];
      bool ok = readAveraged(buf, NUM_DETECTORS, step.samples);
      lasers.allOff();
      delay(5);

      if (!ok) continue;
      any_ok = true;

      // Dark-correct: R_corrected = R_measured - D
      for (int d = 0; d < NUM_DETECTORS; ++d) {
        float c = buf[d] - dark_[d];
        if (c < 0.0f) c = 0.0f;
        accum[d] += c;
      }
    }

    if (!any_ok) {
      Serial.println(F(" FAIL"));
      continue;
    }

    for (int d = 0; d < NUM_DETECTORS; ++d) {
      float v = accum[d] / (float)step.repeats;
      transfer_[laser][d] = v;
      identity_.setExpected((uint16_t)laser, (uint16_t)d, v);
    }
    fingerprint_.setRow(laser, transfer_[laser], NUM_DETECTORS);

    emitObservation(laser, transfer_[laser], NUM_DETECTORS, true);
    Serial.println(F(" ok"));
  }

  // 4. First physical memory → FRAM
  fingerprint_.timestamp_ms = millis();
  fingerprint_.num_lasers = NUM_LASERS;
  fingerprint_.num_detectors = NUM_DETECTORS;
  fingerprint_.valid = true;
  strncpy(fingerprint_.geometry_version, sequence_.label(),
          sizeof(fingerprint_.geometry_version) - 1);

  identity_.setGeometryVersion(fingerprint_.geometry_version);
  identity_.save();

  String cal_event = String("{\"event\":\"self_map_complete\",\"node\":\"") +
                     node_id_ +
                     "\",\"geometry_version\":\"" + String(fingerprint_.geometry_version) +
                     "\",\"dark_corrected\":true}";
  archive_.appendCalibrationEvent(cal_event);

  calibrated_ = true;
  Serial.println(F("[OpticalBody] OpticalFingerprint created + saved to FRAM."));
  Serial.println(F("[OpticalBody] === Calibration complete ==="));
}

void OpticalBody::tickPassive() {
  uint16_t laser = excitation_counter_ % NUM_LASERS;
  float buf[NUM_DETECTORS];

  lasers.fire(laser);
  delay(12);
  bool ok = readAveraged(buf, NUM_DETECTORS, 1);
  lasers.allOff();

  if (ok) {
    // Dark-correct when we have a fingerprint
    if (fingerprint_.valid) {
      for (int d = 0; d < NUM_DETECTORS; ++d) {
        buf[d] -= fingerprint_.dark_frame[d];
        if (buf[d] < 0.0f) buf[d] = 0.0f;
      }
    }
    emitObservation(laser, buf, NUM_DETECTORS, fingerprint_.valid);
  }
  excitation_counter_++;
}

void OpticalBody::emitObservation(uint16_t laser_id, const float* detectors, size_t n,
                                  bool dark_corrected) {
  FieldObservation obs;
  obs.body_id        = node_id_;
  obs.body_type      = "optical";
  obs.excitation_id  = (int32_t)excitation_counter_;
  obs.geometry_state = calibrated_ ? "calibrated" : "calibrating";
  obs.timestamp      = String(millis());
  obs.health         = "ok";
  obs.laser_id       = (int)laser_id;
  obs.schema_version = 1;

  for (size_t i = 0; i < n; ++i) {
    FieldRegion r;
    r.region   = "detector_" + String(i < 10 ? "0" : "") + String(i);
    r.observed = detectors[i];

    if (fingerprint_.valid || calibrated_) {
      r.expected = fingerprint_.expected((uint8_t)laser_id, (uint8_t)i);
      if (isnan(r.expected))
        r.expected = identity_.getExpected(laser_id, (uint16_t)i);

      if (!isnan(r.expected)) {
        r.anomaly = fabsf(r.observed - r.expected);
        if (r.anomaly > 1.0f) r.anomaly = 1.0f;
      } else {
        r.anomaly = 0.0f;
      }
    } else {
      r.expected = NAN;
      r.anomaly  = 0.0f;
    }

    r.confidence = (detectors[i] > 0.05f) ? 0.9f : 0.5f;
    obs.regions.push_back(r);
  }

  String json = encodeFieldObservation(obs);
  Serial.println(json);
  archive_.appendObservation(json);
}
