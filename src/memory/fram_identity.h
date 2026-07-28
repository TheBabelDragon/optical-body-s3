#pragma once

#include <Arduino.h>

/**
 * FramIdentity — persistent calibration memory ("Who am I?")
 *
 * Hardware: MB85RC256V I²C FRAM
 *
 * Stores:
 *   - node_id / geometry_version
 *   - per-laser expected detector signatures (transfer matrix slice)
 *   - per-detector gain / offset corrections
 *
 * Survives power loss. Written after a successful self-map or
 * explicit calibration run. Read on every boot.
 */
class FramIdentity {
public:
  static constexpr uint16_t MAX_LASERS    = 40;
  static constexpr uint16_t MAX_DETECTORS = 100;

  bool begin(uint8_t i2c_addr = 0x50);

  /** Load identity from FRAM. Returns false if empty or corrupt. */
  bool load();

  /** Persist current identity to FRAM. */
  bool save();

  /** Clear FRAM identity (forces a fresh self-map on next boot). */
  bool clear();

  // --- Accessors used by OpticalBody ---
  bool hasIdentity() const { return valid_; }
  const char* geometryVersion() const { return geometry_version_; }

  void setExpected(uint16_t laser, uint16_t detector, float value);
  float getExpected(uint16_t laser, uint16_t detector) const;

  void setDetectorGain(uint16_t detector, float gain);
  void setDetectorOffset(uint16_t detector, float offset);
  float getDetectorGain(uint16_t detector) const;
  float getDetectorOffset(uint16_t detector) const;

  void setGeometryVersion(const char* ver);

private:
  bool valid_ = false;
  char geometry_version_[32] = "uninitialized";

  // Compact in-RAM mirror of the FRAM contents
  float expected_[MAX_LASERS][MAX_DETECTORS];
  float gain_[MAX_DETECTORS];
  float offset_[MAX_DETECTORS];

  // TODO: actual MB85RC256V read/write helpers
  bool framRead(uint16_t addr, uint8_t* buf, size_t len);
  bool framWrite(uint16_t addr, const uint8_t* buf, size_t len);
};
