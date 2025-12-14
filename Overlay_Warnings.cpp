#include "Overlay_Warnings.h"
#include "Display_ST7796S.h"
#include <SD.h>

// ---------------------------------------------------------------------------
// ICON CONFIGURATION
// ---------------------------------------------------------------------------
static const int ICON_W = 96;
static const int ICON_H = 72;
static const int ICON_Y = (320 - ICON_H - 6);   // 6 px above bottom

static const char* ICON_FILES[5] = {
    "brake.raw",
    "checkengine.raw",
    "highbeam.raw",
    "4WD.raw",
    "seatbelt.raw"
};

// ---------------------------------------------------------------------------
// FULL ICON RAW DRAW FUNCTION (embedded by request — independent of RawImage.h)
// Draws a 96×72 RGB565 little-endian icon at (x, y)
// ---------------------------------------------------------------------------
static void drawRAW_icon96x72(const char* filename, int x, int y)
{
    File f = SD.open(filename, FILE_READ);
    if (!f) return;

    const int BUFSIZE = 512;
    uint8_t buf[BUFSIZE];
    int n = f.read(buf, BUFSIZE);
    int px = 0;

    for (uint16_t yy = 0; yy < ICON_H; yy++)
    {
        for (uint16_t xx = 0; xx < ICON_W; xx++)
        {
            if (px + 1 >= n) {
                n = f.read(buf, BUFSIZE);
                px = 0;
                if (!n) { f.close(); return; }
            }

            // RGB565 little-endian: low byte first, then high byte
            uint8_t lo = buf[px++];
            uint8_t hi = buf[px++];
            uint16_t color = (hi << 8) | lo;

            drawPixel_FAST(x + xx, y + yy, color);
        }
    }

    f.close();
}

// ---------------------------------------------------------------------------
// PUBLIC INTERFACE
// ---------------------------------------------------------------------------
void Overlay_Warnings_begin() {
    // Reserved for future features (blink timing, animation, etc.)
}

void Overlay_Warnings_update() {
    int x = 0;
    for (int i = 0; i < 5; i++) {
        drawRAW_icon96x72(ICON_FILES[i], x, ICON_Y);
        x += ICON_W;
    }
}
