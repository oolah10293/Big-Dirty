#include "Display_ST7796S.h"
#include "Display_Fonts.h"
#include "Touch_XPT2046.h"
#include "PageManager.h"
#include "Page_DTC.h"
#include "DTC_Manager.h"

#define AQUA   0x07FF
#define WHITE  0xFFFF
#define BLACK  0x0000
#define GRAY   0x8410
#define DARK   0x4208
#define LIGHT  0xC618

static bool dtcDrawn = false;
static bool clearPressed = false;
static DTCState lastState = DTC_STATE_UNREAD;
static int lastCount = -1;

// ===== Layout =====
static const int LIST_Y0 = 10;
static const int LIST_Y1 = 220;

static const int BUTTON_W = 180;
static const int BUTTON_H = 50;
static const int BUTTON_X0 = (480 - BUTTON_W) / 2;
static const int BUTTON_Y0 = 260;
static const int BUTTON_X1 = BUTTON_X0 + BUTTON_W;
static const int BUTTON_Y1 = BUTTON_Y0 + BUTTON_H;

// Fonts
static const uint8_t SCALE_CODE  = 2;
static const uint8_t SCALE_EMPTY = 3;

// ===== Helpers =====

static void fillRect(int x0, int y0, int x1, int y1, uint16_t color) {
    fillRect_FAST(x0, y0, x1, y1, color);
}

static bool pointInRect(int x, int y, int x0, int y0, int x1, int y1) {
    return (x >= x0 && x <= x1 && y >= y0 && y <= y1);
}

static void drawHyphenScaled(int x, int y, uint16_t color, uint8_t scale) {
    int barY = y + (3 * scale);
    int barW = 5 * scale;
    int barH = scale;

    for (int yy = 0; yy < barH; yy++) {
        for (int xx = 0; xx < barW; xx++) {
            drawPixel_FAST(x + xx, barY + yy, color);
        }
    }
}

static int measureDtcTextMixed(const char *text, uint8_t scale) {
    if (!text) return 0;

    int px = 0;
    while (*text) {
        char c = *text++;

        if (c >= '0' && c <= '9') {
            px += (FONT_FUR_DIGITS_5x7.glyphs[0].width + 1) * scale;
            continue;
        }

        if (c >= 'A' && c <= 'Z') {
            px += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * scale;
            continue;
        }

        if (c == ' ') {
            px += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * scale;
            continue;
        }

        if (c == '-') {
            px += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * scale;
            continue;
        }

        px += 2 * scale;
    }

    return px;
}

static void drawDtcTextMixed(int x, int y, const char *text, uint16_t color, uint8_t scale) {
    if (!text) return;

    int cx = x;

    while (*text) {
        char c = *text++;

        if (c >= '0' && c <= '9') {
            drawGlyphScaled(cx, y, c, FONT_FUR_DIGITS_5x7, color, scale);
            cx += (FONT_FUR_DIGITS_5x7.glyphs[0].width + 1) * scale;
            continue;
        }

        if (c >= 'A' && c <= 'Z') {
            drawGlyphScaled(cx, y, c, FONT_FUR_CAPS_5x7, color, scale);
            cx += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * scale;
            continue;
        }

        if (c == ' ') {
            cx += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * scale;
            continue;
        }

        if (c == '-') {
            drawHyphenScaled(cx, y, color, scale);
            cx += (FONT_FUR_CAPS_5x7.glyphs[0].width + 1) * scale;
            continue;
        }

        cx += 2 * scale;
    }
}

static void drawCenteredMixedLine(int y, const char *text, uint16_t color, uint8_t scale) {
    int w = measureDtcTextMixed(text, scale);
    int x = (480 - w) / 2;
    drawDtcTextMixed(x, y, text, color, scale);
}

// ===== Draw list / state =====

