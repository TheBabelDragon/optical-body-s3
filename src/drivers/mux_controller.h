#pragma once

#include <Arduino.h>

/**
 * MuxController — one CD74HC4067 16-channel analog multiplexer
 *
 * Construct with the four select pins + optional EN pin.
 * Default constructor uses the build-flag pins (or the hard-coded defaults).
 *
 * You already have multiple identical blue mux boards — just create more
 * MuxController instances with different EN pins (share the same S0-S3).
 *
 * Typical wiring for one board:
 *   SIG  → ADS1115 A0 (or A1/A2/A3 for extra muxes)
 *   S0-S3 → GPIO 4/5/6/7 (shared across all muxes)
 *   EN   → unique GPIO (active-LOW) or tie to GND and pass -1
 */
class MuxController {
public:
  // Defaults (override with -D MUX_S0_PIN=... etc.)
#ifndef MUX_S0_PIN
#define MUX_S0_PIN 4
#endif
#ifndef MUX_S1_PIN
#define MUX_S1_PIN 5
#endif
#ifndef MUX_S2_PIN
#define MUX_S2_PIN 6
#endif
#ifndef MUX_S3_PIN
#define MUX_S3_PIN 7
#endif
#ifndef MUX_EN_PIN
#define MUX_EN_PIN 15   // -1 = do not drive EN
#endif

  /** Default pins from build flags. */
  MuxController();

  /** Explicit pins — use this when you have more than one mux board. */
  MuxController(uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, int en = -1);

  bool begin();

  /** Select channel 0..15. Settles a few µs. */
  void select(uint8_t channel);

  /** Drive EN high (disable) if we own the EN pin. */
  void disable();

  /** Drive EN low (enable). */
  void enable();

private:
  uint8_t s0_, s1_, s2_, s3_;
  int     en_;          // -1 = not driven
  bool    ready_ = false;
};
