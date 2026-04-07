#include "Display_Fonts.h"
#include <string.h>

// =======================================================
// 5×7 FUR-style digit font ('0'..'9')
// =======================================================

static const uint8_t glyph_0_bits[7] = {0x70,0x88,0x98,0xA8,0xC8,0x88,0x70};
static const uint8_t glyph_1_bits[7] = {0x20,0x60,0xA0,0x20,0x20,0x20,0xF8};
static const uint8_t glyph_2_bits[7] = {0x70,0x88,0x08,0x10,0x20,0x40,0xF8};
static const uint8_t glyph_3_bits[7] = {0x70,0x88,0x08,0x70,0x08,0x88,0x70};
static const uint8_t glyph_4_bits[7] = {0x10,0x30,0x50,0x90,0xF8,0x10,0x10};
static const uint8_t glyph_5_bits[7] = {0xF8,0x80,0x80,0xF0,0x08,0x88,0x70};
static const uint8_t glyph_6_bits[7] = {0x70,0x88,0x80,0xF0,0x88,0x88,0x70};
static const uint8_t glyph_7_bits[7] = {0xF8,0x08,0x10,0x20,0x40,0x40,0x40};
static const uint8_t glyph_8_bits[7] = {0x70,0x88,0x88,0x70,0x88,0x88,0x70};
static const uint8_t glyph_9_bits[7] = {0x70,0x88,0x88,0x78,0x08,0x88,0x70};

static const BDGlyph furDigits_glyphs[10] = {
    {5,7,glyph_0_bits},{5,7,glyph_1_bits},{5,7,glyph_2_bits},{5,7,glyph_3_bits},{5,7,glyph_4_bits},
    {5,7,glyph_5_bits},{5,7,glyph_6_bits},{5,7,glyph_7_bits},{5,7,glyph_8_bits},{5,7,glyph_9_bits}
};

const BDFont FONT_FUR_DIGITS_5x7 = {'0','9',furDigits_glyphs};

// =======================================================
// 5×7 CAPS font ('A'..'Z')
// =======================================================

#define ROW(x) (x)

