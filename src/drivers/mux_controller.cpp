#include "mux_controller.h"

bool MuxController::begin() {
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);

  if (EN >= 0) {
    pinMode((uint8_t)EN, OUTPUT);
    digitalWrite((uint8_t)EN, LOW);   // active-LOW enable
  }

  // Start on channel 0
  digitalWrite(S0, LOW);
  digitalWrite(S1, LOW);
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);

  ready_ = true;
  Serial.print(F("[Mux] CD74HC4067 ready  S0="));
  Serial.print(S0);
  Serial.print(F(" S1="));
  Serial.print(S1);
  Serial.print(F(" S2="));
  Serial.print(S2);
  Serial.print(F(" S3="));
  Serial.print(S3);
  if (EN >= 0) {
    Serial.print(F(" EN="));
    Serial.print(EN);
  } else {
    Serial.print(F(" EN=tied"));
  }
  Serial.println();
  return true;
}

void MuxController::select(uint8_t channel) {
  if (!ready_) return;
  channel &= 0x0F;   // 0..15

  digitalWrite(S0, (channel & 0x01) ? HIGH : LOW);
  digitalWrite(S1, (channel & 0x02) ? HIGH : LOW);
  digitalWrite(S2, (channel & 0x04) ? HIGH : LOW);
  digitalWrite(S3, (channel & 0x08) ? HIGH : LOW);

  // CD74HC4067 switch time is typically <1 µs; give a comfortable margin
  // for breadboard capacitance and any analog settling.
  delayMicroseconds(5);
}

void MuxController::disable() {
  if (!ready_) return;
  digitalWrite(S0, LOW);
  digitalWrite(S1, LOW);
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  if (EN >= 0) {
    digitalWrite((uint8_t)EN, HIGH);  // disable
  }
}
