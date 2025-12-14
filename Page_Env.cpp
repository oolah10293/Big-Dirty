#include "Page_Env.h"
#include "Display_ST7796S.h"
#include "Display_Fonts.h"
#include "Overlay_Warnings.h"
#include "Touch_XPT2046.h"
#include "PageManager.h"

static bool envDrawn = false;

#define AQUA 0x07FF

void Page_Env_begin() {
    envDrawn = false;
    fillScreen_FAST(0x0000);
    setBacklightPWM(255);
}

void Page_Env_update() {

    if (!envDrawn) {
        envDrawn = true;

        // Vertical layout (overlay strip reserved)
        const int TOP_Y0  = 10;
        const int TOP_Y1  = 159;
        const int BOT_Y0  = 120;
        const int BOT_Y1  = 230;

        const int COL0_X0 = 0;
        const int COL0_X1 = 239;
        const int COL1_X0 = 240;
        const int COL1_X1 = 479;

        const uint8_t SCALE_VAL  = 4;
        const uint8_t SCALE_UNIT = 3;
        const uint8_t SCALE_LBL  = 2;

        auto computeLabelY = [&](int qy0) -> int {
            int valueTop    = qy0 + 20;
            int valueBottom = valueTop + 7 * SCALE_VAL;
            return valueBottom + 10;
        };

        // ====================== Q1 Ambient (top-left) ======================
        drawValueUnitCentered(
            COL0_X0, TOP_Y0, COL0_X1, TOP_Y1,
            "72",   3,  "°F",
            FONT_FUR_DIGITS_5x7, SCALE_VAL,
            FONT_FUR_CAPS_5x7,   SCALE_UNIT,
            AQUA
        );

        {
            const char *lbl = "AMBIENT TEMP";
            int lw = measureTextWidth(lbl, FONT_FUR_CAPS_5x7, SCALE_LBL);
            int cx = (COL0_X0 + COL0_X1) / 2;
            int lx = cx - lw / 2;
            int ly = computeLabelY(TOP_Y0);
            drawTextScaled(lx, ly, lbl, FONT_FUR_CAPS_5x7, AQUA, SCALE_LBL);
        }

        // ====================== Q2 Humidity (top-right) ======================
        drawValueUnitCentered(
            COL1_X0, TOP_Y0, COL1_X1, TOP_Y1,
            "45",   3,  "%",   // now with % symbol
            FONT_FUR_DIGITS_5x7, SCALE_VAL,
            FONT_FUR_CAPS_5x7,   SCALE_UNIT,
            AQUA
        );

        {
            const char *lbl = "HUMIDITY";
            int lw = measureTextWidth(lbl, FONT_FUR_CAPS_5x7, SCALE_LBL);
            int cx = (COL1_X0 + COL1_X1) / 2;
            int lx = cx - lw / 2;
            int ly = computeLabelY(TOP_Y0);
            drawTextScaled(lx, ly, lbl, FONT_FUR_CAPS_5x7, AQUA, SCALE_LBL);
        }

        // ====================== Q3 Alternator (bottom-left) ======================
        drawValueUnitCentered(
            COL0_X0, BOT_Y0, COL0_X1, BOT_Y1,
            "12",   3,  "A",
            FONT_FUR_DIGITS_5x7, SCALE_VAL,
            FONT_FUR_CAPS_5x7,   SCALE_UNIT,
            AQUA
        );

        {
            const char *lbl = "ALTERNATOR";
            int lw = measureTextWidth(lbl, FONT_FUR_CAPS_5x7, SCALE_LBL);
            int cx = (COL0_X0 + COL0_X1) / 2;
            int lx = cx - lw / 2;
            int ly = computeLabelY(BOT_Y0);
            drawTextScaled(lx, ly, lbl, FONT_FUR_CAPS_5x7, AQUA, SCALE_LBL);
        }

        // ====================== Q4 Trans Temp (bottom-right) ======================
        drawValueUnitCentered(
            COL1_X0, BOT_Y0, COL1_X1, BOT_Y1,
            "140",   3,  "°F",
            FONT_FUR_DIGITS_5x7, SCALE_VAL,
            FONT_FUR_CAPS_5x7,   SCALE_UNIT,
            AQUA
        );

        {
            const char *lbl = "TRANS TEMP";
            int lw = measureTextWidth(lbl, FONT_FUR_CAPS_5x7, SCALE_LBL);
            int cx = (COL1_X0 + COL1_X1) / 2;
            int lx = cx - lw / 2;
            int ly = computeLabelY(BOT_Y0);
            drawTextScaled(lx, ly, lbl, FONT_FUR_CAPS_5x7, AQUA, SCALE_LBL);
        }

     Overlay_Warnings_update();

    }

    // Touch → next page
    TouchPoint tp = readTouch();
    if (tp.touched) {
        PageManager_switchTo(PAGE_PID1);
        delay(150);
    }
}