static const uint8_t glyph_A_bits[7] = {ROW(0x70),ROW(0x88),ROW(0x88),ROW(0xF8),ROW(0x88),ROW(0x88),ROW(0x88)};
static const uint8_t glyph_B_bits[7] = {ROW(0xF0),ROW(0x88),ROW(0x88),ROW(0xF0),ROW(0x88),ROW(0x88),ROW(0xF0)};
static const uint8_t glyph_C_bits[7] = {ROW(0x70),ROW(0x88),ROW(0x80),ROW(0x80),ROW(0x80),ROW(0x88),ROW(0x70)};
static const uint8_t glyph_D_bits[7] = {ROW(0xE0),ROW(0x90),ROW(0x88),ROW(0x88),ROW(0x88),ROW(0x90),ROW(0xE0)};
static const uint8_t glyph_E_bits[7] = {ROW(0xF8),ROW(0x80),ROW(0x80),ROW(0xF0),ROW(0x80),ROW(0x80),ROW(0xF8)};
static const uint8_t glyph_F_bits[7] = {ROW(0xF8),ROW(0x80),ROW(0x80),ROW(0xF0),ROW(0x80),ROW(0x80),ROW(0x80)};
static const uint8_t glyph_G_bits[7] = {ROW(0x70),ROW(0x88),ROW(0x80),ROW(0xB8),ROW(0x88),ROW(0x88),ROW(0x70)};
static const uint8_t glyph_H_bits[7] = {ROW(0x88),ROW(0x88),ROW(0x88),ROW(0xF8),ROW(0x88),ROW(0x88),ROW(0x88)};
static const uint8_t glyph_I_bits[7] = {ROW(0x70),ROW(0x20),ROW(0x20),ROW(0x20),ROW(0x20),ROW(0x20),ROW(0x70)};
static const uint8_t glyph_J_bits[7] = {ROW(0x78),ROW(0x10),ROW(0x10),ROW(0x10),ROW(0x10),ROW(0x90),ROW(0x60)};
static const uint8_t glyph_K_bits[7] = {ROW(0x88),ROW(0x90),ROW(0xA0),ROW(0xC0),ROW(0xA0),ROW(0x90),ROW(0x88)};
static const uint8_t glyph_L_bits[7] = {ROW(0x80),ROW(0x80),ROW(0x80),ROW(0x80),ROW(0x80),ROW(0x80),ROW(0xF8)};
static const uint8_t glyph_M_bits[7] = {ROW(0x88),ROW(0xD8),ROW(0xA8),ROW(0xA8),ROW(0x88),ROW(0x88),ROW(0x88)};
static const uint8_t glyph_N_bits[7] = {ROW(0x88),ROW(0xC8),ROW(0xA8),ROW(0x98),ROW(0x88),ROW(0x88),ROW(0x88)};
static const uint8_t glyph_O_bits[7] = {ROW(0x70),ROW(0x88),ROW(0x88),ROW(0x88),ROW(0x88),ROW(0x88),ROW(0x70)};
static const uint8_t glyph_P_bits[7] = {ROW(0xF0),ROW(0x88),ROW(0x88),ROW(0xF0),ROW(0x80),ROW(0x80),ROW(0x80)};
static const uint8_t glyph_Q_bits[7] = {ROW(0x70),ROW(0x88),ROW(0x88),ROW(0x88),ROW(0xA8),ROW(0x90),ROW(0x68)};
static const uint8_t glyph_R_bits[7] = {ROW(0xF0),ROW(0x88),ROW(0x88),ROW(0xF0),ROW(0xA0),ROW(0x90),ROW(0x88)};
static const uint8_t glyph_S_bits[7] = {ROW(0x70),ROW(0x88),ROW(0x80),ROW(0x70),ROW(0x08),ROW(0x88),ROW(0x70)};
static const uint8_t glyph_T_bits[7] = {ROW(0xF8),ROW(0x20),ROW(0x20),ROW(0x20),ROW(0x20),ROW(0x20),ROW(0x20)};
static const uint8_t glyph_U_bits[7] = {ROW(0x88),ROW(0x88),ROW(0x88),ROW(0x88),ROW(0x88),ROW(0x88),ROW(0x70)};
static const uint8_t glyph_V_bits[7] = {ROW(0x88),ROW(0x88),ROW(0x88),ROW(0x88),ROW(0x50),ROW(0x50),ROW(0x20)};
static const uint8_t glyph_W_bits[7] = {ROW(0x88),ROW(0x88),ROW(0x88),ROW(0xA8),ROW(0xA8),ROW(0xD8),ROW(0x88)};
static const uint8_t glyph_X_bits[7] = {ROW(0x88),ROW(0x50),ROW(0x20),ROW(0x20),ROW(0x20),ROW(0x50),ROW(0x88)};
static const uint8_t glyph_Y_bits[7] = {ROW(0x88),ROW(0x50),ROW(0x20),ROW(0x20),ROW(0x20),ROW(0x20),ROW(0x20)};
static const uint8_t glyph_Z_bits[7] = {ROW(0xF8),ROW(0x08),ROW(0x10),ROW(0x20),ROW(0x40),ROW(0x80),ROW(0xF8)};

static const BDGlyph furCaps_glyphs[26] = {
    {5,7,glyph_A_bits},{5,7,glyph_B_bits},{5,7,glyph_C_bits},{5,7,glyph_D_bits},{5,7,glyph_E_bits},
    {5,7,glyph_F_bits},{5,7,glyph_G_bits},{5,7,glyph_H_bits},{5,7,glyph_I_bits},{5,7,glyph_J_bits},
    {5,7,glyph_K_bits},{5,7,glyph_L_bits},{5,7,glyph_M_bits},{5,7,glyph_N_bits},{5,7,glyph_O_bits},
    {5,7,glyph_P_bits},{5,7,glyph_Q_bits},{5,7,glyph_R_bits},{5,7,glyph_S_bits},{5,7,glyph_T_bits},
    {5,7,glyph_U_bits},{5,7,glyph_V_bits},{5,7,glyph_W_bits},{5,7,glyph_X_bits},{5,7,glyph_Y_bits},
    {5,7,glyph_Z_bits}
};

const BDFont FONT_FUR_CAPS_5x7 = {'A','Z',furCaps_glyphs};

// =======================================================
// Extra symbols: %, °, /
// =======================================================

static const uint8_t glyph_percent_bits[7] = {0x88,0x08,0x10,0x20,0x40,0x88,0x00};
static const uint8_t glyph_degree_bits[7]  = {0x60,0x90,0x90,0x60,0x00,0x00,0x00};
static const uint8_t glyph_slash_bits[7]   = {0x08,0x10,0x20,0x40,0x80,0x00,0x00};
static const uint8_t glyph_hyphen_bits[7]  = {0x00,0x00,0x00,0x70,0x00,0x00,0x00};
static const uint8_t glyph_m_bits[7]       = {0x00,0x00,0xD0,0xA8,0xA8,0xA8,0xA8};

