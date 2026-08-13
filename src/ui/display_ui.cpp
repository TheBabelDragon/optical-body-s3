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

// EC11: 4 gray-code transitions per mechanical detent
static const int DETENT = 4;

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
  delay(400);

  Serial.println(F("[UI] ready (detent/4)"));
  return true;
}

void DisplayUI::drawSplash() {}
void IRAM_ATTR DisplayUI::encIsr() {}
void DisplayUI::handleInput() {}

void DisplayUI::tick(OpticalBody& body) {
  uint32_t now = millis();

  // Gray-code table: ±1 per valid transition
  static const int8_t table[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
  };

  static uint8_t lastAB = 0;
  static int     accum  = 0;      // counts transitions toward one detent
  static uint32_t lastRotateMs = 0;

  uint8_t a = digitalRead(UI_ENC_A_PIN) ? 1 : 0;
  uint8_t b = digitalRead(UI_ENC_B_PIN) ? 1 : 0;
  uint8_t ab = (a << 1) | b;

  if (ab != lastAB) {
    int8_t t = table[((lastAB << 2) | ab) & 0x0F];
    lastAB = ab;

    if (t != 0) {
      accum += t;

      // Only when a FULL mechanical click is complete
      if (accum >= DETENT || accum <= -DETENT) {
        int dir = (accum > 0) ? 1 : -1;
        accum = 0;
        lastRotateMs = now;

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
    }
  }

  // Buttons — select ONLY on deliberate push, never from turn bounce
  if (now - _lastBtn > 30) {
    _lastBtn = now;

    static bool pc = 1, pr = 1, ps = 1;
    bool c = digitalRead(UI_CONFIRM_PIN);
    bool r = digitalRead(UI_RETURN_PIN);
    bool s = digitalRead(UI_ENC_SW_PIN);

    bool quiet = (now - lastRotateMs) > 150;  // no select while turning

    if (!c && pc && quiet) { onConfirm(); _lastDraw = 0; }
    if (!s && ps && quiet) { onConfirm(); _lastDraw = 0; }
    if (!r && pr)          { onReturn();  _lastDraw = 0; }

    pc = c; pr = r; ps = s;
  }

  if (now - _lastDraw > 50) {
    _lastDraw = now;
    draw(body);
  }
}

void DisplayUI::onConfirm() {
  switch (_page) {
    case UiPage::Identity:  _doVerify = true; break;
    case UiPage::Mode:      _held = !_held; break;
    case UiPage::Excite:    _doExcite = true; break;  // only on push
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
      break;

    case UiPage::Mode:
      _display.print("Now ");
      _display.println(_held ? "HELD" : "PASSIVE");
      _display.println("Push = toggle");
      break;

    case UiPage::Excite:
      _display.print("Laser ");
      _display.println(_exciteId);
      _display.println("Turn = change");
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
