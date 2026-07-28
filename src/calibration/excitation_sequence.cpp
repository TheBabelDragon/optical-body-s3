#include "excitation_sequence.h"
#include <string.h>

ExcitationSequence::ExcitationSequence() {
  num_steps_ = 0;
}

void ExcitationSequence::buildOneHot(uint8_t num_sources,
                                     uint16_t settle_ms,
                                     uint16_t samples,
                                     uint8_t repeats) {
  num_steps_ = 0;
  uint8_t n = num_sources;
  if (n > MAX_STEPS) n = MAX_STEPS;

  for (uint8_t i = 0; i < n; ++i) {
    steps_[num_steps_].source_id      = i;
    steps_[num_steps_].settle_time_ms = settle_ms;
    steps_[num_steps_].samples        = samples > 0 ? samples : 1;
    steps_[num_steps_].repeats        = repeats > 0 ? repeats : 1;
    num_steps_++;
  }
  setLabel("onehot-v1");
}

void ExcitationSequence::setLabel(const char* label) {
  strncpy(label_, label, sizeof(label_) - 1);
  label_[sizeof(label_) - 1] = '\0';
}
