/**
 * optical-body-s3 / main.cpp
 *
 * Boot path:
 *   1. Init drivers + FRAM + SD
 *   2. Clean calibration (whatsinthebox):
 *        dark frame → one-hot ExcitationSequence → dark-corrected matrix
 *        → OpticalFingerprint → FRAM
 *   3. Passive observation loop (still one source at a time)
 *
 * Real BPW34 via ADS1115 by default.
 * GPIO pin map can come later — lasers.fire(id) / detectors.readAll stay stable.
 */

#include <Arduino.h>
#include "optical_body/optical_body.h"

#ifndef OPTICAL_BODY_NODE_ID
#define OPTICAL_BODY_NODE_ID "optical_s3_001"
#endif

OpticalBody body(OPTICAL_BODY_NODE_ID);

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("  optical-body-s3  —  MetaField body"));
  Serial.println(F("========================================"));
  Serial.print(F("Node ID : "));
  Serial.println(OPTICAL_BODY_NODE_ID);

  if (!body.begin()) {
    Serial.println(F("[FATAL] OpticalBody::begin() failed"));
    while (true) delay(1000);
  }

  Serial.println(F("Hardware ready. Running clean calibration…"));
  body.runSelfMap();
  Serial.println(F("Calibration complete. Entering passive observation loop."));
}

void loop() {
  body.tickPassive();
  delay(200);
}