static const BDGlyph glyph_percent = {5,7,glyph_percent_bits};
static const BDGlyph glyph_degree  = {5,7,glyph_degree_bits};
static const BDGlyph glyph_slash   = {5,7,glyph_slash_bits};
static const BDGlyph glyph_hyphen  = {5,7,glyph_hyphen_bits};
static const BDGlyph glyph_m       = {5,7,glyph_m_bits};

// =======================================================
// Rendering helpers
// =======================================================

void drawGlyphScaled(int x, int y, char c,
                     const BDFont &font,
                     uint16_t color,
                     uint8_t scale)
{
    if (c < font.charFirst || c > font.charLast) return;
    const BDGlyph &g = font.glyphs[c - font.charFirst];

    for (uint8_t row = 0; row < g.height; row++) {
        uint8_t bits = g.bitmap[row];
        for (uint8_t col = 0; col < g.width; col++) {
            if (bits & (0x80 >> col)) {
                for (uint8_t dy = 0; dy < scale; dy++)
                for (uint8_t dx = 0; dx < scale; dx++)
                    drawPixel_FAST(x + col*scale + dx, y + row*scale + dy, color);
            }
        }
    }
}

// =======================================================
// drawTextScaled — final fixed ordering
// =======================================================

void drawTextScaled(int x, int y, const char *text,
                    const BDFont &font,
                    uint16_t color,
                    uint8_t scale)
{
    if (!text || scale == 0) return;
    int cx = x;

    while (*text) {
        unsigned char c = *text;

        // ---- 1) Percent — highest priority (must be first) ----
        if (c == '%') {
            const BDGlyph &g = glyph_percent;
            for (uint8_t row = 0; row < g.height; row++) {
                uint8_t bits = g.bitmap[row];
                for (uint8_t col = 0; col < g.width; col++) {
                    if (bits & (0x80 >> col)) {
                        for (uint8_t dy = 0; dy < scale; dy++)
                        for (uint8_t dx = 0; dx < scale; dx++)
                            drawPixel_FAST(cx + col*scale + dx, y + row*scale + dy, color);
                    }
                }
            }
            cx += (glyph_percent.width + 1) * scale;
            text++;
            continue;
        }

        // ---- 2) UTF-8 degree symbol 0xC2 0xB0 ----
        if (c == 0xC2 && (unsigned char)text[1] == 0xB0) {
            const BDGlyph &g = glyph_degree;
            for (uint8_t row = 0; row < g.height; row++) {
                uint8_t bits = g.bitmap[row];
                for (uint8_t col = 0; col < g.width; col++) {
                    if (bits & (0x80 >> col)) {
                        for (uint8_t dy = 0; dy < scale; dy++)
                        for (uint8_t dx = 0; dx < scale; dx++)
                            drawPixel_FAST(cx + col*scale + dx, y + row*scale + dy, color);
                    }
                }
            }
            cx += (g.width + 1) * scale;
            text += 2;
            continue;
        }

        // ---- 3) Single-byte ANSI degree (rare but supported) ----
        if (c == 0xB0 || c == 0xF8) {
            const BDGlyph &g = glyph_degree;
            for (uint8_t row = 0; row < g.height; row++) {
                uint8_t bits = g.bitmap[row];
                for (uint8_t col = 0; col < g.width; col++) {
                    if (bits & (0x80 >> col)) {
                        for (uint8_t dy = 0; dy < scale; dy++)
                        for (uint8_t dx = 0; dx < scale; dx++)
                            drawPixel_FAST(cx + col*scale + dx, y + row*scale + dy, color);
                    }
                }
            }
            cx += (glyph_degree.width + 1) * scale;
            text++;
            continue;
        }

        // ---- 4) Slash ----
        if (c == '/') {
            const BDGlyph &g = glyph_slash;
            for (uint8_t row = 0; row < g.height; row++) {
                uint8_t bits = g.bitmap[row];
                for (uint8_t col = 0; col < g.width; col++) {
                    if (bits & (0x80 >> col)) {
                        for (uint8_t dy = 0; dy < scale; dy++)
                        for (uint8_t dx = 0; dx < scale; dx++)
                            drawPixel_FAST(cx + col*scale + dx, y + row*scale + dy, color);
                    }
                }
            }
            cx += (glyph_slash.width + 1) * scale;
            text++;
            continue;
        }

        // ---- 5) Space ----
        if (c == ' ') {
            cx += (font.glyphs[0].width + 1) * scale;
            text++;
            continue;
        }

        // ---- 6) Normal ASCII glyph ----
        if (c >= font.charFirst && c <= font.charLast) {
            const BDGlyph &g = font.glyphs[c - font.charFirst];
            drawGlyphScaled(cx, y, c, font, color, scale);
            cx += (g.width + 1) * scale;
            text++;
            continue;
        }

        // ---- 7) Hyphen ----
        if (c == '-') {
            const BDGlyph &g = glyph_hyphen;
            for (uint8_t row = 0; row < g.height; row++) {
                uint8_t bits = g.bitmap[row];
                for (uint8_t col = 0; col < g.width; col++) {
                    if (bits & (0x80 >> col)) {
                        for (uint8_t dy = 0; dy < scale; dy++)
                        for (uint8_t dx = 0; dx < scale; dx++)
                            drawPixel_FAST(cx + col*scale + dx, y + row*scale + dy, color);
                    }
                }
            }
            cx += (glyph_hyphen.width + 1) * scale;
            text++;
            continue;
        }

        // ---- lowercase m ----
        if (c == 'm') {
            const BDGlyph &g = glyph_m;
            for (uint8_t row = 0; row < g.height; row++) {
                uint8_t bits = g.bitmap[row];
                for (uint8_t col = 0; col < g.width; col++) {
                    if (bits & (0x80 >> col)) {
                        for (uint8_t dy = 0; dy < scale; dy++)
                        for (uint8_t dx = 0; dx < scale; dx++)
                            drawPixel_FAST(cx + col*scale + dx, y + row*scale + dy, color);
                    }
                }
            }
            cx += (g.width + 1) * scale;
            text++;
            continue;
        }

        // ---- 7) Fallback skip ----
        cx += 2 * scale;
        text++;
    }
}

