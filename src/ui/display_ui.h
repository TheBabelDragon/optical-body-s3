#pragma once

/**
 * display_ui.h — DKARDU EC11 + SH1106 local controls
 *
 * -D OPTICAL_UI=1
 *
 * Turn  = navigate pages (quadrature A/B, 1 detent = 1 step)
 * Push  = select / on Excite: edit id then fire
 * Back  = Status
 */

#include <Arduino.h>

#if defined(OPTICAL_UI) && OPTICAL_UI

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

class OpticalBody;

enum class UiPage : uint8_t {
  Status = 0,
  Identity,
  Mode,
  Excite,
  Stream,
  Dump,
  Calibrate,
  COUNT
};

class DisplayUI {
public:
  DisplayUI();

  bool begin();
  void tick(OpticalBody& body);

  bool requestExcite() const { return _doExcite; }
  int  exciteId() const { return _exciteId; }
  void clearExcite() { _doExcite = false; }

  bool requestMap() const { return _doMap; }
  void clearMap() { _doMap = false; }

  bool requestDump() const { return _doDump; }
  void clearDump() { _doDump = false; }

  bool requestVerify() const { return _doVerify; }
  void clearVerify() { _doVerify = false; }

  bool streamingEnabled() const { return _streaming; }
  bool heldMode() const { return _held; }

private:
  Adafruit_SH1106G _display;

  UiPage _page = UiPage::Status;
  int    _exciteId = 0;
  bool   _held = false;
  bool   _streaming = true;

  bool _doExcite = false;
  bool _doMap = false;
  bool _doDump = false;
  bool _doVerify = false;

  uint32_t _lastDraw = 0;
  uint32_t _lastBtn = 0;

  void draw(OpticalBody& body);
  void drawSplash();
  void handleInput();
  void onConfirm();
  void onReturn();

  static void IRAM_ATTR encIsr();
  static DisplayUI* _instance;
};

#else

class OpticalBody;
class DisplayUI {
public:
  bool begin() { return true; }
  void tick(OpticalBody&) {}
  bool requestExcite() const { return false; }
  int  exciteId() const { return 0; }
  void clearExcite() {}
  bool requestMap() const { return false; }
  void clearMap() {}
  bool requestDump() const { return false; }
  void clearDump() {}
  bool requestVerify() const { return false; }
  void clearVerify() {}
  bool streamingEnabled() const { return true; }
  bool heldMode() const { return false; }
};

#endif
