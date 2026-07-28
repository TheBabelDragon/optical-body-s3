#include "json_encoder.h"
#include <ArduinoJson.h>

String encodeFieldObservation(const FieldObservation& obs) {
  JsonDocument doc;

  doc["schema_version"] = obs.schema_version;
  doc["body_id"]        = obs.body_id;
  doc["body_type"]      = obs.body_type;
  doc["excitation_id"]  = obs.excitation_id;
  doc["geometry_state"] = obs.geometry_state;
  doc["timestamp"]      = obs.timestamp;
  doc["health"]         = obs.health;

  if (obs.laser_id >= 0) {
    doc["modality"]["laser_id"] = obs.laser_id;
  }

  JsonArray regions = doc["field_regions"].to<JsonArray>();
  for (const auto& r : obs.regions) {
    JsonObject o = regions.add<JsonObject>();
    o["region"]     = r.region;
    o["observed"]   = serialized(String(r.observed, 4));
    if (!isnan(r.expected)) {
      o["expected"] = serialized(String(r.expected, 4));
    }
    o["confidence"] = serialized(String(r.confidence, 3));
    o["anomaly"]    = serialized(String(r.anomaly, 3));
  }

  String out;
  serializeJson(doc, out);
  return out;
}
