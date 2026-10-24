#include "Overlay_Warnings.h"
#include "Display_ST7796S.h"
#include <SD.h>

// ---------------------------------------------------------------------------
// ICON CONFIGURATION
// ---------------------------------------------------------------------------
static const int ICON_W = 96;
static const int ICON_H = 72;
static const int ICON_Y = (320 - ICON_H - 6);   // 6 px above bottom

enum WarningIconIndex {
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

static const int ICON_X[5] = {
    0,
    96,
    192,
    288,
    384
};

static bool lastBrakeOn       = false;
static bool lastCheckEngineOn = false;
static bool lastHighBeamOn    = false;
static bool lastFourWdOn      = false;
static bool lastSeatbeltOn    = false;
static bool stateKnown        = false;

// ---------------------------------------------------------------------------
// ROW-STREAMED ICON DRAW
// 96x72 RGB565 little-endian file -> host-order row -> display high-byte first.
// The display row helper services HC58 after every row.
// ---------------------------------------------------------------------------
static void drawRAW_icon96x72(const char* filename, int x, int y) {
    File f = SD.open(filename, FILE_READ);
    if (!f) return;

    static uint8_t rowBytes[ICON_W * 2];
    static uint16_t rowPixels[ICON_W];

    for (uint16_t yy = 0; yy < ICON_H; ++yy) {
        const size_t bytesRead = f.read(rowBytes, sizeof(rowBytes));
        if (bytesRead != sizeof(rowBytes)) {
            f.close();
            return;
        }

        for (uint16_t xx = 0; xx < ICON_W; ++xx) {
            const size_t i = (size_t)xx * 2U;
            const uint8_t lo = rowBytes[i];
            const uint8_t hi = rowBytes[i + 1];
            rowPixels[xx] = (uint16_t)((uint16_t)hi << 8) | lo;
        }

        drawRGB565Row_FAST(x, y + yy, rowPixels, ICON_W);
    }

    f.close();
}

static void clearIconSlot(int x, int y) {
    fillRect_FAST(x, y, x + ICON_W - 1, y + ICON_H - 1, 0x0000);
}

static void setIconVisible(int iconIndex, bool visible) {
    if (visible) {
        drawRAW_icon96x72(ICON_FILES[iconIndex], ICON_X[iconIndex], ICON_Y);
    } else {
        clearIconSlot(ICON_X[iconIndex], ICON_Y);
    }
}

// ---------------------------------------------------------------------------
// PUBLIC INTERFACE
// ---------------------------------------------------------------------------
void Overlay_Warnings_begin() {
    lastBrakeOn       = false;
    lastCheckEngineOn = false;
    lastHighBeamOn    = false;
    lastFourWdOn      = false;
    lastSeatbeltOn    = false;
    stateKnown        = false;
}

void Overlay_Warnings_screenCleared() {
    // A full black page clear guarantees that no warning icon is physically
    // present. The next update only needs to draw warnings that are actually on.
    lastBrakeOn       = false;
    lastCheckEngineOn = false;
    lastHighBeamOn    = false;
    lastFourWdOn      = false;
    lastSeatbeltOn    = false;
    stateKnown        = true;
}

void Overlay_Warnings_update(
    bool seatbeltOn,
    bool brakeOn,
    bool highBeamOn,
    bool checkEngineOn,
    bool fourWdOn
) {
    if (!stateKnown || brakeOn != lastBrakeOn) {
        setIconVisible(ICON_BRAKE, brakeOn);
        lastBrakeOn = brakeOn;
    }

    if (!stateKnown || checkEngineOn != lastCheckEngineOn) {
        setIconVisible(ICON_CHECK_ENGINE, checkEngineOn);
        lastCheckEngineOn = checkEngineOn;
    }

    if (!stateKnown || highBeamOn != lastHighBeamOn) {
        setIconVisible(ICON_HIGH_BEAM, highBeamOn);
        lastHighBeamOn = highBeamOn;
    }

    if (!stateKnown || fourWdOn != lastFourWdOn) {
        setIconVisible(ICON_4WD, fourWdOn);
        lastFourWdOn = fourWdOn;
    }

    if (!stateKnown || seatbeltOn != lastSeatbeltOn) {
        setIconVisible(ICON_SEATBELT, seatbeltOn);
        lastSeatbeltOn = seatbeltOn;
    }

    stateKnown = true;
}
