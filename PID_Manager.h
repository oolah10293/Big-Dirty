#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "PageManager.h"

enum PIDSlot {
    PID_SLOT_Q1 = 0,
    PID_SLOT_Q2 = 1,
    PID_SLOT_Q3 = 2,
    PID_SLOT_Q4 = 3,
    PID_SLOT_COUNT = 4
};

void PIDManager_begin();
void PIDManager_setPage(PageID page);
void PIDManager_update();

bool PIDManager_isValid(PIDSlot slot);
const char* PIDManager_getValue(PIDSlot slot);
const char* PIDManager_getDisplayText(PIDSlot slot);