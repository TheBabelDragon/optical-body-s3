#include "optical_body.h"
#include "../drivers/laser_matrix.h"
#include "../drivers/bpw34_reader.h"
#include "../drivers/event_reader.h"
#include "../protocol/json_encoder.h"
#include <math.h>

static LaserMatrix lasers;
static BPW34Reader detectors;
static EventReader events;

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
    fingerprint_.valid = true;
    fingerprint_.num_lasers = NUM_LASERS;
    fingerprint_.num_detectors = NUM_DETECTORS;
    strncpy(fingerprint_.geometry_version, identity_.geometryVersion(),
            sizeof(fingerprint_.geometry_version) - 1);
    for (int l = 0; l < NUM_LASERS; ++l) {
      for (int d = 0; d < NUM_DETECTORS; ++d) {
        float e = identity_.getExpected((uint16_t)l, (uint16_t)d);
        transfer_[l][d] = e;
        fingerprint_.matrix[l][d] = e;
      }
    }
  }

  if (!lasers.begin()) {
    Serial.println(F("[OpticalBody] laser matrix init failed"));
  }

  if (!detectors.begin()) {
    Serial.println(F("[OpticalBody] BPW34 reader init failed — continuing without detectors"));
  }

  events.begin();

  Serial.print(F("[OpticalBody] emitters="));
  Serial.print(lasers.numLasers());
  Serial.print(F("  detectors="));
  Serial.print(detectors.numDetectors());
  Serial.print(F("  event_ch="));
  Serial.println(events.numChannels());
  return true;
}

void OpticalBody::dumpRaw(uint8_t count) {
  detectors.dumpRaw(count);
}

float OpticalBody::verifyIdentity(bool* out_unchanged, float threshold) {
  if (out_unchanged) *out_unchanged = false;

  if (!identity_.hasIdentity()) {
    Serial.println(F("[Identity] no stored fingerprint — full calibration required"));
    return 1.0f;
  }

  Serial.println(F("[Identity] probing body against stored fingerprint…"));

  if (!acquireDarkFrame()) {
    Serial.println(F("[Identity] dark frame failed during verify"));
    return 1.0f;
  }

  const int probe_ids[] = {0, NUM_LASERS / 2, NUM_LASERS - 1};
  const int n_probes = 3;
  float sum_abs = 0.0f;
  int n_terms = 0;

  for (int pi = 0; pi < n_probes; ++pi) {
    int laser = probe_ids[pi];
    if (laser < 0 || laser >= NUM_LASERS) continue;

    lasers.fire((uint8_t)laser);
    delay(15);
    float buf[NUM_DETECTORS];
    bool ok = readAveraged(buf, NUM_DETECTORS, 2);
    lasers.allOff();
    delay(5);
    if (!ok) continue;

    isolateDark(2);
    for (int d = 0; d < NUM_DETECTORS; ++d) {
      float corrected = buf[d] - dark_track_.effective((uint8_t)d);
      if (corrected < 0.0f) corrected = 0.0f;
      float expected = identity_.getExpected((uint16_t)laser, (uint16_t)d);
      sum_abs += fabsf(corrected - expected);
      n_terms++;
    }

    Serial.print(F("  probe source "));
    Serial.print(laser);
    Serial.println(F(" ok"));
  }

  float mean_residual = (n_terms > 0) ? (sum_abs / (float)n_terms) : 1.0f;
  bool unchanged = mean_residual <= threshold;
  if (out_unchanged) *out_unchanged = unchanged;

  Serial.print(F("[Identity] mean residual="));
  Serial.print(mean_residual, 4);
  Serial.print(F("  threshold="));
  Serial.print(threshold, 4);
  Serial.print(F("  → "));
  Serial.println(unchanged ? F("Body unchanged") : F("Geometry drift detected"));

  String status = String("{\"event\":\"identity_verify\",\"node\":\"") +
                  node_id_ +
                  "\",\"mean_residual\":" + String(mean_residual, 4) +
                  ",\"unchanged\":" + String(unchanged ? "true" : "false") +
                  "}";
  archive_.appendCalibrationEvent(status);
  Serial.println(status);

  calibrated_ = unchanged || identity_.hasIdentity();
  return mean_residual;
}

bool OpticalBody::exciteOnce(uint16_t laser_id) {
  if (laser_id >= (uint16_t)NUM_LASERS) return false;

  float buf[NUM_DETECTORS];
  lasers.fire((uint8_t)laser_id);
  delay(15);
  bool ok = readAveraged(buf, NUM_DETECTORS, 2);
  lasers.allOff();

  if (!ok) {
    for (int d = 0; d < NUM_DETECTORS; ++d) buf[d] = 0.0f;
    emitObservation(laser_id, buf, NUM_DETECTORS, true);
    excitation_counter_++;
    return false;
  }

  isolateDark(2);
  subtractEffectiveDark(buf, NUM_DETECTORS);

  emitObservation(laser_id, buf, NUM_DETECTORS, true);
  excitation_counter_++;
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
  Serial.println(F("[OpticalBody] Dark frame — all emitters OFF (voltage stance)"));
  lasers.allOff();
  delay(30);

  bool ok = readAveraged(dark_, NUM_DETECTORS, 4);
  if (!ok) {
    Serial.println(F("[OpticalBody] dark frame FAILED (no detectors)"));
    for (int i = 0; i < NUM_DETECTORS; ++i) dark_[i] = 0.0f;
    dark_track_.reset();
    return false;
  }

  float mean = 0.0f;
  for (int i = 0; i < NUM_DETECTORS; ++i) mean += dark_[i];
  mean /= (float)NUM_DETECTORS;
  Serial.print(F("[OpticalBody] dark mean="));
  Serial.println(mean, 4);

  fingerprint_.setDark(dark_, NUM_DETECTORS);
  dark_track_.setBaseline(dark_, NUM_DETECTORS);
  return true;
}

