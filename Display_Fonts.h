#pragma once
#include <Arduino.h>
#include "Display_ST7796S.h"

// ===== Glyph + Font structures =====
struct BDGlyph {
    uint8_t width;
    uint8_t height;
    const uint8_t *bitmap;
};

struct BDFont {
    uint8_t charFirst;
    uint8_t charLast;
    const BDGlyph *glyphs;
};

// ===== Public fonts (5x7 FUR style)
extern const BDFont FONT_FUR_DIGITS_5x7;  // '0'..'9'
extern const BDFont FONT_FUR_CAPS_5x7;    // 'A'..'Z'

// ===== Text and layout drawing =====
void drawGlyphScaled(int x, int y, char c,
                     const BDFont &font,
                     uint16_t color,
                     uint8_t scale);

void drawTextScaled(int x, int y, const char *text,
                    const BDFont &font,
                    uint16_t color,
                    uint8_t scale);

int measureTextWidth(const char *text,
                     const BDFont &font,
                     uint8_t scale);

// Center full VALUE + UNIT block inside a bounding box
void drawValueUnitCentered(int x0, int y0, int x1, int y1,
                           const char *value,
                           uint8_t fieldDigits,      // expected max digits (2-3-4)
                           const char *unit,
                           const BDFont &fontValue,  // digits font
                           uint8_t scaleValue,
                           const BDFont &fontUnit,   // caps font
                           uint8_t scaleUnit,
                           uint16_t color);
