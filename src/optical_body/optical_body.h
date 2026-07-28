#pragma once

#include <Arduino.h>
#include <vector>
#include "../protocol/field_observation.h"
#include "../memory/fram_identity.h"
#include "../memory/sd_archive.h"
#include "../calibration/excitation_sequence.h"
#include "../calibration/optical_fingerprint.h"

/**
 * OpticalBody
 *
 * Phase 0 scientifically clean calibration:
 *   1. Dark frame (all emitters OFF)
 *   2. Formal ExcitationSequence (one-hot by default)
 *   3. Dark-corrected transfer matrix → OpticalFingerprint → FRAM
 *
 * Does NOT run MetaField. Only produces observation packets + identity.
 */
class OpticalBody {
public:
  explicit OpticalBody(const char* node_id);

  bool begin();
  void runSelfMap();
  void tickPassive();

  const char* nodeId() const { return node_id_; }
  bool isCalibrated() const { return calibrated_; }
  const OpticalFingerprint& fingerprint() const { return fingerprint_; }

private:
  const char* node_id_;
  bool calibrated_ = false;
  uint32_t excitation_counter_ = 0;

  FramIdentity identity_;
  SdArchive archive_;
  OpticalFingerprint fingerprint_;
  ExcitationSequence sequence_;

  static constexpr int NUM_LASERS    = 12;   // adjust to real wiring
  static constexpr int NUM_DETECTORS = 20;   // adjust to real wiring

  float dark_[NUM_DETECTORS];
  float transfer_[NUM_LASERS][NUM_DETECTORS];

  void clearTransfer();
  bool acquireDarkFrame();
  bool readAveraged(float* out, size_t n, uint16_t samples);
  void emitObservation(uint16_t laser_id, const float* detectors, size_t n,
                       bool dark_corrected);
};