bool OpticalBody::isolateDark(uint8_t extra_passes) {
  lasers.allOff();
  delay(8);

  float raw[NUM_DETECTORS];
  for (uint8_t p = 0; p <= extra_passes; ++p) {
    if (!readAveraged(raw, NUM_DETECTORS, 1)) return false;
    dark_track_.update(raw, NUM_DETECTORS);
    if (dark_track_.isolationOk()) return true;
    delay(8);
  }
  Serial.print(F("[OpticalBody] dark isolation pending  mean_q="));
  Serial.print(dark_track_.meanQ(), 4);
  Serial.print(F("  fault_mask=0x"));
  Serial.println(dark_track_.faultMask(), HEX);
  return dark_track_.isolationOk();
}

void OpticalBody::subtractEffectiveDark(float* buf, size_t n) {
  if (!buf) return;
  for (size_t d = 0; d < n && d < (size_t)NUM_DETECTORS; ++d) {
    buf[d] -= dark_track_.effective((uint8_t)d);
    if (buf[d] < 0.0f) buf[d] = 0.0f;
  }
}

void OpticalBody::runSelfMap() {
  Serial.println(F("[OpticalBody] === Clean calibration (whatsinthebox) ==="));
  calibrated_ = false;
  clearTransfer();
  fingerprint_.clear();

  if (!acquireDarkFrame()) {
    Serial.println(F("[OpticalBody] aborting self-map — no dark frame"));
    return;
  }

  sequence_.buildOneHot((uint8_t)NUM_LASERS, /*settle*/ 15, /*samples*/ 2, /*repeats*/ 1);
  Serial.print(F("[OpticalBody] sequence="));
  Serial.print(sequence_.label());
  Serial.print(F("  steps="));
  Serial.println(sequence_.numSteps());

  for (int si = 0; si < sequence_.numSteps(); ++si) {
    const ExcitationStep& step = sequence_.step(si);
    uint8_t laser = step.source_id;

    Serial.print(F("  source "));
    Serial.print(laser);
    Serial.print(F(" → isolating…"));

    if (!isolateDark(4)) {
      Serial.println(F(" DARK_BUSY"));
    }

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

      isolateDark(1);
      for (int d = 0; d < NUM_DETECTORS; ++d) {
        float c = buf[d] - dark_track_.effective((uint8_t)d);
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
    isolateDark(1);
    subtractEffectiveDark(buf, NUM_DETECTORS);
    emitObservation(laser, buf, NUM_DETECTORS, true);
  }
  excitation_counter_++;
}

void OpticalBody::emitObservation(uint16_t laser_id, const float* detectors, size_t n,
                                  bool /*dark_corrected*/) {
  FieldObservation obs;
  obs.body_id        = node_id_;
  obs.body_type      = "optical";
  obs.excitation_id  = (int32_t)excitation_counter_;
  obs.geometry_state = calibrated_ ? "calibrated" : "calibrating";
  obs.timestamp      = String(millis());
  obs.health         = dark_track_.isolationOk() ? "ok" : "partial";
  obs.laser_id       = (int)laser_id;
  obs.schema_version = 1;
  obs.event_mask     = events.readMask();

  float max_obs = 0.0f;
  int max_idx = 0;

  for (size_t i = 0; i < n; ++i) {
    FieldRegion r;
    r.region   = "detector_" + String(i < 10 ? "0" : "") + String(i);
    r.observed = detectors[i];

    if (fingerprint_.valid || calibrated_ || identity_.hasIdentity()) {
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

    if (detectors[i] > max_obs) {
      max_obs = detectors[i];
      max_idx = (int)i;
    }
  }

  String json = encodeFieldObservation(obs);
  Serial.println(json);
  archive_.appendObservation(json);

  Serial.print(F("OBS {"));
  Serial.print(F("\"body_id\":\"")); Serial.print(node_id_); Serial.print(F("\","));
  Serial.print(F("\"body_type\":\"optical\","));
  Serial.print(F("\"excitation_id\":")); Serial.print((int)laser_id); Serial.print(F(","));
  Serial.print(F("\"geometry_state\":\"")); Serial.print(obs.geometry_state); Serial.print(F("\","));
  Serial.print(F("\"health\":\"")); Serial.print(obs.health); Serial.print(F("\","));
  Serial.print(F("\"regions\":[{\"region\":\"detector_"));
  if (max_idx < 10) Serial.print('0');
  Serial.print(max_idx);
  Serial.print(F("\",\"observed\":"));
  Serial.print(max_obs, 4);
  Serial.print(F(",\"confidence\":0.85}]"));
  Serial.println(F("}"));
}
