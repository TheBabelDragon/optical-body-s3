#include "sd_archive.h"

// Uncomment and wire when the MicroSD breakout is present.
// #include <SD.h>
// #include <SPI.h>

bool SdArchive::begin(uint8_t /*cs_pin*/) {
  // TODO: SD.begin(cs_pin) and ensure /optical/ directory exists
  Serial.println(F("[SD] archive not yet wired — observations stay on Serial only"));
  ready_ = false;
  obs_count_ = 0;
  return true;   // non-fatal
}

bool SdArchive::appendObservation(const String& json_line) {
  if (!ready_) return false;
  // TODO: open /optical/obs.jsonl in FILE_APPEND and write json_line + '\n'
  obs_count_++;
  return true;
}

bool SdArchive::appendCalibrationEvent(const String& json_line) {
  if (!ready_) return false;
  // TODO: open /optical/cal.jsonl in FILE_APPEND
  return true;
}
