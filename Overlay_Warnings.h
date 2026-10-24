#pragma once
#include <stdint.h>

void Overlay_Warnings_begin();

// Call immediately after a full-screen black clear. It marks all warning slots
// physically empty so the next update draws only warnings that are actually on.
void Overlay_Warnings_screenCleared();

void Overlay_Warnings_update(
    bool seatbeltOn,
    bool brakeOn,
    bool highBeamOn,
    bool checkEngineOn,
    bool fourWdOn
);