static void drawListArea() {
    fillRect(0, LIST_Y0, 479, LIST_Y1, BLACK);

    DTCState state = DTCManager_getState();

    if (state == DTC_STATE_PRESENT) {
        int rowY = LIST_Y0 + 10;
        int rowHeight = 40;
        int count = DTCManager_getCount();

        for (int i = 0; i < count; i++) {
            const char* code = DTCManager_getCode(i);
            if (!code) continue;
            drawDtcTextMixed(20, rowY, code, AQUA, SCALE_CODE);
            rowY += rowHeight;
        }
        return;
    }

    if (state == DTC_STATE_NONE) {
        drawCenteredMixedLine(((LIST_Y0 + LIST_Y1) / 2) - 10, "NO DIAGNOSTIC CODES", AQUA, SCALE_EMPTY);
        return;
    }

    if (state == DTC_STATE_ERROR) {
        drawCenteredMixedLine(((LIST_Y0 + LIST_Y1) / 2) - 10, "CODE READ ERROR", AQUA, SCALE_EMPTY);
        return;
    }

    if (state == DTC_STATE_REQUEST_ERROR) {
        drawCenteredMixedLine(((LIST_Y0 + LIST_Y1) / 2) - 10, "CODE REQUEST ERROR", AQUA, SCALE_EMPTY);
        return;
    }

    // UNREAD / READING / CLEARING all show placeholder
    drawCenteredMixedLine(((LIST_Y0 + LIST_Y1) / 2) - 10, "SCANNING CODES", AQUA, SCALE_EMPTY);
}

// ===== Draw button =====

static void clearButtonArea() {
    fillRect(0, BUTTON_Y0, 479, 319, BLACK);
}

static void drawButton() {
    if (!DTCManager_showClearButton()) {
        clearButtonArea();
        return;
    }

    uint16_t fill = clearPressed ? DARK : GRAY;

    fillRect(BUTTON_X0, BUTTON_Y0, BUTTON_X1, BUTTON_Y1, fill);

    uint16_t tl = clearPressed ? DARK : LIGHT;
    uint16_t br = clearPressed ? LIGHT : DARK;

    fillRect(BUTTON_X0, BUTTON_Y0, BUTTON_X1, BUTTON_Y0 + 2, tl);
    fillRect(BUTTON_X0, BUTTON_Y0, BUTTON_X0 + 2, BUTTON_Y1, tl);
    fillRect(BUTTON_X0, BUTTON_Y1 - 2, BUTTON_X1, BUTTON_Y1, br);
    fillRect(BUTTON_X1 - 2, BUTTON_Y0, BUTTON_X1, BUTTON_Y1, br);

    const char* txt = "CLEAR";
    int w = measureTextWidth(txt, FONT_FUR_CAPS_5x7, 3);
    int x = (480 - w) / 2;
    int y = BUTTON_Y0 + 12;

    drawTextScaled(x, y, txt, FONT_FUR_CAPS_5x7, WHITE, 3);
}

// ===== Redraw sync =====

static void syncDTCPageIfNeeded() {
    DTCState state = DTCManager_getState();
    int count = DTCManager_getCount();

    bool changed = (!dtcDrawn) || (state != lastState) || (count != lastCount);

    if (!changed) return;

    drawListArea();
    drawButton();

    dtcDrawn = true;
    lastState = state;
    lastCount = count;

    if (state != DTC_STATE_PRESENT) {
        clearPressed = false;
    }
}

// ===== Page lifecycle =====

void Page_DTC_begin() {
    dtcDrawn = false;
    clearPressed = false;
    lastState = DTC_STATE_UNREAD;
    lastCount = -1;

    fillScreen_FAST(BLACK);
    setBacklightPWM(255);

    DTCManager_enterPage();
}

void Page_DTC_update() {
    DTCManager_update();
    syncDTCPageIfNeeded();

    TouchPoint tp = readTouch();
    if (!tp.touched) return;

    if (DTCManager_showClearButton() &&
        pointInRect(tp.x, tp.y, BUTTON_X0, BUTTON_Y0, BUTTON_X1, BUTTON_Y1)) {

        if (!clearPressed) {
            clearPressed = true;
            drawButton();
            DTCManager_requestClear();
        }
        return;
    }

    PageManager_switchTo(PAGE_ENV);
}