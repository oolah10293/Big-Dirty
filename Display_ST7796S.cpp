#include "Display_ST7796S.h"
#include "PinConfig.h"
#include "PageManager.h"
#include <SPI.h>
#include <math.h>

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

// ===== Backlight / dimmer constants =====
static const float DIMMER_DIVIDER_R_TOP    = 38100.0f;
static const float DIMMER_DIVIDER_R_BOTTOM = 11870.0f;
static const float DIMMER_MIN_V            = 1.0f;
static const float DIMMER_MAX_V            = 13.0f;
static const float ADC_REF_V               = 3.3f;
static const float ADC_COUNTS              = 4095.0f;

static bool dimmerHasTakenControl = false;

static inline void displaySPI_begin() {
    digitalWrite(T_CS, HIGH);  // keep touch deselected while display owns SPI
    SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE3));
}

static inline void displaySPI_end() {
    SPI.endTransaction();
}

// ===== Display init =====
void initDisplay() {
    pinMode(TFT_CS,  OUTPUT);
    pinMode(TFT_DC,  OUTPUT);
    pinMode(TFT_RST, OUTPUT);
    pinMode(TFT_LED, OUTPUT);
    pinMode(T_CS, OUTPUT);
    pinMode(DIMMER_SENSE, INPUT);

    digitalWrite(TFT_CS, HIGH);
    digitalWrite(T_CS, HIGH);
    digitalWrite(TFT_LED, LOW);

    SPI.begin();

    displaySPI_begin();

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

    displaySPI_end();
}

// ===== Set drawing window =====
// IMPORTANT: caller must already own the SPI transaction.
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

    displaySPI_begin();

    setAddrWindow((uint16_t)x, (uint16_t)y, (uint16_t)x, (uint16_t)y);
    wr_dat16(color);

    displaySPI_end();
}

// ===== FULL SCREEN FILL =====
void fillScreen_FAST(uint16_t color) {
    displaySPI_begin();

    setAddrWindow(0, 0, 479, 319);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);

    uint32_t total = 480UL * 320UL;
    while (total--) {
        SPI.transfer(hi);
        SPI.transfer(lo);
    }

    digitalWrite(TFT_CS, HIGH);

    // restore default full-screen window
    setAddrWindow(0, 0, 479, 319);

    displaySPI_end();
}

// ===== FAST RECTANGLE FILL =====
void fillRect_FAST(int x0, int y0, int x1, int y1, uint16_t color) {
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }

    if (x1 < 0 || y1 < 0 || x0 >= 480 || y0 >= 320) return;

    if (x0 < 0)   x0 = 0;
    if (y0 < 0)   y0 = 0;
    if (x1 > 479) x1 = 479;
    if (y1 > 319) y1 = 319;

    displaySPI_begin();

    setAddrWindow((uint16_t)x0, (uint16_t)y0, (uint16_t)x1, (uint16_t)y1);

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;
    uint32_t total = (uint32_t)(x1 - x0 + 1) * (uint32_t)(y1 - y0 + 1);

    digitalWrite(TFT_DC, HIGH);
    digitalWrite(TFT_CS, LOW);

    while (total--) {
        SPI.transfer(hi);
        SPI.transfer(lo);
    }

    digitalWrite(TFT_CS, HIGH);

    // restore default full-screen window
    setAddrWindow(0, 0, 479, 319);

    displaySPI_end();
}

// ===== LED Backlight PWM (0–255) =====
void setBacklightPWM(uint8_t value) {
    analogWrite(TFT_LED, value);
}

void backlightControl_init() {
    pinMode(DIMMER_SENSE, INPUT);

    analogReadResolution(12);
    analogWriteResolution(8);
    analogWriteFrequency(TFT_LED, 20000);

    analogWrite(TFT_LED, 0);
    dimmerHasTakenControl = false;
}

void backlightControl_update() {
    // Boot logo page keeps full direct control of the backlight.
    if (PageManager_current() == PAGE_BOOTLOGO) {
        dimmerHasTakenControl = false;
        return;
    }

    int raw = analogRead(DIMMER_SENSE);

    // ADC pin voltage
    float pinVoltage = ((float)raw * ADC_REF_V) / ADC_COUNTS;

    // Undo divider to estimate source voltage
    float sourceVoltage =
        pinVoltage * ((DIMMER_DIVIDER_R_TOP + DIMMER_DIVIDER_R_BOTTOM) / DIMMER_DIVIDER_R_BOTTOM);

    uint8_t pwm = 255;

    if (sourceVoltage < DIMMER_MIN_V || sourceVoltage > DIMMER_MAX_V) {
        pwm = 255;
    } else {
        float t = (sourceVoltage - DIMMER_MIN_V) / (DIMMER_MAX_V - DIMMER_MIN_V);
        float curved = powf(t, 4.0f);
        int value = (int)(curved * 255.0f + 0.5f);

        if (value < 0) value = 0;
        if (value > 255) value = 255;

        pwm = (uint8_t)value;
    }

    analogWrite(TFT_LED, pwm);
    dimmerHasTakenControl = true;
}