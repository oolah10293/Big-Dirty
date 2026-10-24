#include "Page_PID2.h"
#include "Display_ST7796S.h"
#include "Display_Fonts.h"
#include "Overlay_Warnings.h"
#include "Touch_XPT2046.h"
#include "PageManager.h"
#include "PID_Manager.h"
#include "HC58_Transport.h"

#include <string.h>

#define AQUA  0x07FF
#define BLACK 0x0000

static bool pid2Drawn = false;

// What is currently drawn
static char q1Value[8] = "---";   // Load
static char q2Value[8] = "---";   // Timing
static char q3Value[8] = "---";   // O2 B1
static char q4Value[8] = "---";   // O2 B2

// Layout (same as others)
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

static int measureMixedLabelWidth(const char *text) {
    if (!text) return 0;

    int px = 0;
    while (*text) {
        char c = *text++;

        if (c >= 'A' && c <= 'Z') {
            px += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * SCALE_LBL;
        }
        else if (c >= '0' && c <= '9') {
            px += (FONT_FUR_DIGITS_5x7.glyphs[0].width + 1) * SCALE_LBL;
        }
        else if (c == ' ') {
            px += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * SCALE_LBL;
        }
    }

    return px;
}

static void drawLabelCentered(int x0, int x1, int y, const char *lbl) {
    int lw = measureMixedLabelWidth(lbl);
    int cx = (x0 + x1) / 2;
    int x  = cx - lw / 2;

    while (*lbl) {
        char c = *lbl++;

        if (c >= 'A' && c <= 'Z') {
            char s[2] = { c, '\0' };
            drawTextScaled(x, y, s, FONT_FUR_CAPS_5x7, AQUA, SCALE_LBL);
            x += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * SCALE_LBL;
        }
        else if (c >= '0' && c <= '9') {
            char s[2] = { c, '\0' };
            drawTextScaled(x, y, s, FONT_FUR_DIGITS_5x7, AQUA, SCALE_LBL);
            x += (FONT_FUR_DIGITS_5x7.glyphs[0].width + 1) * SCALE_LBL;
        }
        else if (c == ' ') {
            x += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * SCALE_LBL;
        }
    }
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
        blockX - 9,
        valueY,
        blockX + blockW + 5,
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

// ===== Q1 CALC LOAD =====
static void drawQ1Full(bool clearBackground) {
    if (clearBackground) {
        fillRect_FAST(COL0_X0, TOP_Y0, COL0_X1, TOP_Y1, BLACK);
    }

    drawValueUnitCentered(
        COL0_X0, TOP_Y0, COL0_X1, TOP_Y1,
        q1Value, 3, "%",
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT,
        AQUA
    );

    drawLabelCentered(COL0_X0, COL0_X1, computeLabelY(TOP_Y0), "CALC LOAD");
}

static void drawQ1ValueOnly() {
    clearValueUnitBlock(COL0_X0, COL0_X1, TOP_Y0, "100", "%");
    drawValueUnitOnly(COL0_X0, TOP_Y0, COL0_X1, TOP_Y1, q1Value, "%");
}

// ===== Q2 TIMING =====
static void drawQ2Full(bool clearBackground) {
    if (clearBackground) {
        fillRect_FAST(COL1_X0, TOP_Y0, COL1_X1, TOP_Y1, BLACK);
    }

    drawValueUnitCentered(
        COL1_X0, TOP_Y0, COL1_X1, TOP_Y1,
        q2Value, 3, "°",
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT,
        AQUA
    );

    drawLabelCentered(COL1_X0, COL1_X1, computeLabelY(TOP_Y0), "TIMING ADV");
}

static void drawQ2ValueOnly() {
    clearValueUnitBlock(COL1_X0, COL1_X1, TOP_Y0, "50", "°");
    drawValueUnitOnly(COL1_X0, TOP_Y0, COL1_X1, TOP_Y1, q2Value, "°");
}

// ===== Q3 O2 B1 =====
static void drawQ3Full(bool clearBackground) {
    if (clearBackground) {
        fillRect_FAST(COL0_X0, BOT_Y0, COL0_X1, BOT_Y1, BLACK);
    }

    drawValueUnitCentered(
        COL0_X0, BOT_Y0, COL0_X1, BOT_Y1,
        q3Value, 3, "mV",
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT,
        AQUA
    );

    drawLabelCentered(COL0_X0, COL0_X1, computeLabelY(BOT_Y0), "O2 B1");
}

static void drawQ3ValueOnly() {
    clearValueUnitBlock(COL0_X0, COL0_X1, BOT_Y0, "1000", "mV");
    drawValueUnitOnly(COL0_X0, BOT_Y0, COL0_X1, BOT_Y1, q3Value, "mV");
}

// ===== Q4 O2 B2 =====
static void drawQ4Full(bool clearBackground) {
    if (clearBackground) {
        fillRect_FAST(COL1_X0, BOT_Y0, COL1_X1, BOT_Y1, BLACK);
    }

    drawValueUnitCentered(
        COL1_X0, BOT_Y0, COL1_X1, BOT_Y1,
        q4Value, 3, "mV",
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT,
        AQUA
    );

    drawLabelCentered(COL1_X0, COL1_X1, computeLabelY(BOT_Y0), "O2 B2");
}

static void drawQ4ValueOnly() {
    clearValueUnitBlock(COL1_X0, COL1_X1, BOT_Y0, "1000", "mV");
    drawValueUnitOnly(COL1_X0, BOT_Y0, COL1_X1, BOT_Y1, q4Value, "mV");
}

static void drawAllQuadrants(bool clearBackground) {
    drawQ1Full(clearBackground);
    HC58Transport::service();
    drawQ2Full(clearBackground);
    HC58Transport::service();
    drawQ3Full(clearBackground);
    HC58Transport::service();
    drawQ4Full(clearBackground);
    HC58Transport::service();
}

// ===== Sync =====

static void setQ1(const char *v) { if (setIfChanged(q1Value, v, sizeof(q1Value)) && pid2Drawn) drawQ1ValueOnly(); }
static void setQ2(const char *v) { if (setIfChanged(q2Value, v, sizeof(q2Value)) && pid2Drawn) drawQ2ValueOnly(); }
static void setQ3(const char *v) { if (setIfChanged(q3Value, v, sizeof(q3Value)) && pid2Drawn) drawQ3ValueOnly(); }
static void setQ4(const char *v) { if (setIfChanged(q4Value, v, sizeof(q4Value)) && pid2Drawn) drawQ4ValueOnly(); }

static void syncFromManager() {
    setQ1(PIDManager_getDisplayText(PID_SLOT_Q1));
    setQ2(PIDManager_getDisplayText(PID_SLOT_Q2));
    setQ3(PIDManager_getDisplayText(PID_SLOT_Q3));
    setQ4(PIDManager_getDisplayText(PID_SLOT_Q4));
}

// ===== Lifecycle =====

void Page_PID2_begin() {
    pid2Drawn = false;

    PIDManager_setPage(PAGE_PID2);

    copyValue(q1Value, PIDManager_getDisplayText(PID_SLOT_Q1), sizeof(q1Value));
    copyValue(q2Value, PIDManager_getDisplayText(PID_SLOT_Q2), sizeof(q2Value));
    copyValue(q3Value, PIDManager_getDisplayText(PID_SLOT_Q3), sizeof(q3Value));
    copyValue(q4Value, PIDManager_getDisplayText(PID_SLOT_Q4), sizeof(q4Value));

    setBacklightPWM(255);

    drawAllQuadrants(!PageManager_transitionClearedScreen());
    pid2Drawn = true;
}

void Page_PID2_update() {
    TouchPoint tp = readTouch();
    if (tp.touched) {
        PageManager_switchTo(PAGE_PID3);
        return;
    }

    PIDManager_update();
    syncFromManager();
}
