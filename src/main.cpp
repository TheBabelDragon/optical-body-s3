/**
 * optical-body-s3 — SAFE_BOOT diagnostic
 *
 * Tries both native USB CDC (Serial) and the hardware UART (Serial0).
 * This covers both styles of ESP32-S3 boards.
 */

#include <Arduino.h>

#if defined(SAFE_BOOT) && SAFE_BOOT

void setup() {
  // Native USB CDC (most S3 DevKits)
  Serial.begin(115200);

  // Hardware UART0 (boards that have a separate USB-UART chip)
  Serial0.begin(115200);

  // Long settle — critical on some S3 boards
  delay(4000);

  for (int i = 0; i < 30; i++) {
    Serial.println("SAFE BOOT ALIVE");
    Serial0.println("SAFE BOOT ALIVE");
    Serial.flush();
    Serial0.flush();
    delay(150);
  }

  Serial.println("========================================");
  Serial.println("  optical-body-s3  SAFE BOOT");
  Serial.println("  USB path is working");
  Serial.println("========================================");
  Serial.flush();

  Serial0.println("========================================");
  Serial0.println("  optical-body-s3  SAFE BOOT");
  Serial0.println("  UART0 path is working");
  Serial0.println("========================================");
  Serial0.flush();
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    Serial.print("heartbeat ");
    Serial.println(millis());
    Serial.flush();

    Serial0.print("heartbeat ");
    Serial0.println(millis());
    Serial0.flush();
  }
  delay(5);
}

#else

// Full firmware intentionally left out of this diagnostic build.
// Set SAFE_BOOT=0 when the serial path is confirmed working.

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("SAFE_BOOT is disabled — rebuild with -D SAFE_BOOT=1 first");
}

void loop() {
  delay(1000);
}

#endif
