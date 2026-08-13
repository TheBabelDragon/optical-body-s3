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

  delay(20);
  Wire.beginTransmission(UI_OLED_ADDR);
  if (Wire.endTransmission() != 0 || !_display.begin(UI_OLED_ADDR, true)) {
    Serial.println(F("[UI] OLED fail"));
    return false;
  }

  _page = UiPage::Status;
  _exciteId = 0;
  _held = false;
  _streaming = true;

  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE);
  _display.setCursor(0, 0);
  _display.println(F("optical-body-s3"));
  _display.println(F("Turn=move Push=ok"));
  _display.display();
  delay(300);
  return true;
}

void DisplayUI::drawSplash() {}
void IRAM_ATTR DisplayUI::encIsr() {}
void DisplayUI::handleInput() {}

void DisplayUI::tick(OpticalBody& body) {
  uint32_t now = millis();

  /*
   * Stable-state detent sampler
   *
   * 1. Any change on A/B starts a settle timer
   * 2. When A/B are unchanged for SETTLE_MS, the detent is done
   * 3. Compare settled state to previous settled state → exactly one step
   *
   * This ignores the intermediate gray edges inside the click.
   */
  static const uint32_t SETTLE_MS = 25;

  static uint8_t lastRaw   = 0xFF;
  static uint8_t settled   = 0;
  static uint8_t prevSettled = 0;
  static uint32_t changeAt = 0;
  static bool     haveSettled = false;
  static uint32_t lastTurnAt = 0;

  uint8_t raw = (digitalRead(UI_ENC_A_PIN) ? 2 : 0) |
                (digitalRead(UI_ENC_B_PIN) ? 1 : 0);

  if (raw != lastRaw) {
    lastRaw = raw;
    changeAt = now;
  }

  if ((now - changeAt) >= SETTLE_MS && raw == lastRaw) {
    if (!haveSettled) {
      settled = raw;
      prevSettled = raw;
      haveSettled = true;
    } else if (raw != settled) {
      // Completed a detent: raw is new stable position
      // Direction from gray transition between prev and new
      static const int8_t tab[16] = {
         0, -1,  1,  0,
         1,  0,  0, -1,
        -1,  0,  0,  1,
         0,  1, -1,  0
      };
      int8_t dir = tab[((settled << 2) | raw) & 15];
      // If table says 0 (invalid jump), fall back to simple bit of A
      if (dir == 0) {
        dir = ((raw ^ settled) & 2) ? ((raw & 2) ? 1 : -1) : ((raw & 1) ? 1 : -1);
      }

      prevSettled = settled;
      settled = raw;
      lastTurnAt = now;

      if (dir != 0) {
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

  // Select only on push; never while settling a turn
  if (now - _lastBtn > 30) {
    _lastBtn = now;
    static bool pc = 1, pr = 1, ps = 1;
    bool c = digitalRead(UI_CONFIRM_PIN);
    bool r = digitalRead(UI_RETURN_PIN);
    bool s = digitalRead(UI_ENC_SW_PIN);
    bool quiet = (now - lastTurnAt) > 200 && (now - changeAt) > 50;

    if (!c && pc && quiet) { onConfirm(); _lastDraw = 0; }
    if (!s && ps && quiet) { onConfirm(); _lastDraw = 0; }
    if (!r && pr)          { onReturn();  _lastDraw = 0; }

    pc = c; pr = r; ps = s;
  }

  if (now - _lastDraw > 40) {
    _lastDraw = now;
    draw(body);
  }
}

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
  (void)body;
  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE);

  static const char* titles[] = {
    "STATUS", "IDENTITY", "MODE", "EXCITE",
    "STREAM", "DUMP", "CALIBRATE"
  };

  _display.setCursor(0, 0);
  _display.print(F("> "));
  _display.println(titles[(int)_page]);
  _display.drawFastHLine(0, 10, 128, SH110X_WHITE);
  _display.setCursor(0, 14);

  switch (_page) {
    case UiPage::Status:
      _display.println(OPTICAL_BODY_NODE_ID);
      _display.print(F("Mode "));
      _display.println(_held ? F("HELD") : F("PASSIVE"));
      _display.print(F("Stream "));
      _display.println(_streaming ? F("ON") : F("OFF"));
      break;
    case UiPage::Identity:
      _display.println(F("Push = VERIFY"));
      break;
    case UiPage::Mode:
      _display.print(F("Now "));
      _display.println(_held ? F("HELD") : F("PASSIVE"));
      _display.println(F("Push = toggle"));
      break;
    case UiPage::Excite:
      _display.print(F("Laser "));
      _display.println(_exciteId);
      _display.println(F("Turn = change"));
      _display.println(F("Push = FIRE"));
      break;
    case UiPage::Stream:
      _display.print(F("Stream "));
      _display.println(_streaming ? F("ON") : F("OFF"));
      _display.println(F("Push = toggle"));
      break;
    case UiPage::Dump:
      _display.println(F("Push = DUMP"));
      break;
    case UiPage::Calibrate:
      _display.println(F("Push = MAP"));
      break;
    default:
      break;
  }
  _display.display();
}

#endif
