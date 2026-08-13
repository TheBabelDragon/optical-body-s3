/**
 * optical-body-s3 / main.cpp
 *
 * Boot: Field Bus HELLO → identity verify → map if needed
 * Loop: passive OR host commands + Field Bus heartbeat / RX + optional UI
 *
 * Pin map: see WIRING_PHASE0.md (dictated, not interactive)
 * UI:    see HARDWARE_PINOUT.md (DKARDU section) when OPTICAL_UI=1
 */

#include <Arduino.h>
#include <Wire.h>
#include "optical_body/optical_body.h"
#include "protocol/command_parser.h"
#include "field_bus/field_bus_node.h"
#include "ui/display_ui.h"

#ifndef OPTICAL_BODY_NODE_ID
#define OPTICAL_BODY_NODE_ID "optical_s3_001"
#endif

#ifndef FB_FIRMWARE_VER
#define FB_FIRMWARE_VER 1
#endif

#ifndef WIRE_SDA_PIN
#define WIRE_SDA_PIN 8
#endif
#ifndef WIRE_SCL_PIN
#define WIRE_SCL_PIN 9
#endif

OpticalBody  body(OPTICAL_BODY_NODE_ID);
FieldBusNode bus(FB_NODE_OPTICAL);   // 0x02
DisplayUI    ui;

enum class RunMode : uint8_t { Passive, Held };
static RunMode mode = RunMode::Passive;

void setup() {
  Serial.begin(115200);
  delay(800);

  Serial.println();
  Serial.println(F("========================================"));
  Serial.println(F("  optical-body-s3  —  MetaField body"));
  Serial.println(F("  cmds: EXCITE <id> | MAP | VERIFY | PASSIVE | DUMP"));
  Serial.println(F("  bus : Field Bus node 0x02 (Optical)"));
#if defined(OPTICAL_UI) && OPTICAL_UI
  Serial.println(F("  ui  : DKARDU OLED + EC11 enabled"));
#endif
  Serial.println(F("========================================"));
  Serial.print(F("Node ID : "));
  Serial.println(OPTICAL_BODY_NODE_ID);

  // Locked I²C pins (WIRING_PHASE0.md)
  Wire.begin(WIRE_SDA_PIN, WIRE_SCL_PIN);
  Serial.print(F("[I2C] SDA=GPIO"));
  Serial.print(WIRE_SDA_PIN);
  Serial.print(F(" SCL=GPIO"));
  Serial.println(WIRE_SCL_PIN);

  // --- Field Bus (non-fatal if MCP2518FD not present yet) ---
  bus.begin();
  uint8_t caps = FB_CAP_OPTICAL | FB_CAP_ADC | FB_CAP_SENSOR | FB_CAP_STORAGE;
  bus.sendHello(FB_FIRMWARE_VER, caps);
  bus.setState(FB_STATE_BOOT);

  if (!body.begin()) {
    Serial.println(F("[FATAL] OpticalBody::begin() failed"));
    bus.setState(FB_STATE_ERROR);
    while (true) {
      bus.poll();
      delay(1000);
    }
  }

  // Optional UI (shares I²C)
  if (!ui.begin()) {
    Serial.println(F("[UI] continuing without display"));
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

  bus.setState(FB_STATE_READY);
  bus.sendStatus(FB_STATE_READY);
  Serial.println(F("[Boot] passive loop + Field Bus heartbeat"));
}

void loop() {
  bus.poll();

  // --- UI actions (if present) ---
  ui.tick(body);

  if (ui.requestExcite()) {
    int id = ui.exciteId();
    if (id >= 0 && id < body.numLasers()) {
      Serial.print(F("[UI] EXCITE "));
      Serial.println(id);
      body.exciteOnce((uint16_t)id);
      mode = RunMode::Held;
    }
    ui.clearExcite();
  }
  if (ui.requestMap()) {
    Serial.println(F("[UI] MAP"));
    body.runSelfMap();
    ui.clearMap();
  }
  if (ui.requestDump()) {
    Serial.println(F("[UI] DUMP"));
    body.dumpRaw(8);
    ui.clearDump();
  }
  if (ui.requestVerify()) {
    Serial.println(F("[UI] VERIFY"));
    bool ok = false;
    body.verifyIdentity(&ok, 0.08f);
    ui.clearVerify();
  }

  // Sync mode from UI if it changed
  if (ui.heldMode()) mode = RunMode::Held;
  else if (mode == RunMode::Held && !ui.heldMode()) mode = RunMode::Passive;

  // --- Serial host commands (still supported) ---
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
    delay(50);
  } else {
    delay(20);
  }
}
