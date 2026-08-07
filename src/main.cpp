/**
 * optical-body-s3 / main.cpp
 *
 * Boot: identity verify → map only if needed
 * Loop: passive OR host commands (EXCITE / MAP / VERIFY / PASSIVE / DUMP)
 *
 * Closing the circle:
 *   MetaField active_probe → "EXCITE N" on Serial → body shapes light
 */

#include <Arduino.h>
#include "optical_body/optical_body.h"
#include "protocol/command_parser.h"

#ifndef OPTICAL_BODY_NODE_ID
#define OPTICAL_BODY_NODE_ID "optical_s3_001"
#endif

OpticalBody body(OPTICAL_BODY_NODE_ID);

enum class RunMode : uint8_t { Passive, Held };
static RunMode mode = RunMode::Passive;

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("  optical-body-s3  —  MetaField body"));
  Serial.println(F("  cmds: EXCITE <id> | MAP | VERIFY | PASSIVE | DUMP"));
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

  Serial.println(F("[Boot] passive loop (send EXCITE n / DUMP / etc.)"));
}

void loop() {
  ParsedCommand cmd;
  if (pollCommand(cmd)) {
    switch (cmd.type) {
      case BodyCommand::Excite:
        if (cmd.source_id < 0 || cmd.source_id >= body.numLasers()) {
          Serial.println(F("[CMD] EXCITE id out of range"));
        } else {
          Serial.print(F("[CMD] shaping light source "));
          Serial.println(cmd.source_id);
          body.exciteOnce((uint16_t)cmd.source_id);
          mode = RunMode::Held;
        }
        break;
      case BodyCommand::Map:
        Serial.println(F("[CMD] full calibration"));
        body.runSelfMap();
        break;
      case BodyCommand::Verify: {
        bool ok = false;
        body.verifyIdentity(&ok, 0.08f);
        break;
      }
      case BodyCommand::Passive:
        Serial.println(F("[CMD] passive resume"));
        mode = RunMode::Passive;
        break;
      case BodyCommand::Dump:
        Serial.println(F("[CMD] raw ADC dump"));
        body.dumpRaw(8);
        break;
      default:
        break;
    }
  }

  if (mode == RunMode::Passive) {
    body.tickPassive();
    delay(200);
  } else {
    delay(50);
  }
}
