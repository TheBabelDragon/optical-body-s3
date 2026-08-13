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

  // Buttons / encoder first — these never block
  pinMode(UI_ENC_A_PIN, INPUT_PULLUP);
  pinMode(UI_ENC_B_PIN, INPUT_PULLUP);
  pinMode(UI_ENC_SW_PIN, INPUT_PULLUP);
  pinMode(UI_CONFIRM_PIN, INPUT_PULLUP);
  pinMode(UI_RETURN_PIN, INPUT_PULLUP);

  _lastEncA = digitalRead(UI_ENC_A_PIN);
  attachInterrupt(digitalPinToInterrupt(UI_ENC_A_PIN), DisplayUI::encIsr, CHANGE);

  Serial.println(F("[UI] probing SH1106 OLED..."));
  Serial.flush();

  // Give the I2C bus a moment after Wire.begin()
  delay(50);

  // Adafruit begin() can hang if the device is missing or the bus is stuck.
  // We attempt it, but if it fails we continue without a display.
  bool ok = false;
  {
    // Simple presence check first
    Wire.beginTransmission(UI_OLED_ADDR);
    uint8_t err = Wire.endTransmission();
    if (err != 0) {
      Serial.print(F("[UI] OLED not found at 0x"));
      Serial.println(UI_OLED_ADDR, HEX);
      Serial.println(F("[UI] continuing without display"));
      return false;
    }

    ok = _display.begin(UI_OLED_ADDR, true);
  }

  if (!ok) {
    Serial.println(F("[UI] SH1106 begin failed — continuing headless"));
    return false;
  }

  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE);
  _display.setCursor(0, 0);
  _display.println(F("optical-body-s3"));
  _display.println(F("UI ready"));
  _display.display();

  Serial.println(F("[UI] DKARDU OLED + EC11 online"));
  return true;
}

void IRAM_ATTR DisplayUI::encIsr() {
  if (!_instance) return;
  bool a = digitalRead(UI_ENC_A_PIN);
  if (!a && _instance->_lastEncA) {
    // falling edge on A
    _instance->_rotDelta += (digitalRead(UI_ENC_B_PIN) ? -1 : 1);
  }
  _instance->_lastEncA = a;
}

void DisplayUI::tick(OpticalBody& body) {
  // If begin() failed we still have a valid object, but no display.
  // Guard the draw path.
  static bool hasDisplay = true;   // set false on first failed draw if needed

  handleInput();

  // Debounce buttons ~40 ms
  uint32_t now = millis();
  if (now - _lastBtn > 40) {
    _lastBtn = now;

    static bool lastConfirm = true, lastReturn = true, lastEncSw = true;
    bool c = digitalRead(UI_CONFIRM_PIN);
    bool r = digitalRead(UI_RETURN_PIN);
    bool e = digitalRead(UI_ENC_SW_PIN);

    if (!c && lastConfirm) { _confirmPressed = true; onConfirm(); }
    if (!r && lastReturn)  { _returnPressed  = true; onReturn(); }
    if (!e && lastEncSw)   { _confirmPressed = true; onConfirm(); } // encoder push = confirm

    lastConfirm = c;
    lastReturn  = r;
    lastEncSw   = e;
  }

  // Redraw ~10 Hz only if we successfully started the display
  if (hasDisplay && (now - _lastDraw > 100)) {
    _lastDraw = now;
    draw(body);
  }
}

void DisplayUI::handleInput() {
  noInterrupts();
  int8_t d = _rotDelta;
  _rotDelta = 0;
  interrupts();

  if (d == 0) return;

  switch (_page) {
    case UiPage::Excite:
      _exciteId = constrain(_exciteId + d, 0, 15);   // 16 possible lasers
      break;
    default:
      // navigate pages
      {
        int p = (int)_page + d;
        if (p < 0) p = (int)UiPage::COUNT - 1;
        if (p >= (int)UiPage::COUNT) p = 0;
        _page = (UiPage)p;
      }
      break;
  }
}

void DisplayUI::onConfirm() {
  switch (_page) {
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
    case UiPage::Identity:
      _doVerify = true;
      break;
    default:
      break;
  }
}

void DisplayUI::onReturn() {
  // Always go back to Status
  _page = UiPage::Status;
}

void DisplayUI::draw(OpticalBody& body) {
  _display.clearDisplay();
  _display.setTextSize(1);
  _display.setTextColor(SH110X_WHITE);
  _display.setCursor(0, 0);

  // Title bar
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
      _display.print(F("ID: "));
      _display.println(OPTICAL_BODY_NODE_ID);
      _display.print(F("Geom: "));
      _display.println(_held ? F("HELD") : F("PASSIVE"));
      _display.print(F("Stream: "));
      _display.println(_streaming ? F("ON") : F("OFF"));
      _display.println(F("Enc+Conf to navigate"));
      break;

    case UiPage::Identity:
      _display.println(F("Confirm = VERIFY"));
      _display.println(F("FRAM identity probe"));
      _display.println();
      _display.println(F("Return = Status"));
      break;

    case UiPage::Mode:
      _display.print(F("Current: "));
      _display.println(_held ? F("HELD") : F("PASSIVE"));
      _display.println();
      _display.println(F("Confirm = toggle"));
      break;

    case UiPage::Excite:
      _display.print(F("Laser ID: "));
      _display.println(_exciteId);
      _display.println();
      _display.println(F("Rotate = change"));
      _display.println(F("Confirm = fire"));
      break;

    case UiPage::Stream:
      _display.print(F("Streaming: "));
      _display.println(_streaming ? F("ON") : F("OFF"));
      _display.println();
      _display.println(F("Confirm = toggle"));
      _display.println(F("(serial JSONL)"));
      break;

    case UiPage::Dump:
      _display.println(F("Confirm = DUMP"));
      _display.println(F("raw ADC volts"));
      _display.println(F("(first 8 ch)"));
      break;

    case UiPage::Calibrate:
      _display.println(F("Confirm = MAP"));
      _display.println(F("full clean cal"));
      _display.println(F("(dark + one-hot)"));
      break;

    default:
      break;
  }

  _display.display();
}

#endif // OPTICAL_UI
