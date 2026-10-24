#include "RawImage.h"
#include <SD.h>
#include "Display_ST7796S.h"

// Constants for the RAW boot image
static const uint16_t RAW_WIDTH  = 480;
static const uint16_t RAW_HEIGHT = 320;
static const size_t RAW_ROW_BYTES = RAW_WIDTH * 2U;

void drawRAW480x320(const char *filename) {
    delay(5);

    File f = SD.open(filename, FILE_READ);
    if (!f) {
        return;
    }

    static uint8_t rowBytes[RAW_ROW_BYTES];
    static uint16_t rowPixels[RAW_WIDTH];

    for (uint16_t y = 0; y < RAW_HEIGHT; ++y) {
        const size_t bytesRead = f.read(rowBytes, RAW_ROW_BYTES);
        if (bytesRead != RAW_ROW_BYTES) {
            f.close();
            return;
        }

        // Preserve the exact byte interpretation used by the recovered code:
        // first file byte is sent as the RGB565 high byte, second as low.
        for (uint16_t x = 0; x < RAW_WIDTH; ++x) {
            const size_t i = (size_t)x * 2U;
            rowPixels[x] =
                (uint16_t)((uint16_t)rowBytes[i] << 8) |
                (uint16_t)rowBytes[i + 1];
        }

        // One row per TFT transaction; this also services HC58 after each row.
        drawRGB565Row_FAST(0, y, rowPixels, RAW_WIDTH);
    }

    f.close();
}
