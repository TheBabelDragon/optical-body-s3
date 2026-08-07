#include "mux_controller.h"

MuxController::MuxController()
  : s0_(MUX_S0_PIN), s1_(MUX_S1_PIN), s2_(MUX_S2_PIN), s3_(MUX_S3_PIN),
    en_(MUX_EN_PIN) {}

MuxController::MuxController(uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, int en)
  : s0_(s0), s1_(s1), s2_(s2), s3_(s3), en_(en) {}

bool MuxController::begin() {
  pinMode(s0_, OUTPUT);
  pinMode(s1_, OUTPUT);
  pinMode(s2_, OUTPUT);
  pinMode(s3_, OUTPUT);

  if (en_ >= 0) {
    pinMode((uint8_t)en_, OUTPUT);
    digitalWrite((uint8_t)en_, LOW);   // active-LOW → enabled
  }

  digitalWrite(s0_, LOW);
  digitalWrite(s1_, LOW);
  digitalWrite(s2_, LOW);
  digitalWrite(s3_, LOW);

  ready_ = true;
  Serial.print(F("[Mux] CD74HC4067  S0="));
  Serial.print(s0_);
  Serial.print(F(" S1="));
  Serial.print(s1_);
  Serial.print(F(" S2="));
  Serial.print(s2_);
  Serial.print(F(" S3="));
  Serial.print(s3_);
  if (en_ >= 0) {
    Serial.print(F(" EN="));
    Serial.print(en_);
  } else {
    Serial.print(F(" EN=tied"));
  }
  Serial.println();
  return true;
}

void MuxController::select(uint8_t channel) {
  if (!ready_) return;
  channel &= 0x0F;

  digitalWrite(s0_, (channel & 0x01) ? HIGH : LOW);
  digitalWrite(s1_, (channel & 0x02) ? HIGH : LOW);
  digitalWrite(s2_, (channel & 0x04) ? HIGH : LOW);
  digitalWrite(s3_, (channel & 0x08) ? HIGH : LOW);

  delayMicroseconds(5);   // switch + analog settle
}

void MuxController::disable() {
  if (!ready_ || en_ < 0) return;
  digitalWrite((uint8_t)en_, HIGH);
}

void MuxController::enable() {
  if (!ready_ || en_ < 0) return;
  digitalWrite((uint8_t)en_, LOW);
}
