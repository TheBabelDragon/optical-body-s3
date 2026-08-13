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

// EC11 typically produces 4 quadrature transitions per detent.
// We accumulate those and only emit ±1 when a full click is done.
#ifndef UI_ENC_DETENT_STEPS
#define UI_ENC_DETENT_STEPS 4
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

  drawSplash();
  Serial.println(F("[UI] menu ready (per-click)"));
  return true;
}

void DisplayUI::drawSplash() {
  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE);
  _display.setCursor(0, 0);
  _display.println(F("optical-body-s3"));
  _display.println();
  _display.println(F("Click turn = move"));
  _display.println(F("Push     = select"));
  _display.println(F("Back     = return"));
  _display.display();
  delay(800);
}

void IRAM_ATTR DisplayUI::encIsr() {}

void DisplayUI::tick(OpticalBody& body) {
  uint32_t now = millis();

  // ---- Full quadrature state machine, count per detent ----
  // Gray code table: index = (oldAB << 2) | newAB
  static const int8_t table[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
  };

  static uint8_t lastAB = 0;
  static int8_t  accum  = 0;   // accumulates fractional steps

  uint8_t a = digitalRead(UI_ENC_A_PIN) ? 1 : 0;
  uint8_t b = digitalRead(UI_ENC_B_PIN) ? 1 : 0;
  uint8_t ab = (a << 1) | b;

  if (ab != lastAB) {
    uint8_t idx = (lastAB << 2) | ab;
    int8_t t = table[idx & 0x0F];
    lastAB = ab;

    if (t != 0) {
      accum += t;

      // One full mechanical click completed?
      if (accum >= UI_ENC_DETENT_STEPS) {
        _rotDelta += 1;
        accum = 0;
      } else if (accum <= -UI_ENC_DETENT_STEPS) {
        _rotDelta -= 1;
        accum = 0;
      }
    }
  }

  // Apply completed clicks to the menu
  if (_rotDelta != 0) {
    int step = (_rotDelta > 0) ? 1 : -1;
    _rotDelta = 0;

    if (_page == UiPage::Excite) {
      _exciteId += step;
      if (_exciteId < 0)  _exciteId = 15;
      if (_exciteId > 15) _exciteId = 0;
    } else {
      int p = (int)_page + step;
      if (p < 0) p = (int)UiPage::COUNT - 1;
      if (p >= (int)UiPage::COUNT) p = 0;
      _page = (UiPage)p;
    }
    _lastDraw = 0;   // force immediate redraw
  }

  // ---- Buttons ----
  if (now - _lastBtn > 40) {
    _lastBtn = now;

    static bool prevConfirm = 1, prevReturn = 1, prevSw = 1;
    bool confirm = digitalRead(UI_CONFIRM_PIN);
    bool ret     = digitalRead(UI_RETURN_PIN);
    bool sw      = digitalRead(UI_ENC_SW_PIN);

    if (confirm == 0 && prevConfirm == 1) { onConfirm(); _lastDraw = 0; }
    if (sw == 0 && prevSw == 1)           { onConfirm(); _lastDraw = 0; }
    if (ret == 0 && prevReturn == 1)      { onReturn();  _lastDraw = 0; }

    prevConfirm = confirm;
    prevReturn  = ret;
    prevSw      = sw;
  }

  // ---- Draw ----
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
      _display.print("Mode: ");
      _display.println(_held ? "HELD" : "PASSIVE");
      _display.print("Stream: ");
      _display.println(_streaming ? "ON" : "OFF");
      _display.println("Click turn = nav");
      break;

    case UiPage::Identity:
      _display.println("Check identity");
      _display.println();
      _display.println("Push = VERIFY");
      _display.println("Back = Status");
      break;

    case UiPage::Mode:
      _display.print("Current: ");
      _display.println(_held ? "HELD" : "PASSIVE");
      _display.println();
      _display.println("Push = toggle");
      _display.println("Back = Status");
      break;

    case UiPage::Excite:
      _display.print("Laser: ");
      _display.println(_exciteId);
      _display.println("Click turn = change");
      _display.println("Push = FIRE");
      _display.println("Back = Status");
      break;

    case UiPage::Stream:
      _display.print("Stream: ");
      _display.println(_streaming ? "ON" : "OFF");
      _display.println();
      _display.println("Push = toggle");
      _display.println("Back = Status");
      break;

    case UiPage::Dump:
      _display.println("Raw ADC dump");
      _display.println();
      _display.println("Push = DUMP");
      _display.println("Back = Status");
      break;

    case UiPage::Calibrate:
      _display.println("Full self-map");
      _display.println();
      _display.println("Push = MAP");
      _display.println("Back = Status");
      break;

    default:
      break;
  }

  _display.display();
}

#endif
