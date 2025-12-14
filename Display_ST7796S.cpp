#include "Display_ST7796S.h"
#include "PinConfig.h"
#include <SPI.h>

// ===== Private SPI write helpers =====
static void wr_cmd(uint8_t c) {
    digitalWrite(TFT_DC, LOW);
    digitalWrite(TFT_CS, LOW);
    SPI.transfer(c);
    digitalWrite(TFT_CS, HIGH);
}

static void wr_dat(uint8_t d) {
    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);
    SPI.transfer(d);
    digitalWrite(TFT_CS, HIGH);
}

static void wr_dat16(uint16_t d) {
    wr_dat(d >> 8);
    wr_dat(d & 0xFF);
}

// ===== Display init (known-good from loader) =====
void initDisplay() {
    pinMode(TFT_CS,  OUTPUT);
    pinMode(TFT_DC,  OUTPUT);
    pinMode(TFT_RST, OUTPUT);
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);

    SPI.begin();
    SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE3));

    // Hardware reset
    digitalWrite(TFT_RST, LOW);
    delay(40);
    digitalWrite(TFT_RST, HIGH);
    delay(120);

    // ST7796S init
    wr_cmd(0xF0); wr_dat(0xC3);
    wr_cmd(0xF0); wr_dat(0x96);
    wr_cmd(0x36); wr_dat(0x28);   // landscape
    wr_cmd(0x3A); wr_dat(0x55);   // RGB565
    wr_cmd(0xB4); wr_dat(0x01);
    wr_cmd(0xB7); wr_dat(0xC6);
    wr_cmd(0x11);
    delay(120);
    wr_cmd(0x29);

    // Default drawing window full screen
    wr_cmd(0x2A); wr_dat16(0); wr_dat16(479);
    wr_cmd(0x2B); wr_dat16(0); wr_dat16(319);
    wr_cmd(0x2C);
}

// ===== Set drawing window =====
void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    wr_cmd(0x2A);
    wr_dat16(x0);
    wr_dat16(x1);

    wr_cmd(0x2B);
    wr_dat16(y0);
    wr_dat16(y1);

    wr_cmd(0x2C);
}

// ===== Draw single pixel =====
void drawPixel_FAST(int x, int y, uint16_t color) {
    if (x < 0 || x >= 480 || y < 0 || y >= 320) return;

    setAddrWindow(x, y, x, y);
    wr_dat16(color);
}

// ===== FULL, INSTANT SCREEN FILL (NO WIPE) =====
void fillScreen_FAST(uint16_t color) {
    // full window once
    setAddrWindow(0, 0, 479, 319);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);

    // blast the entire framebuffer with one continuous stream
    uint32_t total = 480UL * 320UL;
    while (total--) {
        SPI.transfer(hi);
        SPI.transfer(lo);
    }

    digitalWrite(TFT_CS, HIGH);
}

// ===== LED Backlight PWM (0–255) =====
void setBacklightPWM(uint8_t value) {
    analogWrite(TFT_LED, value);
}
