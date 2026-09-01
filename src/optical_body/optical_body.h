#pragma once

#include <Arduino.h>
#include <vector>
#include "../protocol/field_observation.h"
#include "../memory/fram_identity.h"
#include "../memory/sd_archive.h"
#include "../calibration/excitation_sequence.h"
#include "../calibration/optical_fingerprint.h"
#include "../calibration/dark_track.h"

/**
 * OpticalBody
 *
 * Boot: load identity → verify → map if needed
 * Live: passive cycle OR host EXCITE <id> (shaped light)
 */
class OpticalBody {
public:
  explicit OpticalBody(const char* node_id);

  bool begin();
  float verifyIdentity(bool* out_unchanged, float threshold = 0.08f);
  void runSelfMap();
  void tickPassive();

  bool exciteOnce(uint16_t laser_id);
  void dumpRaw(uint8_t count = 8);

  const char* nodeId() const { return node_id_; }
  bool isCalibrated() const { return calibrated_; }
  bool hasStoredIdentity() const { return identity_.hasIdentity(); }
  const OpticalFingerprint& fingerprint() const { return fingerprint_; }
  int numLasers() const { return NUM_LASERS; }

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
  DarkTrack dark_track_;

  void clearTransfer();
  bool acquireDarkFrame();
  bool isolateDark(uint8_t extra_passes = 3);
  void subtractEffectiveDark(float* buf, size_t n);
  bool readAveraged(float* out, size_t n, uint16_t samples);
  void emitObservation(uint16_t laser_id, const float* detectors, size_t n,
                       bool dark_corrected);
};
