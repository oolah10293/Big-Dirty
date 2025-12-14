#pragma once
#include <Arduino.h>
#include <SPI.h>

extern const int TFT_CS;
extern const int TFT_DC;
extern const int TFT_RST;
extern const int TFT_LED;

// Public API for the display module
void initDisplay();
void drawPixel_FAST(int x, int y, uint16_t color);
void fillScreen_FAST(uint16_t color);

// RAW image drawing will be added later
void drawRAW480x320(const char *filename);
void setBacklightPWM(uint8_t value);
void setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
