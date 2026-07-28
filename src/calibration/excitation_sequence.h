#pragma once

#include <Arduino.h>

/**
 * Formal excitation experiment description.
 *
 * Hardware stays the same; the experiment changes.
 *
 * Calibration v1: one-hot lasers
 * Calibration v2: pairs / combinations
 * Calibration v3: pseudo-random patterns
 */

struct ExcitationStep {
  uint8_t  source_id;       // laser / LED index (0-based)
  uint16_t settle_time_ms;  // wait after fire before sampling
  uint16_t samples;         // how many ADC frames to average
  uint8_t  repeats;         // how many times to repeat this step
};

class ExcitationSequence {
public:
  static constexpr int MAX_STEPS = 64;

  ExcitationSequence();

  /** Clear and rebuild as one-hot sequential (laser 0 .. n-1). */
  void buildOneHot(uint8_t num_sources,
                   uint16_t settle_ms = 15,
                   uint16_t samples = 1,
                   uint8_t repeats = 1);

  int numSteps() const { return num_steps_; }
  const ExcitationStep& step(int i) const { return steps_[i]; }

  /** Optional label for logging / geometry_version. */
  void setLabel(const char* label);
  const char* label() const { return label_; }

private:
  ExcitationStep steps_[MAX_STEPS];
  int num_steps_ = 0;
  char label_[32] = "onehot-v1";
};
