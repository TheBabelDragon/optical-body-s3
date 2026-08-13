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

  Serial.println(F("[UI] probing SH1106..."));
  delay(50);

  Wire.beginTransmission(UI_OLED_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("[UI] OLED not found"));
    return false;
  }
  if (!_display.begin(UI_OLED_ADDR, true)) {
    Serial.println(F("[UI] SH1106 begin failed"));
    return false;
  }

  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE);
  _display.setCursor(0, 0);
  _display.println(F("optical-body-s3"));
  _display.println(F("turn encoder"));
  _display.display();

  Serial.print(F("[UI] A=GPIO")); Serial.print(UI_ENC_A_PIN);
  Serial.print(F(" B=GPIO")); Serial.println(UI_ENC_B_PIN);
  return true;
}

void IRAM_ATTR DisplayUI::encIsr() {}

void DisplayUI::tick(OpticalBody& body) {
  // ---- Simple: any change on A or B advances the menu ----
  // This works even if only one encoder pin is connected.
  static uint8_t prevA = 1, prevB = 1;
  static uint32_t lastEdge = 0;

  uint8_t a = digitalRead(UI_ENC_A_PIN);
  uint8_t b = digitalRead(UI_ENC_B_PIN);
  uint32_t now = millis();

  bool edge = (a != prevA) || (b != prevB);
  if (edge && (now - lastEdge) > 20) {   // 20 ms debounce — one step per detent
    lastEdge = now;

    // Direction: prefer B if it moved, else use A
    if (b != prevB) {
      _rotDelta += (b == 0) ? 1 : -1;
    } else {
      _rotDelta += (a == 0) ? 1 : -1;
    }
  }
  prevA = a;
  prevB = b;

  handleInput();

  // ---- Buttons ----
  if (now - _lastBtn > 40) {
    _lastBtn = now;
    static bool prevC = 1, prevR = 1, prevS = 1;
    bool c = digitalRead(UI_CONFIRM_PIN);
    bool r = digitalRead(UI_RETURN_PIN);
    bool s = digitalRead(UI_ENC_SW_PIN);
    if (!c && prevC) onConfirm();
    if (!r && prevR) onReturn();
    if (!s && prevS) onConfirm();
    prevC = c; prevR = r; prevS = s;
  }

  // ---- Draw ----
  if (now - _lastDraw > 50) {
    _lastDraw = now;
    draw(body);
  }
}

void DisplayUI::handleInput() {
  int8_t d = _rotDelta;
  _rotDelta = 0;
  if (d == 0) return;

  d = (d > 0) ? 1 : -1;

  if (_page == UiPage::Excite) {
    _exciteId = constrain(_exciteId + d, 0, 15);
  } else {
    int p = (int)_page + d;
    if (p < 0) p = (int)UiPage::COUNT - 1;
    if (p >= (int)UiPage::COUNT) p = 0;
    _page = (UiPage)p;
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
  _display.print("> ");
  _display.println(titles[(int)_page]);
  _display.drawFastHLine(0, 10, 128, SH110X_WHITE);
  _display.setCursor(0, 14);

  switch (_page) {
    case UiPage::Status:
      _display.println(OPTICAL_BODY_NODE_ID);
      _display.print("Mode ");
      _display.println(_held ? "HELD" : "PASSIVE");
      _display.println("Turn knob");
      break;
    case UiPage::Identity:
      _display.println("Confirm=VERIFY");
      break;
    case UiPage::Mode:
      _display.print("Now ");
      _display.println(_held ? "HELD" : "PASSIVE");
      _display.println("Confirm=toggle");
      break;
    case UiPage::Excite:
      _display.print("Laser ");
      _display.println(_exciteId);
      _display.println("Confirm=fire");
      break;
    case UiPage::Stream:
      _display.print("Stream ");
      _display.println(_streaming ? "ON" : "OFF");
      break;
    case UiPage::Dump:
      _display.println("Confirm=DUMP");
      break;
    case UiPage::Calibrate:
      _display.println("Confirm=MAP");
      break;
    default: break;
  }
  _display.display();
}

#endif
