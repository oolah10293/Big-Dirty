#include "Touch_XPT2046.h"
#include "PinConfig.h"
#include <SPI.h>

// Raw XPT2046 instance
static XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// Internal edge detection state
static bool lastPressed = false;

// Reset touch state (called by PageManager after a page switch)
void Touch_reset() {
    lastPressed = false;
}

void initTouch() {
    ts.begin();
    ts.setRotation(3);   // match display orientation
    Touch_reset();       // ensure clean start
}

// This returns ONE press event per real tap (edge-triggered)
TouchPoint readTouch() {
    bool pressed = ts.touched();

    // Rising edge = NEW PRESS → generate touch event ONCE
    if (pressed && !lastPressed) {
        lastPressed = true;

        TS_Point p = ts.getPoint();

        const int RAW_MIN = 200;
        const int RAW_MAX = 3800;

        int16_t sx = map(p.x, RAW_MIN, RAW_MAX, 0, 479);
        int16_t sy = map(p.y, RAW_MIN, RAW_MAX, 0, 319);

        if (sx < 0)   sx = 0;
        if (sx > 479) sx = 479;
        if (sy < 0)   sy = 0;
        if (sy > 319) sy = 319;

        return { sx, sy, true };   // ← ONE event per tap
    }

    // Falling edge — release → reset state ONLY
    if (!pressed && lastPressed) {
        lastPressed = false;
    }

    // Anything else → no event
    return { 0, 0, false };
}

bool touchPressed() {
    return ts.touched();
}
