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

  drawSplash();
  Serial.println(F("[UI] menu ready"));
  return true;
}

void DisplayUI::drawSplash() {
  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE);
  _display.setCursor(0, 0);
  _display.println(F("optical-body-s3"));
  _display.println(F(""));
  _display.println(F("Turn = move"));
  _display.println(F("Click = select"));
  _display.println(F("Back  = return"));
  _display.display();
  delay(900);
}

void IRAM_ATTR DisplayUI::encIsr() {}

void DisplayUI::tick(OpticalBody& body) {
  uint32_t now = millis();

  // ---------- Encoder (any edge, strong debounce) ----------
  static uint8_t prevA = 1, prevB = 1;
  static uint32_t lastEnc = 0;

  uint8_t a = digitalRead(UI_ENC_A_PIN);
  uint8_t b = digitalRead(UI_ENC_B_PIN);

  if ((a != prevA || b != prevB) && (now - lastEnc) > 35) {
    lastEnc = now;
    // One step per detent-ish edge
    int dir = 1;
    if (b != prevB) dir = (b == 0) ? 1 : -1;
    else            dir = (a == 0) ? 1 : -1;
    _rotDelta += dir;
  }
  prevA = a;
  prevB = b;

  if (_rotDelta != 0) {
    int step = (_rotDelta > 0) ? 1 : -1;
    _rotDelta = 0;

    if (_page == UiPage::Excite) {
      _exciteId += step;
      if (_exciteId < 0) _exciteId = 15;
      if (_exciteId > 15) _exciteId = 0;
    } else {
      int p = (int)_page + step;
      if (p < 0) p = (int)UiPage::COUNT - 1;
      if (p >= (int)UiPage::COUNT) p = 0;
      _page = (UiPage)p;
    }
    _lastDraw = 0;   // force redraw
  }

  // ---------- Buttons (Confirm / Encoder SW / Return) ----------
  if (now - _lastBtn > 45) {
    _lastBtn = now;

    static bool prevConfirm = 1, prevReturn = 1, prevSw = 1;
    bool confirm = digitalRead(UI_CONFIRM_PIN);
    bool ret     = digitalRead(UI_RETURN_PIN);
    bool sw      = digitalRead(UI_ENC_SW_PIN);

    // active LOW
    if (confirm == 0 && prevConfirm == 1) {
      onConfirm();
      _lastDraw = 0;
    }
    if (sw == 0 && prevSw == 1) {
      onConfirm();          // encoder push = select
      _lastDraw = 0;
    }
    if (ret == 0 && prevReturn == 1) {
      onReturn();
      _lastDraw = 0;
    }

    prevConfirm = confirm;
    prevReturn  = ret;
    prevSw      = sw;
  }

  // ---------- Draw ----------
  if (now - _lastDraw > 60) {
    _lastDraw = now;
    draw(body);
  }
}

void DisplayUI::handleInput() {
  // kept for header compatibility; logic is inline in tick()
}

void DisplayUI::onConfirm() {
  switch (_page) {
    case UiPage::Status:
      // nothing — just stay
      break;
    case UiPage::Identity:
      _doVerify = true;
      break;
    case UiPage::Mode:
      _held = !_held;
      break;
    case UiPage::Excite:
      _doExcite = true;
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
  _page = UiPage::Status;
}

void DisplayUI::draw(OpticalBody& body) {
  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE);

  // Title
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
      _display.println("Turn=nav  Click=ok");
      break;

    case UiPage::Identity:
      _display.println("Check identity");
      _display.println("");
      _display.println("Click = VERIFY");
      _display.println("Back  = Status");
      break;

    case UiPage::Mode:
      _display.print("Current: ");
      _display.println(_held ? "HELD" : "PASSIVE");
      _display.println("");
      _display.println("Click = toggle");
      _display.println("Back  = Status");
      break;

    case UiPage::Excite:
      _display.print("Laser: ");
      _display.println(_exciteId);
      _display.println("Turn = change");
      _display.println("Click = FIRE");
      _display.println("Back  = Status");
      break;

    case UiPage::Stream:
      _display.print("Stream: ");
      _display.println(_streaming ? "ON" : "OFF");
      _display.println("");
      _display.println("Click = toggle");
      _display.println("Back  = Status");
      break;

    case UiPage::Dump:
      _display.println("Raw ADC dump");
      _display.println("");
      _display.println("Click = DUMP");
      _display.println("Back  = Status");
      break;

    case UiPage::Calibrate:
      _display.println("Full self-map");
      _display.println("");
      _display.println("Click = MAP");
      _display.println("Back  = Status");
      break;

    default:
      break;
  }

  _display.display();
}

#endif
