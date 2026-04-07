#pragma once
#include <stdint.h>

// Initialize overlay system
void Overlay_Warnings_begin();

// Update warning icon states
// true  = show icon
// false = hide icon
void Overlay_Warnings_update(
    bool seatbeltOn,
    bool brakeOn,
    bool highBeamOn,
    bool checkEngineOn,
    bool fourWdOn
);