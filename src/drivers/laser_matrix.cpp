#include "laser_matrix.h"

// Default pin map (all -1 = not wired yet).
// Override any of these in platformio.ini build_flags:
//   -D LASER_PIN_0=10 -D LASER_PIN_1=11 ...
#ifndef LASER_PIN_0
#define LASER_PIN_0 -1
#endif
#ifndef LASER_PIN_1
#define LASER_PIN_1 -1
#endif
#ifndef LASER_PIN_2
#define LASER_PIN_2 -1
#endif
#ifndef LASER_PIN_3
#define LASER_PIN_3 -1
#endif
#ifndef LASER_PIN_4
#define LASER_PIN_4 -1
#endif
#ifndef LASER_PIN_5
#define LASER_PIN_5 -1
#endif
#ifndef LASER_PIN_6
#define LASER_PIN_6 -1
#endif
#ifndef LASER_PIN_7
#define LASER_PIN_7 -1
#endif
#ifndef LASER_PIN_8
#define LASER_PIN_8 -1
#endif
#ifndef LASER_PIN_9
#define LASER_PIN_9 -1
#endif
#ifndef LASER_PIN_10
#define LASER_PIN_10 -1
#endif
#ifndef LASER_PIN_11
#define LASER_PIN_11 -1
#endif

void LaserMatrix::initPinMap() {
  pins_[0]  = LASER_PIN_0;
  pins_[1]  = LASER_PIN_1;
  pins_[2]  = LASER_PIN_2;
  pins_[3]  = LASER_PIN_3;
  pins_[4]  = LASER_PIN_4;
  pins_[5]  = LASER_PIN_5;
  pins_[6]  = LASER_PIN_6;
  pins_[7]  = LASER_PIN_7;
  pins_[8]  = LASER_PIN_8;
  pins_[9]  = LASER_PIN_9;
  pins_[10] = LASER_PIN_10;
  pins_[11] = LASER_PIN_11;
  // remaining stay -1
  for (int i = 12; i < MAX_LASERS; ++i) pins_[i] = -1;
}

bool LaserMatrix::begin() {
  initPinMap();

  int wired = 0;
  for (int i = 0; i < num_lasers_; ++i) {
    if (pins_[i] >= 0) {
      pinMode((uint8_t)pins_[i], OUTPUT);
      digitalWrite((uint8_t)pins_[i], LOW);
      wired++;
    }
  }

  Serial.print(F("[Laser] matrix ready  lasers="));
  Serial.print(num_lasers_);
  Serial.print(F("  wired pins="));
  Serial.println(wired);

  allOff();
  return true;
}

void LaserMatrix::allOff() {
  for (int i = 0; i < num_lasers_; ++i) {
    if (pins_[i] >= 0) {
      digitalWrite((uint8_t)pins_[i], LOW);
    }
  }
}

void LaserMatrix::fire(uint16_t laser_id) {
  if (laser_id >= (uint16_t)num_lasers_) return;

  allOff();

  int pin = pins_[laser_id];
  if (pin >= 0) {
    digitalWrite((uint8_t)pin, HIGH);
  }

  // Always log so the self-map sequence is visible even before pins are assigned
  Serial.print(F("[Laser] fire "));
  Serial.print(laser_id);
  if (pin < 0) Serial.print(F(" (pin not wired)"));
  Serial.println();
}
