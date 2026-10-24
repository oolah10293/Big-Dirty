#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "PageManager.h"
#include "HC58_Transport.h"

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
void PIDManager_onHc58Frame(const HC58Transport::Frame &frame);

bool PIDManager_isValid(PIDSlot slot);
const char* PIDManager_getValue(PIDSlot slot);
const char* PIDManager_getDisplayText(PIDSlot slot);
