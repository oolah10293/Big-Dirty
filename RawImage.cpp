#include "RawImage.h"
#include <SD.h>
#include "Display_ST7796S.h"  // for drawPixel_FAST(), etc.

// Constants for the RAW boot image
static const uint16_t RAW_WIDTH  = 480;
static const uint16_t RAW_HEIGHT = 320;

// Size of the streaming buffer (must be multiple of 2 for RGB565)
static const size_t RAW_BUFFER_SIZE = 512;

void drawRAW480x320(const char *filename)
{
    delay(5);

    // Try to open the file from SD
    File f = SD.open(filename, FILE_READ);
    if (!f) {
        // If it fails, just bail out quietly (screen stays whatever it was)
        // You could also fillScreen_FAST with a solid color here if you want.
        return;
    }

    uint8_t  buffer[RAW_BUFFER_SIZE];
    size_t   bytesInBuffer = 0;
    size_t   bufferIndex   = 0;

    // Helper lambda to refill the buffer when needed
    auto refillBuffer = [&]() -> bool {
        bytesInBuffer = f.read(buffer, RAW_BUFFER_SIZE);
        bufferIndex   = 0;
        return (bytesInBuffer > 0);
    };

    // Prime the buffer
    if (!refillBuffer()) {
        f.close();
        return;
    }

    // Stream through the file and draw pixels row by row
    for (uint16_t y = 0; y < RAW_HEIGHT; y++) {
        for (uint16_t x = 0; x < RAW_WIDTH; x++) {

            // Ensure we have at least 2 bytes available for one pixel
            if (bufferIndex + 1 >= bytesInBuffer) {
                if (!refillBuffer()) {
                    // File ended unexpectedly; stop drawing
                    f.close();
                    return;
                }
            }

            uint8_t hi = buffer[bufferIndex++];
            uint8_t lo = buffer[bufferIndex++];

            // RAW is little-endian RGB565: hi byte first, then lo byte
            uint16_t color = (uint16_t)(hi << 8) | lo;

            // Draw pixel at (x, y) in the current display orientation
            drawPixel_FAST(x, y, color);
        }
    }

    f.close();
}