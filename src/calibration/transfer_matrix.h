#pragma once

#include <Arduino.h>

/**
 * TransferMatrix
 *
 * Stores M[laser][detector] — the optical geometry fingerprint.
 * Phase 0 fills it during self-map; Phase 1 treats it as the
 * learned identity of the body.
 *
 * Later: persist to FRAM / flash so the node remembers itself
 * across power cycles.
 */
class TransferMatrix {
public:
  static constexpr int MAX_LASERS    = 40;
  static constexpr int MAX_DETECTORS = 100;

  void clear();
  void set(uint16_t laser, uint16_t detector, float value);
  float get(uint16_t laser, uint16_t detector) const;

  // Future: save / load from FRAM
  bool saveToFram() { return false; }
  bool loadFromFram() { return false; }

private:
  float data_[MAX_LASERS][MAX_DETECTORS];
};
