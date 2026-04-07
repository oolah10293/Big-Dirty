#include "Page_Env.h"
#include "Display_ST7796S.h"
#include "Display_Fonts.h"
#include "Overlay_Warnings.h"
#include "Touch_XPT2046.h"
#include "PageManager.h"
#include "DHT20.h"
#include <Wire.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define AQUA  0x07FF
#define BLACK 0x0000

static bool envDrawn = false;

// DHT20
static DHT20 dht;
static bool dhtReady = false;
static uint32_t lastDhtReadMs = 0;
static const uint32_t DHT_READ_INTERVAL_MS = 1000;

// Analog / trans temp / current sensor
static bool analogReady = false;
static uint32_t lastTransReadMs = 0;
static uint32_t lastCurrentReadMs = 0;
static const uint32_t TRANS_READ_INTERVAL_MS   = 250;
static const uint32_t CURRENT_READ_INTERVAL_MS = 250;

// Trans temp pins
static const int TRANS_SENSOR_PIN = 14;
static const int MCP1525_PIN      = 17;

// Current sensor pins
static const int CT_VREF_PIN = 15;
static const int CT_VOUT_PIN = 16;

// MCP1525 / trans temp constants
static const float MCP1525_VOLTS      = 2.500f;
static const float TRANS_PULLUP_VOLTS = 3.300f;
static const float TRANS_PULLUP_OHMS  = 2200.0f;

// Current sensor constants for LA37S200S05
static const float CT_SENSITIVITY_V_PER_A = 0.004f;   // 4.00 mV/A
static const float CT_GAIN = 1.220f;
static const float CT_BIAS_A = 0.0f;

// Steinhart-Hart coefficients from your PDF
static const float SH_A = 1.443335e-03f;
static const float SH_B = 1.722714e-04f;
static const float SH_C = 3.974502e-07f;

// Start all four as no-data
static char q1Value[8] = "---";
static char q2Value[8] = "---";
static char q3Value[8] = "---";
static char q4Value[8] = "---";

// Non-overlapping quadrant regions
static const int TOP_Y0  = 10;
static const int TOP_Y1  = 119;
static const int BOT_Y0  = 120;
static const int BOT_Y1  = 230;

static const int COL0_X0 = 0;
static const int COL0_X1 = 239;
static const int COL1_X0 = 240;
static const int COL1_X1 = 479;

static const uint8_t SCALE_VAL  = 4;
static const uint8_t SCALE_UNIT = 3;
static const uint8_t SCALE_LBL  = 2;

static int computeLabelY(int qy0) {
    int valueTop    = qy0 + 20;
    int valueBottom = valueTop + 7 * SCALE_VAL;
    return valueBottom + 10;
}

static void copyValue(char *dst, const char *src, size_t dstSize) {
    if (!src || dstSize == 0) return;
    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
}

static bool setIfChanged(char *dst, const char *src, size_t dstSize) {
    if (!src) return false;
    if (strncmp(dst, src, dstSize) == 0) return false;
    copyValue(dst, src, dstSize);
    return true;
}

static void drawLabelCentered(int x0, int x1, int y, const char *lbl) {
    int lw = measureTextWidth(lbl, FONT_FUR_CAPS_5x7, SCALE_LBL);
    int cx = (x0 + x1) / 2;
    int lx = cx - lw / 2;
    drawTextScaled(lx, y, lbl, FONT_FUR_CAPS_5x7, AQUA, SCALE_LBL);
}

static int getCenteredValueLeft(
    int x0, int x1,
    const char *value,
    const char *unit,
    const BDFont &fontValue,
    uint8_t scaleValue,
    const BDFont &fontUnit,
    uint8_t scaleUnit
) {
    int digitW = (fontValue.glyphs[0].width + 1) * scaleValue;
    int valueW = digitW * strlen(value);
    int unitW  = measureTextWidth(unit, fontUnit, scaleUnit);

    int totalWidth = valueW + (unitW > 0 ? (digitW / 2) : 0) + unitW;
    int cx = (x0 + x1) / 2;
    return cx - totalWidth / 2;
}

static int getTotalBlockWidth(
    const char *value,
    const char *unit,
    const BDFont &fontValue,
    uint8_t scaleValue,
    const BDFont &fontUnit,
    uint8_t scaleUnit
) {
    int digitW = (fontValue.glyphs[0].width + 1) * scaleValue;
    int valueW = digitW * strlen(value);
    int unitW  = measureTextWidth(unit, fontUnit, scaleUnit);
    return valueW + (unitW > 0 ? (digitW / 2) : 0) + unitW;
}

