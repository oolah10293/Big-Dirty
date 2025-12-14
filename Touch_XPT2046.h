#pragma once
#include <stdint.h>
#include <XPT2046_Touchscreen.h>

// Touch controller pin mapping (Teensy 4.1)
#define TOUCH_CS   22
#define TOUCH_IRQ  21

// Basic touch point
struct TouchPoint {
    int16_t x;
    int16_t y;
    bool touched;
};

// Init touch hardware
void initTouch();

// Get touch state
TouchPoint readTouch();

// true if finger is touching screen
bool touchPressed();   

void Touch_reset();
