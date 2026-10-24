#include "Page_PID3.h"
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

static bool pid3Drawn = false;

static char q1Value[8] = "---";   // STFT1
static char q2Value[8] = "---";   // LTFT1
static char q3Value[8] = "---";   // STFT2
static char q4Value[8] = "---";   // LTFT2

// Layout
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

// ===== Mixed label renderer (reuse from PID2) =====

static int measureMixedLabelWidth(const char *text) {
    int px = 0;
    while (*text) {
        char c = *text++;
        if (c >= 'A' && c <= 'Z')
            px += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * SCALE_LBL;
        else if (c >= '0' && c <= '9')
            px += (FONT_FUR_DIGITS_5x7.glyphs[0].width + 1) * SCALE_LBL;
        else if (c == ' ')
            px += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * SCALE_LBL;
    }
    return px;
}

static void drawLabelCentered(int x0, int x1, int y, const char *lbl) {
    int lw = measureMixedLabelWidth(lbl);
    int cx = (x0 + x1) / 2;
    int x  = cx - lw / 2;

    while (*lbl) {
        char c = *lbl++;

        char s[2] = { c, '\0' };

        if (c >= 'A' && c <= 'Z') {
            drawTextScaled(x, y, s, FONT_FUR_CAPS_5x7, AQUA, SCALE_LBL);
            x += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * SCALE_LBL;
        }
        else if (c >= '0' && c <= '9') {
            drawTextScaled(x, y, s, FONT_FUR_DIGITS_5x7, AQUA, SCALE_LBL);
            x += (FONT_FUR_DIGITS_5x7.glyphs[0].width + 1) * SCALE_LBL;
        }
        else if (c == ' ') {
            x += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * SCALE_LBL;
        }
    }
}

// ===== Shared helpers =====

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
    return ((x0 + x1) / 2) - totalWidth / 2;
}

static int getTotalBlockWidth(
    const char *value,
    const char *unit
) {
    int digitW = (FONT_FUR_DIGITS_5x7.glyphs[0].width + 1) * SCALE_VAL;
    int valueW = digitW * strlen(value);
    int unitW  = measureTextWidth(unit, FONT_FUR_CAPS_5x7, SCALE_UNIT);
    return valueW + (unitW > 0 ? (digitW / 2) : 0) + unitW;
}

static void clearBlock(int x0, int x1, int y0, const char *tmpl, const char *unit) {
    int x = getCenteredValueLeft(
        x0, x1, tmpl, unit,
        FONT_FUR_DIGITS_5x7, SCALE_VAL,
        FONT_FUR_CAPS_5x7,   SCALE_UNIT
    );

    int w = getTotalBlockWidth(tmpl, unit);
    int y = y0 + 20;

    fillRect_FAST(x - 3, y, x + w + 3, y + (7 * SCALE_VAL) - 1, BLACK);
}

// ===== Quadrants =====

#define DRAW_Q_FULL(x0,y0,x1,y1,val,label,clearBackground) do { \
    if (clearBackground) fillRect_FAST(x0,y0,x1,y1,BLACK); \
    drawValueUnitCentered(x0,y0,x1,y1,val,3,"%", \
        FONT_FUR_DIGITS_5x7,SCALE_VAL, \
        FONT_FUR_CAPS_5x7,SCALE_UNIT,AQUA); \
    drawLabelCentered(x0,x1,computeLabelY(y0),label); \
} while (0)

#define DRAW_Q_VAL(x0,y0,x1,y1,val) \
    clearBlock(x0,x1,y0,"-100","%"); \
    drawValueUnitCentered(x0,y0,x1,y1,val,3,"%", \
        FONT_FUR_DIGITS_5x7,SCALE_VAL, \
        FONT_FUR_CAPS_5x7,SCALE_UNIT,AQUA);

// Q1
static void drawQ1Full(bool clearBackground){ DRAW_Q_FULL(COL0_X0,TOP_Y0,COL0_X1,TOP_Y1,q1Value,"STFT1",clearBackground); }
static void drawQ1Val(){  DRAW_Q_VAL (COL0_X0,TOP_Y0,COL0_X1,TOP_Y1,q1Value); }

// Q2
static void drawQ2Full(bool clearBackground){ DRAW_Q_FULL(COL1_X0,TOP_Y0,COL1_X1,TOP_Y1,q2Value,"LTFT1",clearBackground); }
static void drawQ2Val(){  DRAW_Q_VAL (COL1_X0,TOP_Y0,COL1_X1,TOP_Y1,q2Value); }

// Q3
static void drawQ3Full(bool clearBackground){ DRAW_Q_FULL(COL0_X0,BOT_Y0,COL0_X1,BOT_Y1,q3Value,"STFT2",clearBackground); }
static void drawQ3Val(){  DRAW_Q_VAL (COL0_X0,BOT_Y0,COL0_X1,BOT_Y1,q3Value); }

// Q4
static void drawQ4Full(bool clearBackground){ DRAW_Q_FULL(COL1_X0,BOT_Y0,COL1_X1,BOT_Y1,q4Value,"LTFT2",clearBackground); }
static void drawQ4Val(){  DRAW_Q_VAL (COL1_X0,BOT_Y0,COL1_X1,BOT_Y1,q4Value); }

static void drawAll(bool clearBackground) {
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

static void setQ1(const char *v){ if(setIfChanged(q1Value,v,sizeof(q1Value))&&pid3Drawn)drawQ1Val();}
static void setQ2(const char *v){ if(setIfChanged(q2Value,v,sizeof(q2Value))&&pid3Drawn)drawQ2Val();}
static void setQ3(const char *v){ if(setIfChanged(q3Value,v,sizeof(q3Value))&&pid3Drawn)drawQ3Val();}
static void setQ4(const char *v){ if(setIfChanged(q4Value,v,sizeof(q4Value))&&pid3Drawn)drawQ4Val();}

static void sync() {
    setQ1(PIDManager_getDisplayText(PID_SLOT_Q1));
    setQ2(PIDManager_getDisplayText(PID_SLOT_Q2));
    setQ3(PIDManager_getDisplayText(PID_SLOT_Q3));
    setQ4(PIDManager_getDisplayText(PID_SLOT_Q4));
}

// ===== Lifecycle =====

void Page_PID3_begin() {
    pid3Drawn = false;

    PIDManager_setPage(PAGE_PID3);

    copyValue(q1Value, PIDManager_getDisplayText(PID_SLOT_Q1), sizeof(q1Value));
    copyValue(q2Value, PIDManager_getDisplayText(PID_SLOT_Q2), sizeof(q2Value));
    copyValue(q3Value, PIDManager_getDisplayText(PID_SLOT_Q3), sizeof(q3Value));
    copyValue(q4Value, PIDManager_getDisplayText(PID_SLOT_Q4), sizeof(q4Value));

    setBacklightPWM(255);

    drawAll(!PageManager_transitionClearedScreen());
    pid3Drawn = true;
}

void Page_PID3_update() {
    TouchPoint tp = readTouch();
    if (tp.touched) {
        PageManager_switchTo(PAGE_DTC);
        return;
    }

    PIDManager_update();
    sync();
}
