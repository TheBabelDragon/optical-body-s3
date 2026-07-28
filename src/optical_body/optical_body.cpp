#include "optical_body.h"
#include "../drivers/laser_matrix.h"
#include "../drivers/bpw34_reader.h"
#include "../protocol/json_encoder.h"

static LaserMatrix lasers;
static BPW34Reader detectors;

OpticalBody::OpticalBody(const char* node_id) : node_id_(node_id) {
  clearTransfer();
}

void OpticalBody::clearTransfer() {
  for (int l = 0; l < NUM_LASERS; ++l)
    for (int d = 0; d < NUM_DETECTORS; ++d)
      transfer_[l][d] = 0.0f;
}

bool OpticalBody::begin() {
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

void OpticalBody::runSelfMap() {
  Serial.println(F("[OpticalBody] === Beginning self-map ==="));
  calibrated_ = false;
  clearTransfer();

  float buf[NUM_DETECTORS];

  for (int laser = 0; laser < NUM_LASERS; ++laser) {
    Serial.print(F("  Laser "));
    Serial.print(laser);
    Serial.print(F(" → recording…"));

    lasers.fire(laser);
    delay(15);                       // settle
    bool ok = detectors.readAll(buf, NUM_DETECTORS);
    lasers.allOff();
    delay(5);

    if (!ok) {
      Serial.println(F(" FAIL"));
      continue;
    }

    for (int d = 0; d < NUM_DETECTORS; ++d) {
      transfer_[laser][d] = buf[d];
    }

    // Emit a Phase-0 observation for this excitation so the log is complete
    emitObservation(laser, buf, NUM_DETECTORS);
    Serial.println(F(" ok"));
  }

  calibrated_ = true;
  Serial.println(F("[OpticalBody] Geometry fingerprint created."));
  Serial.println(F("[OpticalBody] === Self-map complete ==="));
}

void OpticalBody::tickPassive() {
  // Cycle through lasers one by one for continuous passive observability
  uint16_t laser = excitation_counter_ % NUM_LASERS;
  float buf[NUM_DETECTORS];

  lasers.fire(laser);
  delay(12);
  bool ok = detectors.readAll(buf, NUM_DETECTORS);
  lasers.allOff();

  if (ok) {
    emitObservation(laser, buf, NUM_DETECTORS);
  }
  excitation_counter_++;
}

void OpticalBody::emitObservation(uint16_t laser_id, const float* detectors, size_t n) {
  FieldObservation obs;
  obs.body_id        = node_id_;
  obs.body_type      = "optical";
  obs.excitation_id  = (int32_t)excitation_counter_;
  obs.geometry_state = calibrated_ ? "calibrated" : "calibrating";
  obs.timestamp      = String(millis());     // replace with RTC/ISO later
  obs.health         = "ok";
  obs.laser_id       = (int)laser_id;
  obs.schema_version = 1;

  for (size_t i = 0; i < n; ++i) {
    FieldRegion r;
    r.region     = "detector_" + String(i < 10 ? "0" : "") + String(i);
    r.observed   = detectors[i];
    r.expected   = NAN;                     // filled once transfer matrix is trusted
    r.confidence = (detectors[i] > 0.05f) ? 0.9f : 0.5f;
    r.anomaly    = 0.0f;
    obs.regions.push_back(r);
  }

  String json = encodeFieldObservation(obs);
  Serial.println(json);                     // Aurora / host can scrape Serial for now
}
