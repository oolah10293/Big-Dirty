#include <SPI.h>
#include <SD.h>
#include "Display_ST7796S.h"
#include "Touch_XPT2046.h"
#include "RawImage.h"
#include "PageManager.h"
#include "PinConfig.h"

void setup() {
  Serial.begin(115200);
  SPI.begin();
  SD.begin(BUILTIN_SDCARD);
  initDisplay();
  fillScreen_FAST(0x0000);
  initTouch();

  PageManager_begin();         // BootLogo page is now first page
 
  Serial.println("UI boot complete");
}

void loop() {
  PageManager_update();
}
