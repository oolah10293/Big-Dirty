#include "Page_PID1.h"
#include "Display_ST7796S.h"
#include "Display_Fonts.h"
#include "Overlay_Warnings.h"
#include "Touch_XPT2046.h"
#include "PageManager.h"
#include "PID_Manager.h"

#include <string.h>

#define AQUA  0x07FF
#define BLACK 0x0000

static bool pid1Drawn = false;

// What is currently drawn on screen
static char q1Value[8] = "---";   // TPS
static char q2Value[8] = "---";   // IAT
static char q3Value[8] = "---";   // MAP
static char q4Value[8] = "---";   // MAF

// Same quadrant layout as ENV
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
        blockX,
        valueY,
        blockX + blockW - 1,
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

// ===== Q1 TPS =====

static void drawQ1Full() {
    fillRect_FAST(COL0_X0, TOP_Y0, COL0_X1, TOP_Y1, BLACK);

    drawValueUnitCentered(
        COL0_X0, TOP_Y0, COL0_X1, TOP_Y1,
        q1Value, 3, "%",
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT,
        AQUA
    );

    drawLabelCentered(COL0_X0, COL0_X1, computeLabelY(TOP_Y0), "THROTTLE POS");
}

static void drawQ1ValueOnly() {
    clearValueUnitBlock(COL0_X0, COL0_X1, TOP_Y0, "100", "%");
    drawValueUnitOnly(COL0_X0, TOP_Y0, COL0_X1, TOP_Y1, q1Value, "%");
}

// ===== Q2 IAT =====

static void drawQ2Full() {
    fillRect_FAST(COL1_X0, TOP_Y0, COL1_X1, TOP_Y1, BLACK);

    drawValueUnitCentered(
        COL1_X0, TOP_Y0, COL1_X1, TOP_Y1,
        q2Value, 3, "°F",
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT,
        AQUA
    );

    drawLabelCentered(COL1_X0, COL1_X1, computeLabelY(TOP_Y0), "INTAKE TEMP");
}

static void drawQ2ValueOnly() {
    clearValueUnitBlock(COL1_X0, COL1_X1, TOP_Y0, "200", "°F");
    drawValueUnitOnly(COL1_X0, TOP_Y0, COL1_X1, TOP_Y1, q2Value, "°F");
}

// ===== Q3 MAP =====

static void drawQ3Full() {
    fillRect_FAST(COL0_X0, BOT_Y0, COL0_X1, BOT_Y1, BLACK);

    drawValueUnitCentered(
        COL0_X0, BOT_Y0, COL0_X1, BOT_Y1,
        q3Value, 3, "KPA",
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT,
        AQUA
    );

    drawLabelCentered(COL0_X0, COL0_X1, computeLabelY(BOT_Y0), "MANIFOLD PRES");
}

static void drawQ3ValueOnly() {
    clearValueUnitBlock(COL0_X0, COL0_X1, BOT_Y0, "255", "KPA");
    drawValueUnitOnly(COL0_X0, BOT_Y0, COL0_X1, BOT_Y1, q3Value, "KPA");
}

// ===== Q4 MAF =====

static void drawQ4Full() {
    fillRect_FAST(COL1_X0, BOT_Y0, COL1_X1, BOT_Y1, BLACK);

    drawValueUnitCentered(
        COL1_X0, BOT_Y0, COL1_X1, BOT_Y1,
        q4Value, 3, "G/S",
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT,
        AQUA
    );

    drawLabelCentered(COL1_X0, COL1_X1, computeLabelY(BOT_Y0), "MASS AIRFLOW");
}

static void drawQ4ValueOnly() {
    clearValueUnitBlock(COL1_X0, COL1_X1, BOT_Y0, "255", "G/S");
    drawValueUnitOnly(COL1_X0, BOT_Y0, COL1_X1, BOT_Y1, q4Value, "G/S");
}

static void drawAllQuadrants() {
    drawQ1Full();
    drawQ2Full();
    drawQ3Full();
    drawQ4Full();
    Overlay_Warnings_update(false, false, false, false, false);
}

static void Page_PID1_setQ1Value(const char *value) {
    if (setIfChanged(q1Value, value, sizeof(q1Value)) && pid1Drawn) {
        drawQ1ValueOnly();
    }
}

static void Page_PID1_setQ2Value(const char *value) {
    if (setIfChanged(q2Value, value, sizeof(q2Value)) && pid1Drawn) {
        drawQ2ValueOnly();
    }
}

static void Page_PID1_setQ3Value(const char *value) {
    if (setIfChanged(q3Value, value, sizeof(q3Value)) && pid1Drawn) {
        drawQ3ValueOnly();
    }
}

static void Page_PID1_setQ4Value(const char *value) {
    if (setIfChanged(q4Value, value, sizeof(q4Value)) && pid1Drawn) {
        drawQ4ValueOnly();
    }
}

static void syncFromPIDManager() {
    Page_PID1_setQ1Value(PIDManager_getDisplayText(PID_SLOT_Q1));
    Page_PID1_setQ2Value(PIDManager_getDisplayText(PID_SLOT_Q2));
    Page_PID1_setQ3Value(PIDManager_getDisplayText(PID_SLOT_Q3));
    Page_PID1_setQ4Value(PIDManager_getDisplayText(PID_SLOT_Q4));
}

void Page_PID1_begin() {
    pid1Drawn = false;

    PIDManager_setPage(PAGE_PID1);

    copyValue(q1Value, PIDManager_getDisplayText(PID_SLOT_Q1), sizeof(q1Value));
    copyValue(q2Value, PIDManager_getDisplayText(PID_SLOT_Q2), sizeof(q2Value));
    copyValue(q3Value, PIDManager_getDisplayText(PID_SLOT_Q3), sizeof(q3Value));
    copyValue(q4Value, PIDManager_getDisplayText(PID_SLOT_Q4), sizeof(q4Value));

    fillScreen_FAST(BLACK);
    setBacklightPWM(255);

    drawAllQuadrants();
    pid1Drawn = true;
}

void Page_PID1_update() {
    TouchPoint tp = readTouch();
    if (tp.touched) {
        PageManager_switchTo(PAGE_PID2);
        return;
    }

    PIDManager_update();
    syncFromPIDManager();
}