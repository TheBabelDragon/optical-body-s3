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

  delay(30);
  Wire.beginTransmission(UI_OLED_ADDR);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("[UI] OLED not found"));
    return false;
  }
  if (!_display.begin(UI_OLED_ADDR, true)) {
    Serial.println(F("[UI] SH1106 failed"));
    return false;
  }

  _page = UiPage::Status;
  _held = false;
  _streaming = true;
  _exciteId = 0;

  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE);
  _display.setCursor(0, 0);
  _display.println(F("optical-body-s3"));
  _display.println(F("1 click = 1 step"));
  _display.display();
  delay(600);

  Serial.println(F("[UI] ready"));
  return true;
}

void DisplayUI::drawSplash() {}

void IRAM_ATTR DisplayUI::encIsr() {}

void DisplayUI::tick(OpticalBody& body) {
  uint32_t now = millis();

  /*
   * Standard EC11 full-step decoder
   *
   * Read A/B as a 2-bit gray code.
   * Valid transitions that complete one detent produce ±1.
   * This is the common "full step" set used by many EC11 libraries:
   *   CW:  0b00→0b01→0b11→0b10→0b00  (or reverse start)
   * We only fire when we hit the detent-aligned transitions.
   */
  static uint8_t prev = 0;
  static uint32_t lastStepMs = 0;

  uint8_t curr = (digitalRead(UI_ENC_A_PIN) ? 2 : 0) |
                 (digitalRead(UI_ENC_B_PIN) ? 1 : 0);

  if (curr != prev) {
    // Combined previous+current nibble
    uint8_t s = (prev << 2) | curr;

    int8_t step = 0;

    // Full-step CW transitions
    if (s == 0b0001 || s == 0b0111 || s == 0b1110 || s == 0b1000) step = 1;
    // Full-step CCW transitions
    if (s == 0b0010 || s == 0b1011 || s == 0b1101 || s == 0b0100) step = -1;

    prev = curr;

    // One physical click should not produce multiple steps from bounce
    if (step != 0 && (now - lastStepMs) > 40) {
      lastStepMs = now;
      _rotDelta += step;
    }
  }

  if (_rotDelta != 0) {
    int dir = (_rotDelta > 0) ? 1 : -1;
    _rotDelta = 0;

    if (_page == UiPage::Excite) {
      _exciteId += dir;
      if (_exciteId < 0)  _exciteId = 15;
      if (_exciteId > 15) _exciteId = 0;
    } else {
      int p = (int)_page + dir;
      if (p < 0) p = (int)UiPage::COUNT - 1;
      if (p >= (int)UiPage::COUNT) p = 0;
      _page = (UiPage)p;
    }
    _lastDraw = 0;
  }

  // Buttons
  if (now - _lastBtn > 40) {
    _lastBtn = now;
    static bool pc = 1, pr = 1, ps = 1;
    bool c = digitalRead(UI_CONFIRM_PIN);
    bool r = digitalRead(UI_RETURN_PIN);
    bool s = digitalRead(UI_ENC_SW_PIN);

    if (!c && pc) { onConfirm(); _lastDraw = 0; }
    if (!s && ps) { onConfirm(); _lastDraw = 0; }
    if (!r && pr) { onReturn();  _lastDraw = 0; }

    pc = c; pr = r; ps = s;
  }

  if (now - _lastDraw > 50) {
    _lastDraw = now;
    draw(body);
  }
}

void DisplayUI::handleInput() {}

void DisplayUI::onConfirm() {
  switch (_page) {
    case UiPage::Identity:  _doVerify = true; break;
    case UiPage::Mode:      _held = !_held; break;
    case UiPage::Excite:    _doExcite = true; break;
    case UiPage::Stream:    _streaming = !_streaming; break;
    case UiPage::Dump:      _doDump = true; break;
    case UiPage::Calibrate: _doMap = true; break;
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

  const char* titles[] = {
    "STATUS", "IDENTITY", "MODE", "EXCITE",
    "STREAM", "DUMP", "CALIBRATE"
  };

  _display.setCursor(0, 0);
  _display.print("> ");
  _display.println(titles[(int)_page]);
  _display.drawFastHLine(0, 10, 128, SH110X_WHITE);
  _display.setCursor(0, 14);

  switch (_page) {
    case UiPage::Status:
      _display.println(OPTICAL_BODY_NODE_ID);
      _display.print("Mode ");
      _display.println(_held ? "HELD" : "PASSIVE");
      _display.print("Stream ");
      _display.println(_streaming ? "ON" : "OFF");
      break;

    case UiPage::Identity:
      _display.println("Push = VERIFY");
      _display.println("Back = Status");
      break;

    case UiPage::Mode:
      _display.print("Now ");
      _display.println(_held ? "HELD" : "PASSIVE");
      _display.println("Push = toggle");
      break;

    case UiPage::Excite:
      _display.print("Laser ");
      _display.println(_exciteId);
      _display.println("Push = FIRE");
      break;

    case UiPage::Stream:
      _display.print("Stream ");
      _display.println(_streaming ? "ON" : "OFF");
      _display.println("Push = toggle");
      break;

    case UiPage::Dump:
      _display.println("Push = DUMP");
      break;

    case UiPage::Calibrate:
      _display.println("Push = MAP");
      break;

    default:
      break;
  }

  _display.display();
}

#endif
