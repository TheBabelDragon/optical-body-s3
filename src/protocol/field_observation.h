#pragma once

#include <Arduino.h>
#include <vector>

/**
 * Minimal C++ mirror of schemas/field_observation.py
 * so the ESP32 can emit the same conceptual packet.
 *
 * We keep it simple: a few fixed fields + a detector vector.
 * JSON encoding happens in json_encoder.
 */

struct FieldRegion {
  String region;          // e.g. "detector_03"
  float  observed;        // 0..1 normalized
  float  expected;        // NaN if unknown (Phase 0)
  float  confidence;      // 0..1
  float  anomaly;         // 0..1
};

struct FieldObservation {
  String body_id;
  String body_type;       // always "optical" for this firmware
  int32_t excitation_id;
  std::vector<FieldRegion> regions;
  String geometry_state;  // "uncalibrated" | "calibrating" | "calibrated" | "degraded"
  String timestamp;       // ISO-ish or millis for now
  String health;          // "ok" | "partial" | "error"
  int    schema_version = 1;

  // Optional modality payload (laser id, pattern, etc.)
  int    laser_id = -1;
};
