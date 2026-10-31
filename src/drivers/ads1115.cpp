#include "ads1115.h"

ADS1115::ADS1115(uint8_t address, TwoWire& wire)
    : address_(address), wire_(&wire) {}

bool ADS1115::begin() {
  present_ = false;
  wire_->beginTransmission(address_);
  if (wire_->endTransmission() != 0) {
    return false;
  }
  uint16_t cfg = 0x8000;
  cfg |= static_cast<uint16_t>(Mux::SINGLE_0);
  cfg |= static_cast<uint16_t>(gain_);
  cfg |= static_cast<uint16_t>(rate_);
  cfg |= 0x0003;
  if (!writeRegister(REG_CONFIG, cfg)) {
    return false;
  }
  present_ = true;
  return true;
}

void ADS1115::setGain(Gain g) { gain_ = g; }
void ADS1115::setDataRate(Rate r) { rate_ = r; }

bool ADS1115::writeRegister(uint8_t reg, uint16_t value) {
  wire_->beginTransmission(address_);
  wire_->write(reg);
  wire_->write(static_cast<uint8_t>((value >> 8) & 0xFF));
  wire_->write(static_cast<uint8_t>(value & 0xFF));
  return wire_->endTransmission() == 0;
}

bool ADS1115::readRegister(uint8_t reg, uint16_t& value) {
  wire_->beginTransmission(address_);
  wire_->write(reg);
  if (wire_->endTransmission(false) != 0) return false;
  if (wire_->requestFrom(address_, static_cast<uint8_t>(2)) != 2) return false;
  value = (static_cast<uint16_t>(wire_->read()) << 8) |
          static_cast<uint16_t>(wire_->read());
  return true;
}

float ADS1115::lsbVolts() const {
  switch (gain_) {
    case Gain::PGA_6_144V: return 6.144f / 32768.0f;
    case Gain::PGA_4_096V: return 4.096f / 32768.0f;
    case Gain::PGA_2_048V: return 2.048f / 32768.0f;
    case Gain::PGA_1_024V: return 1.024f / 32768.0f;
    case Gain::PGA_0_512V: return 0.512f / 32768.0f;
    case Gain::PGA_0_256V: return 0.256f / 32768.0f;
  }
  return 4.096f / 32768.0f;
}

float ADS1115::computeVolts(int16_t raw) const {
  return static_cast<float>(raw) * lsbVolts();
}

int16_t ADS1115::readConversion(Mux mux) {
  uint16_t cfg = 0x8000;
  cfg |= static_cast<uint16_t>(mux);
  cfg |= static_cast<uint16_t>(gain_);
  cfg |= 0x0100;
  cfg |= static_cast<uint16_t>(rate_);
  cfg |= 0x0003;
  if (!writeRegister(REG_CONFIG, cfg)) {
    return 0;
  }

  const uint32_t deadline = millis() + 50;
  while (millis() < deadline) {
    uint16_t status = 0;
    if (!readRegister(REG_CONFIG, status)) break;
    if (status & 0x8000) {
      uint16_t raw = 0;
      if (!readRegister(REG_CONVERSION, raw)) return 0;
      return static_cast<int16_t>(raw);
    }
    delayMicroseconds(100);
  }
  return 0;
}

int16_t ADS1115::readSingleEnded(uint8_t channel) {
  Mux mux;
  switch (channel & 0x03) {
    case 0: mux = Mux::SINGLE_0; break;
    case 1: mux = Mux::SINGLE_1; break;
    case 2: mux = Mux::SINGLE_2; break;
    default: mux = Mux::SINGLE_3; break;
  }
  return readConversion(mux);
}

int16_t ADS1115::readDifferential_0_1() {
  return readConversion(Mux::DIFF_0_1);
}