static void clearValueUnitBlock(
    int x0, int x1, int y0,
    const char *templateValue,
    const char *unit
) {
    int blockX = getCenteredValueLeft(
        x0, x1,
        templateValue, unit,
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT
    );

    int blockW = getTotalBlockWidth(
        templateValue, unit,
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT
    );

    int valueY = y0 + 20;

    fillRect_FAST(
        blockX - 3,
        valueY,
        blockX + blockW + 3,
        valueY + (7 * SCALE_VAL) - 1,
        BLACK
    );
}

static void drawValueUnitOnly(
    int x0, int y0, int x1, int y1,
    const char *value,
    const char *unit
) {
    drawValueUnitCentered(
        x0, y0, x1, y1,
        value, 3, unit,
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT,
        AQUA
    );
}

// ===== Q1 =====

static void drawQ1Full() {
    fillRect_FAST(COL0_X0, TOP_Y0, COL0_X1, TOP_Y1, BLACK);

    drawValueUnitCentered(
        COL0_X0, TOP_Y0, COL0_X1, TOP_Y1,
        q1Value, 3, "°F",
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT,
        AQUA
    );

    drawLabelCentered(COL0_X0, COL0_X1, computeLabelY(TOP_Y0), "AMBIENT TEMP");
}

static void drawQ1ValueOnly() {
    clearValueUnitBlock(COL0_X0, COL0_X1, TOP_Y0, "200", "°F");
    drawValueUnitOnly(COL0_X0, TOP_Y0, COL0_X1, TOP_Y1, q1Value, "°F");
}

// ===== Q2 =====

static void drawQ2Full() {
    fillRect_FAST(COL1_X0, TOP_Y0, COL1_X1, TOP_Y1, BLACK);

    drawValueUnitCentered(
        COL1_X0, TOP_Y0, COL1_X1, TOP_Y1,
        q2Value, 3, "%",
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT,
        AQUA
    );

    drawLabelCentered(COL1_X0, COL1_X1, computeLabelY(TOP_Y0), "HUMIDITY");
}

static void drawQ2ValueOnly() {
    clearValueUnitBlock(COL1_X0, COL1_X1, TOP_Y0, "100", "%");
    drawValueUnitOnly(COL1_X0, TOP_Y0, COL1_X1, TOP_Y1, q2Value, "%");
}

// ===== Q3 =====

static void drawQ3Full() {
    fillRect_FAST(COL0_X0, BOT_Y0, COL0_X1, BOT_Y1, BLACK);

    drawValueUnitCentered(
        COL0_X0, BOT_Y0, COL0_X1, BOT_Y1,
        q3Value, 3, "A",
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT,
        AQUA
    );

    drawLabelCentered(COL0_X0, COL0_X1, computeLabelY(BOT_Y0), "ALTERNATOR");
}

static void drawQ3ValueOnly() {
    clearValueUnitBlock(COL0_X0, COL0_X1, BOT_Y0, "888", "A");
    drawValueUnitOnly(COL0_X0, BOT_Y0, COL0_X1, BOT_Y1, q3Value, "A");
}

// ===== Q4 =====

static void drawQ4Full() {
    fillRect_FAST(COL1_X0, BOT_Y0, COL1_X1, BOT_Y1, BLACK);

    drawValueUnitCentered(
        COL1_X0, BOT_Y0, COL1_X1, BOT_Y1,
        q4Value, 3, "°F",
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT,
        AQUA
    );

    drawLabelCentered(COL1_X0, COL1_X1, computeLabelY(BOT_Y0), "TRANS TEMP");
}

static void drawQ4ValueOnly() {
    clearValueUnitBlock(COL1_X0, COL1_X1, BOT_Y0, "300", "°F");
    drawValueUnitOnly(COL1_X0, BOT_Y0, COL1_X1, BOT_Y1, q4Value, "°F");
}

static void drawAllQuadrants() {
    drawQ1Full();
    drawQ2Full();
    drawQ3Full();
    drawQ4Full();
    Overlay_Warnings_update(false,false,false,false,false);
}

static bool readTransTempF(float &tempFOut) {
    int rawRef    = analogRead(MCP1525_PIN);
    int rawSensor = analogRead(TRANS_SENSOR_PIN);

    if (rawRef <= 0) return false;
    if (rawSensor <= 0) return false;
    if (rawSensor >= rawRef * (TRANS_PULLUP_VOLTS / MCP1525_VOLTS)) return false;

    float voltsPerCount = MCP1525_VOLTS / (float)rawRef;
    float vNode = (float)rawSensor * voltsPerCount;

    if (vNode <= 0.001f) return false;
    if (vNode >= (TRANS_PULLUP_VOLTS - 0.001f)) return false;

    float rSensor = TRANS_PULLUP_OHMS * vNode / (TRANS_PULLUP_VOLTS - vNode);
    if (rSensor <= 0.0f) return false;

    float lnR = logf(rSensor);
    float invT = SH_A + (SH_B * lnR) + (SH_C * lnR * lnR * lnR);
    if (invT <= 0.0f) return false;

    float tempK = 1.0f / invT;
    float tempC = tempK - 273.15f;
    float tempF = (tempC * 1.8f) + 32.0f;

    tempFOut = tempF;
    return true;
}

