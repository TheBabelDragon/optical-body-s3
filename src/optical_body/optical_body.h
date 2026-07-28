#pragma once

#include <Arduino.h>
#include <vector>
#include "../protocol/field_observation.h"

/**
 * OpticalBody
 *
 * Owns the physical optical node lifecycle for Phase 0.
 * - begin()        : init drivers, announce identity
 * - runSelfMap()   : fire every laser, record detector vectors, store fingerprint
 * - tickPassive()  : single excitation + emit FieldObservation packet
 *
 * Does NOT run MetaField. Only produces observation packets.
 */
class OpticalBody {
public:
  explicit OpticalBody(const char* node_id);

  bool begin();
  void runSelfMap();
  void tickPassive();

  const char* nodeId() const { return node_id_; }
  bool isCalibrated() const { return calibrated_; }

private:
  const char* node_id_;
  bool calibrated_ = false;
  uint32_t excitation_counter_ = 0;

  // Config (will later come from FRAM / config)
  static constexpr int NUM_LASERS    = 12;   // adjust to real wiring
  static constexpr int NUM_DETECTORS = 20;   // adjust to real wiring

  // Simple in-RAM transfer matrix for Phase 0 / early Phase 1
  // M[laser][detector]  (normalized 0..1)
  float transfer_[NUM_LASERS][NUM_DETECTORS];

  void clearTransfer();
  void emitObservation(uint16_t laser_id, const float* detectors, size_t n);
};
