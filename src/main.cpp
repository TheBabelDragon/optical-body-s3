/**
 * optical-body-s3 / main.cpp
 *
 * Phase 0 entry point.
 * Boots, announces identity, runs a passive self-map sequence,
 * and emits FieldObservation-style packets on Serial.
 *
 * Real BPW34 values via ADS1115 are the default path.
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

  Serial.println(F("Hardware ready. Starting Phase-0 self-map…"));
  body.runSelfMap();
  Serial.println(F("Self-map complete. Entering passive observation loop."));
}

void loop() {
  // Phase 0 passive loop: one excitation at a time, emit packet, small pause.
  // Later Aurora will schedule specific sequences; for now we just keep the body alive.
  body.tickPassive();
  delay(200);
}
