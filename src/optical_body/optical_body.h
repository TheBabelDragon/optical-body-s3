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
 * Boot path for first milestone:
 *   load identity → verifyIdentity (probe) → body unchanged | drift → self-map if needed
 *
 * Calibration:
 *   dark frame → ExcitationSequence → OpticalFingerprint → FRAM
 */
class OpticalBody {
public:
  explicit OpticalBody(const char* node_id);

  bool begin();

  /**
   * Probe a few sources against stored expected responses.
   * Returns mean residual (0 = perfect match).
   * Sets out_unchanged true when residual below threshold.
   */
  float verifyIdentity(bool* out_unchanged, float threshold = 0.08f);

  void runSelfMap();
  void tickPassive();

  const char* nodeId() const { return node_id_; }
  bool isCalibrated() const { return calibrated_; }
  bool hasStoredIdentity() const { return identity_.hasIdentity(); }
  const OpticalFingerprint& fingerprint() const { return fingerprint_; }

private:
  const char* node_id_;
  bool calibrated_ = false;
  uint32_t excitation_counter_ = 0;

  FramIdentity identity_;
  SdArchive archive_;
  OpticalFingerprint fingerprint_;
  ExcitationSequence sequence_;

  static constexpr int NUM_LASERS    = 12;
  static constexpr int NUM_DETECTORS = 20;

  float dark_[NUM_DETECTORS];
  float transfer_[NUM_LASERS][NUM_DETECTORS];

  void clearTransfer();
  bool acquireDarkFrame();
  bool readAveraged(float* out, size_t n, uint16_t samples);
  void emitObservation(uint16_t laser_id, const float* detectors, size_t n,
                       bool dark_corrected);
};
