#pragma once

#include <Arduino.h>

/**
 * MuxController — CD74HC4067 16-channel analog multiplexer
 *
 * Select channel 0..15 before reading the corresponding ADS1115 input.
 * Default pin map targets free GPIOs on ESP32-S3-DevKitC-1.
 * Override via build flags if your board differs:
 *
 *   -D MUX_S0_PIN=4 -D MUX_S1_PIN=5 -D MUX_S2_PIN=6 -D MUX_S3_PIN=7
 *   -D MUX_EN_PIN=15          (optional; set to -1 to leave EN floating/high)
 *
 * Typical wiring:
 *   CD74HC4067 SIG  → ADS1115 A0
 *   CD74HC4067 S0-S3 → the pins below
 *   CD74HC4067 EN   → GND (always enabled) or the EN pin (active-LOW)
 */
class MuxController {
public:
  // Default pins (override with build_flags if needed)
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
#define MUX_EN_PIN 15   // -1 = do not drive EN (tie EN to GND on the module)
#endif

  bool begin();

  /** Select channel 0..15. Settles for a few microseconds. */
  void select(uint8_t channel);

  /** Force all select lines low (channel 0) and disable if EN is driven. */
  void disable();

private:
  static constexpr uint8_t S0 = (uint8_t)MUX_S0_PIN;
  static constexpr uint8_t S1 = (uint8_t)MUX_S1_PIN;
  static constexpr uint8_t S2 = (uint8_t)MUX_S2_PIN;
  static constexpr uint8_t S3 = (uint8_t)MUX_S3_PIN;
  static constexpr int     EN = MUX_EN_PIN;   // may be -1

  bool ready_ = false;
};
