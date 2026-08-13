/**
 * optical-body-s3 / main.cpp
 *
 * SAFE_BOOT=1 → minimal diagnostic that forces Serial output
 * as early and as hard as possible on ESP32-S3 USB-CDC.
 */

#include <Arduino.h>

#if defined(SAFE_BOOT) && SAFE_BOOT

void setup() {
  // Force USB CDC Serial on ESP32-S3
  Serial.begin(115200);

  // Long settle time — some S3 boards need this
  delay(3000);

  // Spam the serial port so something appears even if
  // the first few packets are lost during enumeration
  for (int i = 0; i < 20; i++) {
    Serial.println();
    Serial.println(F("==== SAFE BOOT ALIVE ===="));
    Serial.print(F("count = "));
    Serial.println(i);
    Serial.flush();
    delay(200);
  }

  Serial.println(F("========================================"));
  Serial.println(F("  optical-body-s3  —  SAFE BOOT"));
  Serial.println(F("  If you see this, USB + flash work."));
  Serial.println(F("========================================"));
  Serial.flush();
}

void loop() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last >= 1000) {
    last = now;
    Serial.print(F("[SAFE] alive  ms="));
    Serial.println(now);
    Serial.flush();
  }
  delay(5);
}

#else

// Full firmware path (unchanged from previous version)
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
FieldBusNode bus(FB_NODE_OPTICAL);
DisplayUI    ui;

enum class RunMode : uint8_t { Passive, Held };
static RunMode mode = RunMode::Passive;

void setup() {
  Serial.begin(115200);
  delay(1000);
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
  Serial.flush();

  Serial.println(F("[Boot] starting I2C..."));
  Serial.flush();
  Wire.begin(WIRE_SDA_PIN, WIRE_SCL_PIN);
  Serial.print(F("[I2C] SDA=GPIO"));
  Serial.print(WIRE_SDA_PIN);
  Serial.print(F(" SCL=GPIO"));
  Serial.println(WIRE_SCL_PIN);
  Serial.flush();

  Serial.println(F("[Boot] Field Bus begin..."));
  Serial.flush();
  bus.begin();
  uint8_t caps = FB_CAP_OPTICAL | FB_CAP_ADC | FB_CAP_SENSOR | FB_CAP_STORAGE;
  bus.sendHello(FB_FIRMWARE_VER, caps);
  bus.setState(FB_STATE_BOOT);

  Serial.println(F("[Boot] OpticalBody begin..."));
  Serial.flush();
  if (!body.begin()) {
    Serial.println(F("[FATAL] OpticalBody::begin() failed"));
    bus.setState(FB_STATE_ERROR);
    while (true) { bus.poll(); delay(1000); }
  }

  Serial.println(F("[Boot] UI begin..."));
  Serial.flush();
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
  Serial.flush();
}

void loop() {
  bus.poll();
  ui.tick(body);

  if (ui.requestExcite()) {
    int id = ui.exciteId();
    if (id >= 0 && id < body.numLasers()) {
      body.exciteOnce((uint16_t)id);
      mode = RunMode::Held;
    }
    ui.clearExcite();
  }
  if (ui.requestMap()) {
    body.runSelfMap();
    ui.clearMap();
  }
  if (ui.requestDump()) {
    body.dumpRaw(8);
    ui.clearDump();
  }
  if (ui.requestVerify()) {
    bool ok = false;
    body.verifyIdentity(&ok, 0.08f);
    ui.clearVerify();
  }

  if (ui.heldMode()) mode = RunMode::Held;
  else if (mode == RunMode::Held && !ui.heldMode()) mode = RunMode::Passive;

  ParsedCommand cmd;
  if (pollCommand(cmd)) {
    switch (cmd.type) {
      case BodyCommand::Excite:
        if (cmd.source_id >= 0 && cmd.source_id < body.numLasers()) {
          body.exciteOnce((uint16_t)cmd.source_id);
          mode = RunMode::Held;
        }
        break;
      case BodyCommand::Map:     body.runSelfMap(); break;
      case BodyCommand::Verify: {
        bool ok = false;
        body.verifyIdentity(&ok, 0.08f);
        break;
      }
      case BodyCommand::Passive: mode = RunMode::Passive; break;
      case BodyCommand::Dump:    body.dumpRaw(8); break;
      default: break;
    }
  }

  if (mode == RunMode::Passive) {
    body.tickPassive();
    delay(50);
  } else {
    delay(20);
  }
}

#endif
