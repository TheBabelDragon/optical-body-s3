#include "ui/display_ui.h"

#if defined(OPTICAL_UI) && OPTICAL_UI

#include "optical_body/optical_body.h"

#ifndef UI_ENC_A_PIN
#define UI_ENC_A_PIN 1
#endif
#ifndef UI_ENC_B_PIN
#define UI_ENC_B_PIN 2
#endif
#ifndef UI_ENC_SW_PIN
#define UI_ENC_SW_PIN 3
#endif
#ifndef UI_CONFIRM_PIN
#define UI_CONFIRM_PIN 16
#endif
#ifndef UI_RETURN_PIN
#define UI_RETURN_PIN 17
#endif
#ifndef UI_OLED_ADDR
#define UI_OLED_ADDR 0x3C
#endif

DisplayUI* DisplayUI::_instance = nullptr;

DisplayUI::DisplayUI()
  : _display(128, 64, &Wire, -1) {}

bool DisplayUI::begin() {
  _instance = this;

  pinMode(UI_ENC_A_PIN, INPUT_PULLUP);
  pinMode(UI_ENC_B_PIN, INPUT_PULLUP);
  pinMode(UI_ENC_SW_PIN, INPUT_PULLUP);
  pinMode(UI_CONFIRM_PIN, INPUT_PULLUP);
  pinMode(UI_RETURN_PIN, INPUT_PULLUP);

  // No interrupt — pure polling is more reliable for cheap EC11 modules
  _lastEncA = digitalRead(UI_ENC_A_PIN);

  Serial.println(F("[UI] probing SH1106 OLED..."));
  Serial.flush();
  delay(50);

  bool ok = false;
  {
    Wire.beginTransmission(UI_OLED_ADDR);
    if (Wire.endTransmission() != 0) {
      Serial.println(F("[UI] OLED not found — continuing headless"));
      return false;
    }
    ok = _display.begin(UI_OLED_ADDR, true);
  }

  if (!ok) {
    Serial.println(F("[UI] SH1106 begin failed"));
    return false;
  }

  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE);
  _display.setCursor(0, 0);
  _display.println(F("optical-body-s3"));
  _display.println(F("UI ready"));
  _display.display();

  Serial.println(F("[UI] OLED + EC11 ready (polling)"));
  return true;
}

// Empty ISR stub (we no longer use interrupt)
void IRAM_ATTR DisplayUI::encIsr() {}

void DisplayUI::tick(OpticalBody& body) {
  // ---- Encoder polling (simple and reliable) ----
  static uint8_t lastA = 1;
  static uint8_t lastB = 1;
  static uint32_t lastChange = 0;

  uint8_t a = digitalRead(UI_ENC_A_PIN);
  uint8_t b = digitalRead(UI_ENC_B_PIN);
  uint32_t now = millis();

  // Detect transition on A
  if (a != lastA && (now - lastChange) > 5) {   // 5 ms debounce
    lastChange = now;
    if (a == 0) {   // falling edge on A
      // Direction from B
      if (b == 1) _rotDelta += 1;   // CW
      else        _rotDelta -= 1;   // CCW
    }
  }
  lastA = a;
  lastB = b;

  handleInput();

  // ---- Buttons ----
  if (now - _lastBtn > 40) {
    _lastBtn = now;

    static bool lastConfirm = true, lastReturn = true, lastEncSw = true;
    bool c = digitalRead(UI_CONFIRM_PIN);
    bool r = digitalRead(UI_RETURN_PIN);
    bool e = digitalRead(UI_ENC_SW_PIN);

    if (!c && lastConfirm) onConfirm();
    if (!r && lastReturn)  onReturn();
    if (!e && lastEncSw)   onConfirm();

    lastConfirm = c;
    lastReturn  = r;
    lastEncSw   = e;
  }

  // ---- Debug every 1.5 s ----
  static uint32_t lastDbg = 0;
  if (now - lastDbg > 1500) {
    lastDbg = now;
    Serial.print(F("[ENC] A="));
    Serial.print(a);
    Serial.print(F(" B="));
    Serial.print(b);
    Serial.print(F(" delta="));
    Serial.println(_rotDelta);
  }

  // ---- Draw ----
  if (now - _lastDraw > 80) {
    _lastDraw = now;
    draw(body);
  }
}

void DisplayUI::handleInput() {
  int8_t d = _rotDelta;
  _rotDelta = 0;

  if (d == 0) return;
  if (d > 0) d = 1;
  if (d < 0) d = -1;

  switch (_page) {
    case UiPage::Excite:
      _exciteId = constrain(_exciteId + d, 0, 15);
      break;
    default: {
      int p = (int)_page + d;
      if (p < 0) p = (int)UiPage::COUNT - 1;
      if (p >= (int)UiPage::COUNT) p = 0;
      _page = (UiPage)p;
      break;
    }
  }
}

void DisplayUI::onConfirm() {
  switch (_page) {
    case UiPage::Mode:      _held = !_held; break;
    case UiPage::Excite:    _doExcite = true; break;
    case UiPage::Stream:    _streaming = !_streaming; break;
    case UiPage::Dump:      _doDump = true; break;
    case UiPage::Calibrate: _doMap = true; break;
    case UiPage::Identity:  _doVerify = true; break;
    default: break;
  }
}

void DisplayUI::onReturn() {
  _page = UiPage::Status;
}

void DisplayUI::draw(OpticalBody& body) {
  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE);
  _display.setCursor(0, 0);

  const char* titles[] = {
    "STATUS", "IDENTITY", "MODE", "EXCITE",
    "STREAM", "DUMP", "CALIBRATE"
  };
  _display.print(F("> "));
  _display.println(titles[(int)_page]);
  _display.drawFastHLine(0, 10, 128, SH110X_WHITE);
  _display.setCursor(0, 14);

  switch (_page) {
    case UiPage::Status:
      _display.print(F("ID: ")); _display.println(OPTICAL_BODY_NODE_ID);
      _display.print(F("Mode: ")); _display.println(_held ? F("HELD") : F("PASSIVE"));
      _display.print(F("Stream: ")); _display.println(_streaming ? F("ON") : F("OFF"));
      _display.println(F("Rotate = menu"));
      break;
    case UiPage::Identity:
      _display.println(F("Confirm = VERIFY"));
      break;
    case UiPage::Mode:
      _display.print(F("Current: ")); _display.println(_held ? F("HELD") : F("PASSIVE"));
      _display.println(F("Confirm = toggle"));
      break;
    case UiPage::Excite:
      _display.print(F("Laser: ")); _display.println(_exciteId);
      _display.println(F("Confirm = fire"));
      break;
    case UiPage::Stream:
      _display.print(F("Stream: ")); _display.println(_streaming ? F("ON") : F("OFF"));
      _display.println(F("Confirm = toggle"));
      break;
    case UiPage::Dump:
      _display.println(F("Confirm = DUMP"));
      break;
    case UiPage::Calibrate:
      _display.println(F("Confirm = MAP"));
      break;
    default: break;
  }
  _display.display();
}

#endif
