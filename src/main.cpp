/**
 * optical-body-s3 / main.cpp
 *
 * First milestone boot path:
 *   1. Init drivers + FRAM + SD
 *   2. If identity exists → verifyIdentity (sparse probe)
 *        Body unchanged  → skip full map
 *        Geometry drift  → full clean calibration
 *   3. If no identity → full clean calibration
 *   4. Passive observation loop
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

  bool need_map = true;
  if (body.hasStoredIdentity()) {
    bool unchanged = false;
    body.verifyIdentity(&unchanged, 0.08f);
    need_map = !unchanged;
  } else {
    Serial.println(F("[Boot] no stored identity — first calibration"));
  }

  if (need_map) {
    Serial.println(F("[Boot] running clean calibration…"));
    body.runSelfMap();
  } else {
    Serial.println(F("[Boot] identity trusted — skipping full self-map"));
  }

  Serial.println(F("[Boot] entering passive observation loop"));
}

void loop() {
  body.tickPassive();
  delay(200);
}
