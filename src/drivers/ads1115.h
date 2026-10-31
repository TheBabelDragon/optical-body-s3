#pragma once

/**
 * Thin ADS1115 driver — no Adafruit dependency.
 *
 * Covers single-ended / differential reads, gain, data rate, and
 * conversion-ready poll. Register map from TI ADS1115 datasheet.
 */

#include <Arduino.h>
#include <Wire.h>

class ADS1115 {
public:
  enum class Gain : uint16_t {
    PGA_6_144V = 0x0000,  // ±6.144 V  (2/3x)
    PGA_4_096V = 0x0200,  // ±4.096 V
    PGA_2_048V = 0x0400,  // ±2.048 V  (default)
    PGA_1_024V = 0x0600,  // ±1.024 V
    PGA_0_512V = 0x0800,  // ±0.512 V
    PGA_0_256V = 0x0A00,  // ±0.256 V
  };

  enum class Rate : uint16_t {
    SPS_8   = 0x0000,
    SPS_16  = 0x0020,
    SPS_32  = 0x0040,
    SPS_64  = 0x0060,
    SPS_128 = 0x0080,  // default
    SPS_250 = 0x00A0,
    SPS_475 = 0x00C0,
    SPS_860 = 0x00E0,
  };

  enum class Mux : uint16_t {
    DIFF_0_1 = 0x0000,
    DIFF_0_3 = 0x1000,
    DIFF_1_3 = 0x2000,
    DIFF_2_3 = 0x3000,
    SINGLE_0 = 0x4000,
    SINGLE_1 = 0x5000,
    SINGLE_2 = 0x6000,
    SINGLE_3 = 0x7000,
  };

  explicit ADS1115(uint8_t address = 0x48, TwoWire& wire = Wire);

  /** Probe I2C and apply default config (gain 1 = ±4.096 V, 128 SPS). */
  bool begin();

  void setGain(Gain g);
  void setDataRate(Rate r);

  /** Single-ended channel 0–3. Blocks until conversion ready. */
  int16_t readSingleEnded(uint8_t channel);

  /** Differential AIN0 − AIN1 (common for mux SIG vs GND). */
  int16_t readDifferential_0_1();

  /** Convert raw counts to volts for the current gain. */
  float computeVolts(int16_t raw) const;

  uint8_t address() const { return address_; }
  bool present() const { return present_; }

private:
  static constexpr uint8_t REG_CONVERSION = 0x00;
  static constexpr uint8_t REG_CONFIG     = 0x01;

  uint8_t address_;
  TwoWire* wire_;
  Gain gain_ = Gain::PGA_4_096V;
  Rate rate_ = Rate::SPS_128;
  bool present_ = false;

  bool writeRegister(uint8_t reg, uint16_t value);
  bool readRegister(uint8_t reg, uint16_t& value);
  int16_t readConversion(Mux mux);
  float lsbVolts() const;
};
