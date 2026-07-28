#include "fram_identity.h"
#include <Wire.h>
#include <string.h>

bool FramIdentity::begin(uint8_t i2c_addr) {
  // Probe the FRAM. Real MB85RC256V appears at 0x50–0x57 depending on A0–A2.
  Wire.beginTransmission(i2c_addr);
  uint8_t err = Wire.endTransmission();
  if (err != 0) {
    Serial.print(F("[FRAM] not found at 0x"));
    Serial.println(i2c_addr, HEX);
    Serial.println(F("[FRAM] continuing without persistent identity"));
    valid_ = false;
    return true;   // non-fatal for Phase 0; self-map still works
  }
  Serial.println(F("[FRAM] MB85RC256V present"));
  return load();
}

bool FramIdentity::load() {
  // TODO: read magic + version + expected_ + gain_ + offset_ from FRAM
  // For now we just mark as empty so self-map will populate it.
  valid_ = false;
  strncpy(geometry_version_, "uninitialized", sizeof(geometry_version_) - 1);
  for (uint16_t d = 0; d < MAX_DETECTORS; ++d) {
    gain_[d] = 1.0f;
    offset_[d] = 0.0f;
  }
  Serial.println(F("[FRAM] no valid identity yet — will create on self-map"));
  return true;
}

bool FramIdentity::save() {
  // TODO: write magic + version + matrices to FRAM
  valid_ = true;
  Serial.print(F("[FRAM] identity saved  geometry_version="));
  Serial.println(geometry_version_);
  return true;
}

bool FramIdentity::clear() {
  valid_ = false;
  strncpy(geometry_version_, "uninitialized", sizeof(geometry_version_) - 1);
  // TODO: wipe FRAM region
  Serial.println(F("[FRAM] identity cleared"));
  return true;
}

void FramIdentity::setExpected(uint16_t laser, uint16_t detector, float value) {
  if (laser < MAX_LASERS && detector < MAX_DETECTORS)
    expected_[laser][detector] = value;
}

float FramIdentity::getExpected(uint16_t laser, uint16_t detector) const {
  if (laser < MAX_LASERS && detector < MAX_DETECTORS)
    return expected_[laser][detector];
  return 0.0f;
}

void FramIdentity::setDetectorGain(uint16_t detector, float gain) {
  if (detector < MAX_DETECTORS) gain_[detector] = gain;
}

void FramIdentity::setDetectorOffset(uint16_t detector, float offset) {
  if (detector < MAX_DETECTORS) offset_[detector] = offset;
}

float FramIdentity::getDetectorGain(uint16_t detector) const {
  return (detector < MAX_DETECTORS) ? gain_[detector] : 1.0f;
}

float FramIdentity::getDetectorOffset(uint16_t detector) const {
  return (detector < MAX_DETECTORS) ? offset_[detector] : 0.0f;
}

void FramIdentity::setGeometryVersion(const char* ver) {
  strncpy(geometry_version_, ver, sizeof(geometry_version_) - 1);
  geometry_version_[sizeof(geometry_version_) - 1] = '\0';
}

bool FramIdentity::framRead(uint16_t /*addr*/, uint8_t* /*buf*/, size_t /*len*/) {
  // TODO: MB85RC256V sequential read
  return false;
}

bool FramIdentity::framWrite(uint16_t /*addr*/, const uint8_t* /*buf*/, size_t /*len*/) {
  // TODO: MB85RC256V sequential write
  return false;
}