// =======================================================
// measureTextWidth — supports all symbols
// =======================================================

int measureTextWidth(const char *text,
                     const BDFont &font,
                     uint8_t scale)
{
    if (!text) return 0;
    int px = 0;

    while (*text) {
        unsigned char c = *text++;

        if (c == '%')      { px += (glyph_percent.width + 1) * scale; continue; }
        if (c == 0xC2 && (unsigned char)*text == 0xB0) { px += (glyph_degree.width + 1) * scale; text++; continue; }
        if (c == 0xB0 || c == 0xF8) { px += (glyph_degree.width + 1) * scale; continue; }
        if (c == '/')      { px += (glyph_slash.width + 1) * scale; continue; }
        if (c == '-')      { px += (glyph_hyphen.width + 1) * scale; continue; }
        if (c == 'm')      { px += (glyph_m.width + 1) * scale; continue; }
        if (c == ' ')      { px += (font.glyphs[0].width + 1) * scale; continue; }

        if (c >= font.charFirst && c <= font.charLast) {
            const BDGlyph &g = font.glyphs[c - font.charFirst];
            px += (g.width + 1) * scale;
        } else {
            px += 2 * scale;
        }
    }

    return px;
}

// =======================================================
// Centers VALUE + UNIT block
// =======================================================

void drawValueUnitCentered(int x0, int y0, int x1, int y1,
                           const char *value,
                           uint8_t fieldDigits,
                           const char *unit,
                           const BDFont &fontValue,
                           uint8_t scaleValue,
                           const BDFont &fontUnit,
                           uint8_t scaleUnit,
                           uint16_t color)
{
    int digitW = (fontValue.glyphs[0].width + 1) * scaleValue;
    int valueW = digitW * strlen(value);
    int unitW  = measureTextWidth(unit, fontUnit, scaleUnit);

    int totalWidth = valueW + (unitW > 0 ? (digitW / 2) : 0) + unitW;
    int cx = (x0 + x1) / 2;
    int left = cx - totalWidth / 2;

    int valueTop = y0 + 20;
    drawTextScaled(left, valueTop, value, fontValue, color, scaleValue);

    if (unitW > 0) {
        int unitTop = valueTop + 7 * scaleValue - 7 * scaleUnit;
        drawTextScaled(left + valueW + (digitW / 2), unitTop, unit, fontUnit, color, scaleUnit);
    }
}
