#pragma once

#include <Arduino.h>

/**
 * DarkTrack — memristor-lite isolation state for the BPW array.
 *
 * Stance is voltage (emitters OFF). Current/dark residual is the witness.
 * State q integrates (raw - baseline) and leaks. That q is the track.
 *
 * Not a discrete memristor IC. The diode + TIA + this integrator is the memory.
 *
 * Isolation rule: do not accept a one-hot optical row while any channel
 * is CHARGE or FAULT. Wait in allOff until HOLD/RELAX, or mark degraded.
 */
enum DarkPhase : uint8_t {
  DARK_HOLD  = 0,
  DARK_CHARGE = 1,
  DARK_RELAX = 2,
  DARK_FAULT = 3
};

struct DarkCell {
  float     baseline;
  float     q;
  float     last_raw;
  DarkPhase phase;
};

class DarkTrack {
public:
  static constexpr int MAX_N = 20;
  static constexpr float ETA      = 0.35f;
  static constexpr float LEAK     = 0.08f;
  static constexpr float HOLD_ABS = 0.015f;
  static constexpr float CHARGE_UP = 0.025f;
  static constexpr float FAULT_ABS = 0.12f;

  DarkTrack();

  void reset();
  void setBaseline(const float* dark, uint8_t n);
  void update(const float* raw_dark, uint8_t n);

  uint8_t count() const { return n_; }
  float effective(uint8_t i) const;
  float baseline(uint8_t i) const;
  float q(uint8_t i) const;
  DarkPhase phase(uint8_t i) const;

  bool isolationOk() const;
  uint8_t faultMask() const;
  float meanQ() const;
  const char* phaseName(DarkPhase p) const;

private:
  DarkCell cell_[MAX_N];
  uint8_t n_ = 0;
};
