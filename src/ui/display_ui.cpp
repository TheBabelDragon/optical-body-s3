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

#ifndef UI_ENC_DETENT
#define UI_ENC_DETENT 4
#endif

DisplayUI* DisplayUI::_instance = nullptr;
static bool s_editing = false;

// Shared with ISR — one completed detent step (±1), consumed in tick()
static volatile int8_t s_step = 0;
static volatile uint8_t s_prev = 0;
static volatile int16_t s_acc = 0;

// CW: 00→01→11→10→00 = +1 each; CCW opposite
static const int8_t QUAD[16] = {
  // new:     00  01  10  11
  /* old 00 */ 0, +1, -1,  0,
  /* old 01 */-1,  0,  0, +1,
  /* old 10 */+1,  0,  0, -1,
  /* old 11 */ 0, -1, +1,  0
};

void IRAM_ATTR DisplayUI::encIsr() {
  uint8_t a = digitalRead(UI_ENC_A_PIN) ? 1 : 0;
  uint8_t b = digitalRead(UI_ENC_B_PIN) ? 1 : 0;
  uint8_t cur = (uint8_t)((a << 1) | b);

  if (cur == s_prev) return;

  int8_t d = QUAD[(s_prev << 2) | cur];
  s_prev = cur;
  if (d == 0) return;

  s_acc = (int16_t)(s_acc + d);

  if (s_acc >= UI_ENC_DETENT) {
    s_acc = 0;
    s_step = (int8_t)(s_step + 1);
  } else if (s_acc <= -UI_ENC_DETENT) {
    s_acc = 0;
    s_step = (int8_t)(s_step - 1);
  }
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

  // Seed state before enabling interrupts
  uint8_t a = digitalRead(UI_ENC_A_PIN) ? 1 : 0;
  uint8_t b = digitalRead(UI_ENC_B_PIN) ? 1 : 0;
  s_prev = (uint8_t)((a << 1) | b);
  s_acc = 0;
  s_step = 0;

  attachInterrupt(digitalPinToInterrupt(UI_ENC_A_PIN), DisplayUI::encIsr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(UI_ENC_B_PIN), DisplayUI::encIsr, CHANGE);

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
  _display.println(F("encoder IRQ"));
  _display.display();
  delay(200);
  return true;
}

void DisplayUI::drawSplash() {}
void DisplayUI::handleInput() {}

void DisplayUI::tick(OpticalBody& body) {
  uint32_t now = millis();
  static uint32_t lastTurnAt = 0;

  // Take completed detent steps from ISR
  noInterrupts();
  int8_t step = s_step;
  s_step = 0;
  interrupts();

  // Clamp to one menu move per tick so a burst cannot skip the whole menu
  if (step > 1) step = 1;
  if (step < -1) step = -1;

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

  if (now - _lastBtn > 30) {
    _lastBtn = now;
    static bool pc = 1, pr = 1, ps = 1;
    bool c = digitalRead(UI_CONFIRM_PIN);
    bool r = digitalRead(UI_RETURN_PIN);
    bool s = digitalRead(UI_ENC_SW_PIN);
    bool quiet = (now - lastTurnAt) > 180;

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
    case UiPage::Identity:  _doVerify = true; break;
    case UiPage::Mode:      _held = !_held; break;
    case UiPage::Excite:
      if (!s_editing) s_editing = true;
      else { s_editing = false; _doExcite = true; }
      break;
    case UiPage::Stream:    _streaming = !_streaming; break;
    case UiPage::Dump:      _doDump = true; break;
    case UiPage::Calibrate: _doMap = true; break;
    default: break;
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
