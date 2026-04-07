#include <SPI.h>
#include <SD.h>
#include "Display_ST7796S.h"
#include "Touch_XPT2046.h"
#include "RawImage.h"
#include "PageManager.h"
#include "PinConfig.h"
#include "Overlay_Warnings.h"
#include "PID_Manager.h"

void setup() {
  Serial.begin(115200);
  SPI.begin();
  SD.begin(BUILTIN_SDCARD);
  initDisplay();
  backlightControl_init();   
  fillScreen_FAST(0x0000);
  initTouch();

  pinMode(PIN_SEATBELT, INPUT);
  pinMode(PIN_BRAKE, INPUT);
  pinMode(PIN_HIGHBEAM, INPUT);
  pinMode(PIN_CHECK_ENGINE, INPUT);
  pinMode(PIN_4WD, INPUT);

  Overlay_Warnings_begin();
  PIDManager_begin();
  PageManager_begin();
 }

void loop() {
  bool seatbeltOn    = (digitalRead(PIN_SEATBELT) == HIGH);
  bool brakeOn       = (digitalRead(PIN_BRAKE) == HIGH);
  bool highBeamOn    = (digitalRead(PIN_HIGHBEAM) == HIGH);
  bool checkEngineOn = (digitalRead(PIN_CHECK_ENGINE) == LOW);
  bool fourWdOn      = (digitalRead(PIN_4WD) == LOW);

  PageManager_update();

  if (PageManager_current() != PAGE_BOOTLOGO) {
    Overlay_Warnings_update(
      seatbeltOn,
      brakeOn,
      highBeamOn,
      checkEngineOn,
      fourWdOn
    );
  }

  backlightControl_update();
}