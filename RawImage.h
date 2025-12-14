#ifndef RAWIMAGE_H
#define RAWIMAGE_H

#include <Arduino.h>

// Draw a 480x320 RGB565 (little-endian) RAW file fullscreen at (0,0).
// Requirements:
//  - File must exist on the SD card root (e.g. "/bowtie.raw")
//  - File format: 480x320 pixels, RGB565, little-endian, no header
//  - Display must already be initialized
//  - SD.begin(BUILTIN_SDCARD) must have been called in setup()
void drawRAW480x320(const char *filename);

#endif // RAWIMAGE_H