static bool readCurrentAmps(float &ampsOut) {
    int rawMcp  = analogRead(MCP1525_PIN);
    int rawVref = analogRead(CT_VREF_PIN);
    int rawVout = analogRead(CT_VOUT_PIN);

    if (rawMcp <= 0) return false;

    float voltsPerCount = MCP1525_VOLTS / (float)rawMcp;
    float vref = (float)rawVref * voltsPerCount;
    float vout = (float)rawVout * voltsPerCount;

    float deltaV = (vout - vref);
    float amps = (-deltaV / CT_SENSITIVITY_V_PER_A);
    amps = (amps * CT_GAIN) + CT_BIAS_A;

    if (fabsf(amps) < 2.0f) amps = 0.0f;
    if (amps < 0.0f) amps = 0.0f;

    ampsOut = amps;
    return true;
}

static void updateDht20Readings() {
    if (!dhtReady) return;

    uint32_t now = millis();
    if ((now - lastDhtReadMs) < DHT_READ_INTERVAL_MS) return;
    lastDhtReadMs = now;

    int status = dht.read();
    if (status != DHT20_OK) return;

    float tempF = (dht.getTemperature() * 1.8f) + 32.0f;
    float humRH = dht.getHumidity();

    int tempWhole = (int)lroundf(tempF);
    int humWhole  = (int)lroundf(humRH);

    char buf[8];

    snprintf(buf, sizeof(buf), "%d", tempWhole);
    Page_Env_setQ1Value(buf);

    snprintf(buf, sizeof(buf), "%d", humWhole);
    Page_Env_setQ2Value(buf);
}

static void updateCurrentReadings() {
    if (!analogReady) return;

    uint32_t now = millis();
    if ((now - lastCurrentReadMs) < CURRENT_READ_INTERVAL_MS) return;
    lastCurrentReadMs = now;

    float amps = 0.0f;
    if (!readCurrentAmps(amps)) return;

    int ampsWhole = (int)lroundf(amps);

    if (ampsWhole < 0) ampsWhole = 0;
    if (ampsWhole > 999) ampsWhole = 999;

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", ampsWhole);
    Page_Env_setQ3Value(buf);
}

static void updateTransTempReadings() {
    if (!analogReady) return;

    uint32_t now = millis();
    if ((now - lastTransReadMs) < TRANS_READ_INTERVAL_MS) return;
    lastTransReadMs = now;

    float tempF = 0.0f;
    if (!readTransTempF(tempF)) return;

    int tempWhole = (int)lroundf(tempF);

    char buf[8];
    snprintf(buf, sizeof(buf), "%d", tempWhole);
    Page_Env_setQ4Value(buf);
}

void Page_Env_begin() {
    envDrawn = false;

    copyValue(q1Value, "---", sizeof(q1Value));
    copyValue(q2Value, "---", sizeof(q2Value));
    copyValue(q3Value, "---", sizeof(q3Value));
    copyValue(q4Value, "---", sizeof(q4Value));

    fillScreen_FAST(BLACK);
    setBacklightPWM(255);

    if (!dhtReady) {
        Wire.begin();
        dht.begin();
        dhtReady = true;
    }

    if (!analogReady) {
        analogReadResolution(12);
        analogReadAveraging(16);
        pinMode(TRANS_SENSOR_PIN, INPUT_DISABLE);
        pinMode(MCP1525_PIN, INPUT_DISABLE);
        pinMode(CT_VREF_PIN, INPUT_DISABLE);
        pinMode(CT_VOUT_PIN, INPUT_DISABLE);
        analogReady = true;
    }

    drawAllQuadrants();
    envDrawn = true;
}

void Page_Env_update() {
    TouchPoint tp = readTouch();
    if (tp.touched) {
        PageManager_switchTo(PAGE_PID1);
        return;
    }

    updateDht20Readings();
    updateCurrentReadings();
    updateTransTempReadings();
}

void Page_Env_setQ1Value(const char *value) {
    if (setIfChanged(q1Value, value, sizeof(q1Value)) && envDrawn) {
        drawQ1ValueOnly();
    }
}

void Page_Env_setQ2Value(const char *value) {
    if (setIfChanged(q2Value, value, sizeof(q2Value)) && envDrawn) {
        drawQ2ValueOnly();
    }
}

void Page_Env_setQ3Value(const char *value) {
    if (setIfChanged(q3Value, value, sizeof(q3Value)) && envDrawn) {
        drawQ3ValueOnly();
    }
}

void Page_Env_setQ4Value(const char *value) {
    if (setIfChanged(q4Value, value, sizeof(q4Value)) && envDrawn) {
        drawQ4ValueOnly();
    }
}