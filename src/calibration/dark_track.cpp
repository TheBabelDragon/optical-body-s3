#include "dark_track.h"
#include <math.h>

DarkTrack::DarkTrack() { reset(); }

void DarkTrack::reset() {
  n_ = 0;
  for (int i = 0; i < MAX_N; ++i) {
    cell_[i].baseline = 0.0f;
    cell_[i].q = 0.0f;
    cell_[i].last_raw = 0.0f;
    cell_[i].phase = DARK_HOLD;
  }
}

void DarkTrack::setBaseline(const float* dark, uint8_t n) {
  if (!dark) return;
  if (n > MAX_N) n = MAX_N;
  n_ = n;
  for (uint8_t i = 0; i < n_; ++i) {
    cell_[i].baseline = dark[i];
    cell_[i].q = 0.0f;
    cell_[i].last_raw = dark[i];
    cell_[i].phase = DARK_HOLD;
  }
}

void DarkTrack::update(const float* raw_dark, uint8_t n) {
  if (!raw_dark) return;
  if (n > n_ && n <= MAX_N) n_ = n;
  if (n > n_) n = n_;

  for (uint8_t i = 0; i < n; ++i) {
    const float raw = raw_dark[i];
    const float resid = raw - cell_[i].baseline;
    const float dq = raw - cell_[i].last_raw;

    cell_[i].q += ETA * resid;
    cell_[i].q *= (1.0f - LEAK);

    if (fabsf(cell_[i].q) >= FAULT_ABS || fabsf(resid) >= FAULT_ABS) {
      cell_[i].phase = DARK_FAULT;
    } else if (resid > CHARGE_UP && dq > 0.0f) {
      cell_[i].phase = DARK_CHARGE;
    } else if (fabsf(cell_[i].q) > HOLD_ABS) {
      cell_[i].phase = DARK_RELAX;
    } else {
      cell_[i].phase = DARK_HOLD;
    }

    cell_[i].last_raw = raw;
  }
}

float DarkTrack::effective(uint8_t i) const {
  if (i >= n_) return 0.0f;
  float v = cell_[i].baseline + cell_[i].q;
  if (v < 0.0f) v = 0.0f;
  return v;
}

float DarkTrack::baseline(uint8_t i) const {
  return (i < n_) ? cell_[i].baseline : 0.0f;
}

float DarkTrack::q(uint8_t i) const {
  return (i < n_) ? cell_[i].q : 0.0f;
}

DarkPhase DarkTrack::phase(uint8_t i) const {
  return (i < n_) ? cell_[i].phase : DARK_FAULT;
}

bool DarkTrack::isolationOk() const {
  if (n_ == 0) return false;
  for (uint8_t i = 0; i < n_; ++i) {
    if (cell_[i].phase == DARK_CHARGE || cell_[i].phase == DARK_FAULT)
      return false;
  }
  return true;
}

uint8_t DarkTrack::faultMask() const {
  uint8_t m = 0;
  uint8_t lim = n_ < 8 ? n_ : 8;
  for (uint8_t i = 0; i < lim; ++i) {
    if (cell_[i].phase == DARK_FAULT) m |= (uint8_t)(1u << i);
  }
  return m;
}

float DarkTrack::meanQ() const {
  if (n_ == 0) return 0.0f;
  float s = 0.0f;
  for (uint8_t i = 0; i < n_; ++i) s += cell_[i].q;
  return s / (float)n_;
}

const char* DarkTrack::phaseName(DarkPhase p) const {
  switch (p) {
    case DARK_HOLD:   return "hold";
    case DARK_CHARGE: return "charge";
    case DARK_RELAX:  return "relax";
    case DARK_FAULT:  return "fault";
    default:          return "?";
  }
}
