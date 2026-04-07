#include "Overlay_Warnings.h"
#include "Display_ST7796S.h"
#include <SD.h>

// ---------------------------------------------------------------------------
// ICON CONFIGURATION
// ---------------------------------------------------------------------------
static const int ICON_W = 96;
static const int ICON_H = 72;
static const int ICON_Y = (320 - ICON_H - 6);   // 6 px above bottom

enum WarningIconIndex
{
    ICON_BRAKE = 0,
    ICON_CHECK_ENGINE,
    ICON_HIGH_BEAM,
    ICON_4WD,
    ICON_SEATBELT
};

static const char* ICON_FILES[5] = {
    "brake.raw",
    "checkengine.raw",
    "highbeam.raw",
    "4WD.raw",
    "seatbelt.raw"
};

// Slot X positions match file order above
static const int ICON_X[5] = {
    0,
    96,
    192,
    288,
    384
};

// Track previous states so we only redraw when something changes
static bool lastBrakeOn       = false;
static bool lastCheckEngineOn = false;
static bool lastHighBeamOn    = false;
static bool lastFourWdOn      = false;
static bool lastSeatbeltOn    = false;
static bool firstRun          = true;

// ---------------------------------------------------------------------------
// FULL ICON RAW DRAW FUNCTION
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

            uint8_t lo = buf[px++];
            uint8_t hi = buf[px++];
            uint16_t color = (hi << 8) | lo;

            drawPixel_FAST(x + xx, y + yy, color);
        }
    }

    f.close();
}

// ---------------------------------------------------------------------------
// CLEAR ICON AREA
// Assumes the overlay background is black.
// If your bottom bar is not black, replace 0x0000 with the correct bg color
// or redraw that section of the page background instead.
// ---------------------------------------------------------------------------
static void clearIconSlot(int x, int y)
{
    fillRect_FAST(x, y, x + ICON_W - 1, y + ICON_H - 1, 0x0000);
}

static void setIconVisible(int iconIndex, bool visible)
{
    if (visible) {
        drawRAW_icon96x72(ICON_FILES[iconIndex], ICON_X[iconIndex], ICON_Y);
    } else {
        clearIconSlot(ICON_X[iconIndex], ICON_Y);
    }
}

// ---------------------------------------------------------------------------
// PUBLIC INTERFACE
// ---------------------------------------------------------------------------
void Overlay_Warnings_begin()
{
    firstRun = true;
    lastBrakeOn       = false;
    lastCheckEngineOn = false;
    lastHighBeamOn    = false;
    lastFourWdOn      = false;
    lastSeatbeltOn    = false;
}

void Overlay_Warnings_update(
    bool seatbeltOn,
    bool brakeOn,
    bool highBeamOn,
    bool checkEngineOn,
    bool fourWdOn
)
{
    if (firstRun || brakeOn != lastBrakeOn) {
        setIconVisible(ICON_BRAKE, brakeOn);
        lastBrakeOn = brakeOn;
    }

    if (firstRun || checkEngineOn != lastCheckEngineOn) {
        setIconVisible(ICON_CHECK_ENGINE, checkEngineOn);
        lastCheckEngineOn = checkEngineOn;
    }

    if (firstRun || highBeamOn != lastHighBeamOn) {
        setIconVisible(ICON_HIGH_BEAM, highBeamOn);
        lastHighBeamOn = highBeamOn;
    }

    if (firstRun || fourWdOn != lastFourWdOn) {
        setIconVisible(ICON_4WD, fourWdOn);
        lastFourWdOn = fourWdOn;
    }

    if (firstRun || seatbeltOn != lastSeatbeltOn) {
        setIconVisible(ICON_SEATBELT, seatbeltOn);
        lastSeatbeltOn = seatbeltOn;
    }

    firstRun = false;
}