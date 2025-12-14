#include "Page_PID3.h"
#include "Display_ST7796S.h"
#include "Touch_XPT2046.h"
#include "PageManager.h"

void Page_PID3_begin() {
    fillScreen_FAST(0x0000);

    setAddrWindow(60, 80, 419, 239);
    uint16_t color = 0x001F; // BLUE
    uint8_t hi = color >> 8, lo = color & 0xFF;

    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);
    for (int i = 0; i < (360 * 160); i++) { SPI.transfer(hi); SPI.transfer(lo); }
    digitalWrite(TFT_CS, HIGH);
    setAddrWindow(0, 0, 479, 319);


    setBacklightPWM(255);
}

void Page_PID3_update() {
    TouchPoint tp = readTouch();
    if (tp.touched) {
        PageManager_switchTo(PAGE_DTC);
        delay(200);
    }
}
