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

/*
 * Rotary encoder (EC11) — standard quadrature decoding
 *
 * Pins A and B form a 2-bit gray code. Valid transitions:
 *
 *   CW:  00 → 01 → 11 → 10 → 00
 *   CCW: 00 → 10 → 11 → 01 → 00
 *
 * Lookup: index = (old_state << 2) | new_state
 * Value:  +1 = CW, -1 = CCW, 0 = invalid/no move
 *
 * A typical 20-detent EC11 produces 4 gray transitions per mechanical click.
 * We sum those and emit one menu step when |sum| reaches 4.
 */

#ifndef UI_ENC_DETENT
#define UI_ENC_DETENT 4
#endif

DisplayUI* DisplayUI::_instance = nullptr;
static bool s_editing = false;

// Gray-code transition table (same one used in nearly every EC11 example)
static const int8_t QUAD[16] = {
  // new: 00  01  10  11     <- old in high bits
  /*00*/   0, -1, +1,  0,
  /*01*/  +1,  0,  0, -1,
  /*10*/  -1,  0,  0, +1,
  /*11*/   0, +1, -1,  0
};

static int8_t quadratureStep() {
  static uint8_t prev = 0;
  static int16_t acc  = 0;

  uint8_t a = digitalRead(UI_ENC_A_PIN) ? 1 : 0;
  uint8_t b = digitalRead(UI_ENC_B_PIN) ? 1 : 0;
  uint8_t cur = (a << 1) | b;   // bit1=A, bit0=B

  if (cur == prev) return 0;

  int8_t d = QUAD[(prev << 2) | cur];
  prev = cur;
  if (d == 0) return 0;

  acc += d;

  if (acc >= UI_ENC_DETENT) {
    acc = 0;
    return +1;   // one full CW click
  }
  if (acc <= -UI_ENC_DETENT) {
    acc = 0;
    return -1;   // one full CCW click
  }
  return 0;
}

DisplayUI::DisplayUI()
  : _display(128, 64, &Wire, -1) {}

bool DisplayUI::begin() {
  _instance = this;

  pinMode(UI_ENC_A_PIN, INPUT_PULLUP);
  pinMode(UI_ENC_B_PIN, INPUT_PULLUP);
  pinMode(UI_ENC_SW_PIN, INPUT_PULLUP);
  pinMode(UI_CONFIRM_PIN, INPUT_PULLUP);
  pinMode(UI_RETURN_PIN, INPUT_PULLUP);

  // Seed previous state so first edges are valid
  uint8_t a = digitalRead(UI_ENC_A_PIN) ? 1 : 0;
  uint8_t b = digitalRead(UI_ENC_B_PIN) ? 1 : 0;
  (void)a; (void)b;

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
  s_editing = false;

  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE);
  _display.setCursor(0, 0);
  _display.println(F("optical-body-s3"));
  _display.println(F("CW/CCW from A/B"));
  _display.display();
  delay(250);
  return true;
}

void DisplayUI::drawSplash() {}
void IRAM_ATTR DisplayUI::encIsr() {}
void DisplayUI::handleInput() {}

void DisplayUI::tick(OpticalBody& body) {
  uint32_t now = millis();
  static uint32_t lastTurnAt = 0;

  // Direction comes only from A/B transition order
  int8_t step = quadratureStep();
  if (step != 0) {
    lastTurnAt = now;

    if (s_editing && _page == UiPage::Excite) {
      _exciteId += step;
      if (_exciteId < 0)  _exciteId = 15;
      if (_exciteId > 15) _exciteId = 0;
    } else {
      s_editing = false;
      int p = (int)_page + step;
      if (p < 0) p = (int)UiPage::COUNT - 1;
      if (p >= (int)UiPage::COUNT) p = 0;
      _page = (UiPage)p;
    }
    _lastDraw = 0;
  }

  // Push = select. Ignore SW bounce for 200 ms after a turn.
  if (now - _lastBtn > 30) {
    _lastBtn = now;
    static bool pc = 1, pr = 1, ps = 1;
    bool c = digitalRead(UI_CONFIRM_PIN);
    bool r = digitalRead(UI_RETURN_PIN);
    bool s = digitalRead(UI_ENC_SW_PIN);
    bool quiet = (now - lastTurnAt) > 200;

    if ((!c && pc && quiet) || (!s && ps && quiet)) {
      onConfirm();
      _lastDraw = 0;
    }
    if (!r && pr) {
      s_editing = false;
      onReturn();
      _lastDraw = 0;
    }
    pc = c; pr = r; ps = s;
  }

  if (now - _lastDraw > 40) {
    _lastDraw = now;
    draw(body);
  }
}

void DisplayUI::onConfirm() {
  switch (_page) {
    case UiPage::Identity:
      _doVerify = true;
      break;
    case UiPage::Mode:
      _held = !_held;
      break;
    case UiPage::Excite:
      if (!s_editing) {
        s_editing = true;          // push 1: edit laser id
      } else {
        s_editing = false;
        _doExcite = true;          // push 2: fire
      }
      break;
    case UiPage::Stream:
      _streaming = !_streaming;
      break;
    case UiPage::Dump:
      _doDump = true;
      break;
    case UiPage::Calibrate:
      _doMap = true;
      break;
    default:
      break;
  }
}

void DisplayUI::onReturn() {
  s_editing = false;
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
      if (s_editing) {
        _display.println(F("* EDIT *"));
        _display.println(F("Turn=id Push=FIRE"));
      } else {
        _display.println(F("Push = edit id"));
      }
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
